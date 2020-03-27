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

//Parses the input .cnf file
//Sets up structures that are common for all instances of BooleanSolver
//Does not add the clauses and the variables to a particular SAT solver,
//this is done in the constructor of the particular solver (inherited class of BooleanSolver)
bool BooleanSolver::parse(string path){
        ifstream infile(path, ifstream::in);
        if (!infile.is_open())
                print_err("wrong input file");

        string line;
        vector<int> clause;
        string pom;
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
                else if(clauses_unique_map.find(line) != clauses_unique_map.end()){
                        cout << "a duplicate clause found in the input formula" << endl;
                        continue;
                }
                else{
                        clauses_str.push_back(line);
                        clauses_unique_map[line] = clauses_str.size() - 1;
                }
        }
        cout << "vars: " << vars << endl;
        hitmap_pos.resize(vars);
        hitmap_neg.resize(vars);
        for(size_t i = 0; i < clauses_str.size(); i++){
                clause = convert_clause(clauses_str[i]);
                clause.push_back(vars + i + 1); //control variable
                add_clause(clause); //add clause to the solver
        }
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
		} else {
			for(auto c1: hitmap_neg[var - 1]){ //clauses satisfied by implied negative value of i-th literal
				if(subset[c1] && c1 != c2){ count++; }
			}
		}
		if(count == 0) return false;
	}
	return true;
}


