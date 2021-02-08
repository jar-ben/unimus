#include "satSolvers/Dimacs.h"
#include "core/misc.h"
#include "satSolvers/BooleanSolver.h"
#include <sstream>
#include <fstream>
#include <sstream>
#include <iostream>
#include <ctime>
#include <algorithm>
#include <random>
#include <stdlib.h>   
#include <time.h>   
#include <cstdio>
#include <assert.h>
#include <queue>

using namespace std;

string trimGid(string clause, string &gid){
	if(!clause[0] == '{')
		return clause;
        istringstream is(clause);
	string cl;
	is >> gid;
	getline(is, cl);
	trim(cl);
	return cl;
}

vector<int> BooleanSolver::convert_clause(string clause){
	vector<int> ret;
        istringstream is(clause);
        int n;
        while(is >> n)
                ret.push_back(n);
        ret.pop_back();
        return ret;
}

void BooleanSolver::add_clause(vector<int> cl){
        std::sort(cl.begin(), cl.end());
        vector<int> copy = cl;
        copy.pop_back(); //get rid of control variable
        clauses.push_back(cl);
        clauses_map[copy] = clauses.size() - 1; //used for manipulation with single MUS extractions (muser2, dmuser)
        for(auto &lit: copy){
                if(lit > 0)
                        hitmap_pos[lit - 1].push_back(clauses.size() - 1);
                else
                        hitmap_neg[(-1 * lit) - 1].push_back(clauses.size() - 1);
        }
}

void BooleanSolver::add_hard_clause(vector<int> cl){
        std::sort(cl.begin(), cl.end());
        hard_clauses.push_back(cl);
        hard_clauses_map[cl] = hard_clauses.size() - 1; //used for manipulation with single MUS extractions (muser2, dmuser)
        for(auto &lit: cl){
                if(lit > 0)
                        hard_hitmap_pos[lit - 1].push_back(hard_clauses.size() - 1);
                else
                        hard_hitmap_neg[(-1 * lit) - 1].push_back(hard_clauses.size() - 1);
        }
}

//Parses the input .cnf or .gcnf file
//Sets up structures that are common for all instances of BooleanSolver
//Does not add the clauses and the variables to a particular SAT solver,
//this is done in the constructor of the particular solver (inherited class of BooleanSolver)
bool BooleanSolver::parse(string path){
	gcnf = false;
	ifstream infile(path, ifstream::in);
        if (!infile.is_open())
                print_err("wrong input file");

        string line;
        vector<int> clause;
        string pom;
	string gid;
        while (getline(infile, line))
        {
                if (line[0] == 'p'){
                        istringstream is(line);
                        is >> pom;      // p
                        is >> pom;      // cnf
                        is >> vars;     //number of variables
                }
                else if(line[0] == 'c')
                        continue;
		else if(starts_with(line, "{0} ")){
			string c = trimGid(line, gid);
                	if(clauses_unique_map.find(c) != clauses_unique_map.end()){
				cout << "A duplicate clause found in the input formula: " << c << endl;
			}
			hard_clauses_str.push_back(c);
			hard_clauses_gid.push_back(gid);
			clauses_unique_map[c] = hard_clauses_str.size() - 1;
		}
		else if(starts_with(line, "{")){
			gcnf = true;
			string c = trimGid(line, gid);
                	if(clauses_unique_map.find(c) != clauses_unique_map.end()){
				cout << "WARNING: A duplicate clause found in the input formula: " << c << endl;
				cout << "The group id of the duplicated clause is " << gid << endl;
				cout << "We skip the duplicated clause (only the first instance of the clause will be used during the computation)." << endl;
				cout << endl;
			}else{
				clauses_str.push_back(c);
				clauses_gid.push_back(gid);
				clauses_unique_map[c] = clauses_str.size() - 1;
			}
		}
                else{
                	if(clauses_unique_map.find(line) != clauses_unique_map.end()){
				cout << "WARNING: A duplicate clause found in the input formula: " << line << endl;
				cout << "We skip the duplicate clause (only the first instance of the clause will be used during the computation)." << endl;
				cout << endl;
			}else{
				clauses_str.push_back(line);
				clauses_unique_map[line] = clauses_str.size() - 1;
			}
                }
        }
        hitmap_pos.resize(vars);
        hitmap_neg.resize(vars);
        hard_hitmap_pos.resize(vars);
        hard_hitmap_neg.resize(vars);
        for(size_t i = 0; i < clauses_str.size(); i++){
                clause = convert_clause(clauses_str[i]);
                clause.push_back(vars + i + 1); //control variable
                add_clause(clause); //add clause to the solver
        }
        for(size_t i = 0; i < hard_clauses_str.size(); i++){
                clause = convert_clause(hard_clauses_str[i]);
                add_hard_clause(clause); //add clause to the solver
        }
	cout << "hard clauses: " << hard_clauses.size() << endl;
	cout << "soft clauses: " << clauses.size() << endl;
	dimension = clauses.size();
	srand (time(NULL));
	rotated_crits = 0;
	flip_edges_computed.resize(dimension, false);
	flip_edges.resize(dimension);
	flip_edges_flatten.resize(dimension);

        return true;
}
BooleanSolver::BooleanSolver(string filename):SatSolver(filename){
}