vector<bool> BooleanSolver::propagateToUnsat(vector<bool> base, vector<bool> cover, vector<int> implied){
	queue<int> toPropagate;
	for(auto l: implied)
		toPropagate.push(l);
	vector<bool> conflict = base;
	vector<int> lits_left (dimension,0);
	for(int i = 0 ; i < dimension; i++){
		lits_left[i] = clauses[i].size() - 1; // -1 because there is the activation literal
	}
	vector<bool> satisfied(dimension, false);
	vector<bool> setup(vars + 1, false);
	vector<bool> value(vars + 1, false);
	while(!toPropagate.empty()){
		auto l = toPropagate.front();
		toPropagate.pop();
		int var = (l > 0)? l : -l;
		if(setup[var])
			continue;
		setup[var] = true;
		value[var] = l < 0;
		if(l < 0){
			for(auto c1: hitmap_neg[var - 1]){ //clauses satisfied by implied negative value of i-th literal
				satisfied[c1] = true;
			}
			for(auto c1: hitmap_pos[var - 1]){ //trim clauses that contain positive i-th literal
				if(satisfied[c1] || !cover[c1]) continue;
				lits_left[c1]--;
				if(lits_left[c1] == 1){
					for(auto lit: clauses[c1]){
						if(lit >= vars) break; //the activation literal
						int var2 = (lit > 0)? lit : -lit;
						if(!setup[var2]){
							toPropagate.push(lit);
							implied.push_back(lit);
							break;
						}
					}
					conflict[c1] = true;
				}
				if(lits_left[c1] == 0){
					conflict[c1] = true;
					return conflict;
				}
			}
		}else{
			for(auto c1: hitmap_pos[var - 1]){ //clauses satisfied by implied negative value of i-th literal
				satisfied[c1] = true;
			}
			for(auto c1: hitmap_neg[var - 1]){ //trim clauses that contain positive i-th literal
				if(satisfied[c1] || !cover[c1]) continue;
				lits_left[c1]--;
				if(lits_left[c1] == 1){
					for(auto lit: clauses[c1]){
						if(lit >= vars) break; //the activation literal
						int var2 = (lit > 0)? lit : -lit;
						if(!setup[var2]){
							toPropagate.push(lit);
							implied.push_back(lit);
							break;
						}
					}
					conflict[c1] = true;
				}
				if(lits_left[c1] == 0){
					conflict[c1] = true;
					return conflict;
				}
			}
		}
	}
	return vector<bool>();
}
vector<int> BooleanSolver::get_implied(vector<bool> mus, int c){
	if(!mus[c]) print_err("c is not in mus");
	mus[c] = false;
	vector<int> lits_left (dimension,0);
	for(int i = 0 ; i < dimension; i++){
		lits_left[i] = clauses[i].size() - 1; // -1 because there is the activation literal
	}
	vector<bool> satisfied(dimension, false);
	queue<int> to_propagate;
	vector<bool> setup(vars + 1, false);
	vector<bool> value(vars + 1, false);
	vector<int> model;
	for(auto l: clauses[c]){
		int var = (l > 0)? l : -l;
		if(var <= vars){
			to_propagate.push(-l);
			model.push_back(-l);
		}
	}
	while(!to_propagate.empty()){
		auto l = to_propagate.front();
		to_propagate.pop();
		int var = (l > 0)? l : -l;
		if(setup[var])
			continue;
		setup[var] = true;
		value[var] = l < 0;
		model.push_back(l);
		if(l < 0){
			for(auto c1: hitmap_neg[var - 1]){ //clauses satisfied by implied negative value of i-th literal
				satisfied[c1] = true;
			}
			for(auto c1: hitmap_pos[var - 1]){ //trim clauses that contain positive i-th literal
				if(!mus[c1] || satisfied[c1]) continue;
				lits_left[c1]--;
				if(lits_left[c1] == 1){
					for(auto lit: clauses[c1]){
						int var2 = (lit > 0)? lit : -lit;
						if(!setup[var2]){
							to_propagate.push(lit);
							break;
						}
					}
				}
				if(lits_left[c1] == 0){
					print_err("conflict during propagation");
				}
			}
		}else{
			for(auto c1: hitmap_pos[var - 1]){ //clauses satisfied by implied negative value of i-th literal
				satisfied[c1] = true;
			}
			for(auto c1: hitmap_neg[var - 1]){ //trim clauses that contain positive i-th literal
				if(!mus[c1] || satisfied[c1]) continue;
				lits_left[c1]--;
				if(lits_left[c1] == 1){
					for(auto lit: clauses[c1]){
						int var2 = (lit > 0)? lit : -lit;
						if(!setup[var2]){
							to_propagate.push(lit);
							break;
						}
					}
				}
				if(lits_left[c1] == 0){
					print_err("conflict during propagation");
				}
			}
		}
	}
	return model;
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
        int formulas = std::count(f.begin(), f.end(), true);
        FILE *file;
        file = fopen(filename.c_str(), "w");
        if(file == NULL) cout << "failed to open " << filename << ", err: " << strerror(errno) << endl;

        fprintf(file, "p gcnf %d %d %d\n", vars, formulas, formulas);
        for(int i = 0; i < f.size(); i++)
                if(f[i] && crits[i]){
			fprintf(file, "{0} %s\n", clauses_str[i].c_str());
                }
        int group = 1;
        for(int i = 0; i < f.size(); i++)
                if(f[i] && !crits[i]){
                        fprintf(file, "{%d} %s\n", group++, clauses_str[i].c_str());
                }
        if (fclose(file) == EOF) {
                cout << "error closing file: " << strerror(errno) << endl;
        }
}

vector<bool> BooleanSolver::import_formula_crits(string filename){
        vector<bool> f(dimension, false);
        vector<vector<int>> cls;
        ReMUS::parse_DIMACS(filename, cls);
        for(auto cl: cls){
                sort(cl.begin(), cl.end());
                if(clauses_map.count(cl))
                        f[clauses_map[cl]] = true;
                else { assert(false); }
        }
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
        int status = system(cmd.str().c_str());
        if(status < 0){
                std::cout << "Invalid muser return code" << std::endl; exit(0);
        }
        imp << ".gcnf";
        vector<bool> mus = import_formula_crits(imp.str());
        cout << cmd.str() << endl;
	int sat_calls = muser_output(muser_out.str());
//      checks += sat_calls;
        remove(imp.str().c_str());
        remove(muser_out.str().c_str());
        remove(input.c_str());
        return mus;
}

std::vector<bool> BooleanSolver::grow(std::vector<bool> &f, std::vector<bool> conflicts){
        grows++;
        if(grow_alg == "default"){
                return SatSolver::grow(f, conflicts);
        }else if(grow_alg == "uwr"){
                return grow_uwrmaxsat(f, conflicts);
        }else if(grow_alg == "mcsls"){
                std::vector<std::vector<bool>> ms = growMultiple(f, conflicts, 1);
                if(!ms.empty()) // there is a chance that mcsls fail; in such a case, we proceed with grow_cmp
                        return ms.back();
        }
        return grow_cmp(f, conflicts);
}

std::vector<bool> BooleanSolver::grow_cmp(std::vector<bool> &f, std::vector<bool> &conflicts){
        cout << "growing cmp" << endl;
        stringstream ex;
        ex << "./tmp/f_" << hash << ".wcnf";
        vector<int> mapa = export_formula_wcnf(f, conflicts, ex.str());
        stringstream cmd;
        cmd << "./cmp_linux -strategyCoMss=" << growStrategy << " " << ex.str();
        string result = exec(cmd.str().c_str());

        std::string line;
        bool reading = false;
        std::vector<int> res_mcs;
        std::istringstream res(result);
        while (std::getline(res, line)) {
                if (line.find("UNSATISFIABLE") != std::string::npos)
                        reading = true;
                else if(reading && line[0] == 'v'){
                        line.erase(0, 2);
                        res_mcs = convert_clause(line);
                }
        }
        cout << "mcs size: " << res_mcs.size() << endl;
        if(!reading)
                print_err("cmp failed to find a mcs");
        vector<bool> mss(dimension, true);
        for(int i = 0; i < dimension; i++)
                if(conflicts[i])
                        mss[i] = false;
        for(auto l:res_mcs){
                mss[mapa[l - 1]] = false;
        }
        return mss;
}

std::vector<bool> BooleanSolver::grow_uwrmaxsat(std::vector<bool> &f, std::vector<bool> &conflicts){
        stringstream ex;
        ex << "./tmp/uwr_" << hash << ".wcnf";
        vector<int> mapa = export_formula_wcnf(f, conflicts, ex.str());
        stringstream cmd;
        cmd << "./uwrmaxsat -no-msu -m -v0 " << ex.str();
        string result = exec(cmd.str().c_str());

        std::string line;
        std::vector<int> res_vars;
        std::istringstream res(result);
        while (std::getline(res, line)) {
                if(line[0] == 'v' && line[1] == ' '){
                        line.erase(0, 2);
                        istringstream is(line);
                        int n;
                        while(is >> n)
                                res_vars.push_back(n);
                }
        }
        return satisfied(res_vars);
}

vector<int> BooleanSolver::export_formula_wcnf(std::vector<bool> f, std::vector<bool> &conflicts, std::string filename){
        int formulas = dimension - std::count(conflicts.begin(), conflicts.end(), true);
        int hard = std::count(f.begin(), f.end(), true);
        int hh = dimension * dimension;

        FILE *file;
        file = fopen(filename.c_str(), "w");
        if(file == NULL) cout << "failed to open " << filename << ", err: " << strerror(errno) << endl;
        vector<int> mapa;
        fprintf(file, "p wcnf %d %d %d\n", vars, formulas, hh);
        for(int i = 0; i < f.size(); i++){
                if(f[i]){
                        fprintf(file, "%d %s\n", hh, clauses_str[i].c_str());
                        mapa.push_back(i);
                }else if(!f[i] && !conflicts[i]){
                        fprintf(file, "1 %s\n", clauses_str[i].c_str());
                        mapa.push_back(i);
                }
        }
        if (fclose(file) == EOF) {
                cout << "error closing file: " << strerror(errno) << endl;
        }
        return mapa;
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
        int formulas = std::count(f.begin(), f.end(), true);
        stringstream result;
        result << "p cnf " << vars << " " << formulas << "\n";
        for(int i = 0; i < f.size(); i++)
                if(f[i]){
                        result << clauses_str[i] << "\n";
                }
        return result.str();
}