BooleanSolver::~BooleanSolver(){
}

void print_clause(vector<int> cl){
	cout << "clause: ";
	for(auto l: cl)
		cout << l << " ";
	cout << endl;
}

void print_model(vector<int> model){
	for(auto l: model)
		cout << l << " ";
	cout << endl;
}

bool BooleanSolver::lit_occurences(vector<bool> subset, int c2){
	for(auto l: clauses[c2]){
		int var = (l > 0)? l : -l;
		if(var > vars) break; //the activation literal
		int count = 0;
		if(l > 0){
			for(auto c1: hitmap_pos[var - 1]){ //clauses satisfied by implied negative value of i-th literal
				if(subset[c1] && c1 != c2){ count++; }
			}
			//optional - using hard clauses 
			count += hard_hitmap_pos[var - 1].size();
		} else {
			for(auto c1: hitmap_neg[var - 1]){ //clauses satisfied by implied negative value of i-th literal
				if(subset[c1] && c1 != c2){ count++; }
			}
			//optional - using hard clauses 
			count += hard_hitmap_neg[var - 1].size();
		}
		if(count == 0) return false;
	}
	return true;
}

void BooleanSolver::compute_flip_edges(int c){
	if(flip_edges_computed.empty()){
		flip_edges_computed.resize(dimension, false);
		flip_edges.resize(dimension);
		flip_edges_flatten.resize(dimension);
	}
	if(flip_edges_computed[c]) return;
	flip_edges_computed[c] = true;

	vector<bool> flatten(dimension, false);
	for(int l = 0; l < clauses[c].size() - 1; l++){
		auto lit = clauses[c][l];
		vector<int> edges;
		if(lit > 0){
			for(auto &h: hitmap_neg[lit - 1]){
				if(h != c){
					edges.push_back(h);
					flatten[h] = true;
				}
			}
		}
		else{
			for(auto &h: hitmap_pos[(-1 * lit) - 1]){
				if(h != c){
					edges.push_back(h);			
					flatten[h] = true;
				}
			}
		}
		flip_edges[c].push_back(edges);
	}
	for(int i = 0; i < dimension; i++)
		if(flatten[i])
			flip_edges_flatten[c].push_back(i);
}

void BooleanSolver::criticals_rotation(vector<bool>& criticals, vector<bool> subset){
	vector<int> criticals_int;
	for(int i = 0; i < dimension; i++)
		if(criticals[i] && subset[i]) criticals_int.push_back(i);

	for(int i = 0; i < criticals_int.size(); i++){
		int c = criticals_int[i];
		compute_flip_edges(c); //TODO: better encansulape
		for(auto &lit_group: flip_edges[c]){
			int count = 0;			
			int flip_c;	
			for(auto c2: lit_group){
				if(subset[c2]){ count++; flip_c = c2; }
			}
			if(count == 1 && !criticals[flip_c]){
				criticals_int.push_back(flip_c);
				criticals[flip_c] = true;
			}
		}
	}
}

vector<int> BooleanSolver::critical_extension_clause(vector<bool> &f, vector<bool> &crits, int c){
	vector<int> extensions;
	for(auto l: clauses[c]){
		vector<int> hits;
		int var = (l > 0)? l - 1 : (-l) - 1;
		if(var >= vars) break; //these are the control variables
		if(l > 0){
			for(auto c2: hitmap_neg[var]){
				if(f[c2])
					hits.push_back(c2);
			}
		}else{
			for(auto c2: hitmap_pos[var]){
				if(f[c2])
					hits.push_back(c2);
			}
		}
		if(hits.size() == 1 && !crits[hits[0]]){
			extensions.push_back(hits[0]);
			crits[hits[0]] = true;
		}
	}
	return extensions;
}

int BooleanSolver::critical_extension(vector<bool> &f, vector<bool> &crits){
	int initSize = count_ones(crits);
        queue<int> S;
        for(int i = 0; i < dimension; i++){
                if(crits[i]){
                        S.push(i);
                }
        }

        while(!S.empty()){
                int c = S.front();
                S.pop();
		for(auto e: critical_extension_clause(f, crits, c))
			S.push(e);
        }
	return count_ones(crits) - initSize;
}

int BooleanSolver::critical_propagation(vector<bool> &f, vector<bool> &crits, int cl){
        int extensions = 0;
	queue<int> S;
	S.push(cl);
        while(!S.empty()){
		extensions++;
                int c = S.front();
                S.pop();
		for(auto e: critical_extension_clause(f, crits, c))
			S.push(e);
        }
	return extensions - 1; //exclude the initical clause cl
}

// implementation of the shrinking procedure
// based on the value of basic_shrink employes either muser2 or dmuser
vector<bool> BooleanSolver::shrink(std::vector<bool> &f, std::vector<bool> crits){
        shrinks++;
	if(crits.empty())
		crits = std::vector<bool> (dimension, false);
        if(shrink_alg == "custom"){
                return SatSolver::shrink(f, crits); //shrink with unsat cores
        }
        if(shrink_alg == "extension"){
		vector<bool> s = f;
		int extensions = 0;
		for(int i = 0; i < dimension; i++){
			if(s[i] && !crits[i]){
				s[i] = false;
				if(solve(s, true, false)){
					s[i] = true;
					crits[i] = true;
					extensions += critical_propagation(f, crits, i);
				}
			}
		}
		return s;
        }
        if(shrink_alg == "default"){
		vector<bool> mus;
                try{
                        mus = shrink_mcsmus(f, crits);
                } catch (...){
                        //mcsmus sometimes fails so we use muser instead
                        cout << "mcsmus crashed during shrinking, using muser2 instead" << endl;
                        stringstream exp;
                        exp << "./tmp/f_" << hash << ".cnf";
                        export_formula_crits(f, exp.str(), crits);
                        mus = shrink_muser(exp.str(), hash);
                }
                return mus;
        }
        stringstream exp;
        exp << "./tmp/f_" << hash << ".cnf";
        export_formula_crits(f, exp.str(), crits);
        return shrink_muser(exp.str(), hash);
}

vector<bool> BooleanSolver::shrink(std::vector<bool> &f, Explorer *e, std::vector<bool> crits){
        shrinks++;
        if(crits.empty())
                crits = std::vector<bool> (dimension, false);
        vector<bool> s = f;
	int extensions = 0;
        for(int i = 0; i < dimension; i++){
                if(s[i] && !crits[i]){
                        s[i] = false;
                        if(solve(s, true, false)){
                                s[i] = true;
				crits[i] = true;
				extensions += critical_propagation(f, crits, i);
			}
                }
        }
	cout << "crit extensions: " << extensions << endl;
        return s;
}

void BooleanSolver::export_formula_crits(vector<bool> f, string filename, vector<bool> crits){
        int crits_count = std::count(crits.begin(), crits.end(), true);
	int soft_count = std::count(f.begin(), f.end(), true) - crits_count;
        FILE *file;
        file = fopen(filename.c_str(), "w");
        if(file == NULL) cout << "failed to open " << filename << ", err: " << strerror(errno) << endl;

        fprintf(file, "p gcnf %d %d %d\n", vars, soft_count + crits_count + hard_clauses.size(), soft_count);
	for(auto &cl: hard_clauses_str){
		fprintf(file, "{0} %s\n", cl.c_str());
	}
	if(DBG){ cout << "crit clauses:"; }
	for(int i = 0; i < f.size(); i++)
                if(f[i] && crits[i]){
			fprintf(file, "{0} %s\n", clauses_str[i].c_str());
			if(DBG){ cout << " " << i; }
                }
        int group = 1;
	if(DBG){ cout << endl << "soft clauses:"; }
        for(int i = 0; i < f.size(); i++)
                if(f[i] && !crits[i]){
                        fprintf(file, "{%d} %s\n", group++, clauses_str[i].c_str());
			if(DBG){ cout << " " << i; }
                }
	if(DBG){ cout << endl; }
        if (fclose(file) == EOF) {
                cout << "error closing file: " << strerror(errno) << endl;
        }
}

vector<bool> BooleanSolver::import_formula_crits(string filename){
        vector<bool> f(dimension, false);
        vector<vector<int>> cls;
        ReMUS::parse_DIMACS(filename, cls);
	if(DBG){ cout << "clauses in the MUS:";	}
        for(auto cl: cls){
                sort(cl.begin(), cl.end());
                if(clauses_map.count(cl)) //add only soft clauses (the hard ones are implicit)
                        f[clauses_map[cl]] = true;
			if(DBG){ cout << " " << clauses_map[cl]; }
        }
	if(DBG) cout << endl;
        return f;
}


int BooleanSolver::muser_output(std::string filename){
        ifstream file(filename, ifstream::in);
        std::string line;
        while (getline(file, line))
        {
                if (line.find("c Calls") != std::string::npos){
                        std::string calls = line.substr(line.find(":") + 2, line.size()); // token is "scott"
                        return atoi(calls.c_str());
                }
        }
        return 0;
}

vector<bool> BooleanSolver::shrink_muser(string input, int hash2){
        stringstream cmd, muser_out, imp;
        muser_out << "./tmp/f_" << hash << "_output";
        imp << "./tmp/f_" << hash << "_mus";
        cmd << "./muser2-para -grp -wf " << imp.str() << " " << input << " > " << muser_out.str();// */ " > /dev/null";
	if(DBG){ cout << cmd.str() << endl; }
	int status = system(cmd.str().c_str());
        if(status < 0){
                std::cout << "Invalid muser return code" << std::endl; exit(0);
        }
        imp << ".gcnf";
        vector<bool> mus = import_formula_crits(imp.str());
	int sat_calls = muser_output(muser_out.str());
//      checks += sat_calls;
        if(!DBG){
		remove(imp.str().c_str());
		remove(muser_out.str().c_str());
		remove(input.c_str());
	}
        return mus;
}


std::vector<bool> BooleanSolver::satisfied(std::vector<int> &valuation){
        std::vector<bool> result(dimension, false);
        for(auto l: valuation){
                if(l > 0){
                        for(auto c: hitmap_pos[l - 1]){
                                result[c] = true;
                        }
                }else{
                        for(auto c: hitmap_neg[(-1 * l) - 1]){
                                result[c] = true;
                        }
                }
        }
        return result;
}

string BooleanSolver::toString(vector<bool> &f){
	int f_size = std::count(f.begin(), f.end(), true);
        int formulas = f_size + hard_clauses.size();
        stringstream result;
	if(gcnf){
		result << "p gcnf " << vars << " " << formulas << " " << f.size() << "\n";
		for(int i = 0; i < hard_clauses.size(); i++){
			result << hard_clauses_gid[i] << hard_clauses_str[i] << "\n";
		}
		for(int i = 0; i < f.size(); i++){
			if(f[i]){
				result << clauses_gid[i] << clauses_str[i] << "\n";
			}
		}
	}
	else{
		result << "p cnf " << vars << " " << formulas << "\n";
		for(int i = 0; i < f.size(); i++){
			if(f[i]){
				result << clauses_str[i] << "\n";
			}
		}
	}	
	return result.str();
}
