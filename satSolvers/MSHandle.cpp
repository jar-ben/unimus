#include "core/misc.h"
#include "satSolvers/MSHandle.h"
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

using namespace CustomMinisat;
using namespace std;

Lit itoLit(int i){
	bool sign = i < 0;
	int var = (sign)? -i-1 : i-1;
	return (sign) ? ~mkLit(var) : mkLit(var);
}

int Littoi(Lit l){
	return (var(l)+1) * (sign(l) ? -1 : 1);
}

MSHandle::MSHandle(string filename):BooleanSolver(filename){
	solver = new Solver();
	vars = 0;
	parse(filename);
	
	//add variables to the solver
	for(int i = 0; i < vars; i++){
		solver->newVar();	// clause variables
	}
	for(int i = 0; i < clauses_str.size(); i++)
		solver->newVar(lbool(uint8_t(1)), true);	// control variables
	
	//add clauses to the solver
	for(auto &cl: clauses){
		vec<Lit> msClause;
		for(auto &lit: cl)
			msClause.push(itoLit(lit));		
		solver->addClause(msClause);
	}

	//add hard clauses to the solver
	for(auto &cl: hard_clauses){
		vec<Lit> msClause;
		for(auto &lit: cl)
			msClause.push(itoLit(lit));		
		solver->addClause(msClause);
	}
}

MSHandle::~MSHandle(){
	delete solver;
}

vector<bool> MSHandle::model_extension(vector<bool> subset, vector<bool> model){
	int flipped = 0;
	vector<bool> extension = subset;
	for(int i = 0; i < extension.size(); i++){
		if(!extension[i]){
			for(int j = 0; j < clauses[i].size() - 1; j++){
				int lit = clauses[i][j];
				if(lit > 0 && model[lit - 1]){
					extension[i] = true;
					flipped++;
					break;
				}
				else if(lit < 0 && !model[(-1 * lit) - 1]){
					extension[i] = true;
					flipped++;
					break;
				}				
			}
		}
	}
	return extension;
}

bool MSHandle::solve(std::vector<bool> &controls, std::vector<int> conflicts, bool unsat_improve, bool sat_improve){
	checks++;
	vec<Lit> lits;
	for(unsigned int i = 0; i < controls.size(); i++){
		if(controls[i])
			lits.push(itoLit((i + vars + 1) * (-1)));
		else{
			lits.push(itoLit(i + vars + 1 ));
		}
	}

	//for(auto c: conflicts){
	//	for(auto l: clauses[c]){
	//		lits.push(itoLit(l)); 
	//	}
	//}

	bool sat = solver->solve(lits);
	if(sat && sat_improve){ // extract model extension		
		int initSize = count_ones(controls);
		for(int f = 0; f < controls.size(); f++){
			if(!controls[f]){
				for(int l = 0; l < clauses[f].size() - 1; l++){
					if(clauses[f][l] > 0){
						if(solver->model[clauses[f][l] - 1] == l_True){
							controls[f] = true;
							break;
						}
					}
					else{
						if(solver->model[-1 * clauses[f][l] - 1] == l_False){
							controls[f] = true;
							break;
						}
					}
				}
			}
		}
		int finalSize = count_ones(controls);
	}			
	else if(!sat && unsat_improve){ // extract unsat core
		vector<bool> core = vector<bool> (dimension, false);		
	        for (int i = 0 ; i < solver->conflict.size() ; i++) 
			//if(var(solver->conflict[i]) - vars > 0 && var(solver->conflict[i]) - vars < dimension)
				core[var(solver->conflict[i]) - vars] = true;
		controls = core;		

	}				
	return sat;

}

// check formula for satisfiability using miniSAT
// the core and grow variables controls whether to return an unsat core or model extension, respectively
bool MSHandle::solve(vector<bool>& controls, bool unsat_improve, bool sat_improve){
	return solve(controls, vector<int>(), unsat_improve, sat_improve);
}

vector<bool> MSHandle::get_model(){
	vector<bool> model(vars, false);
	for(int i = 0; i < vars; i++){
		if(solver->model[i] == l_True)
			model[i] = true;
		else if(solver->model[i] == l_False)
			model[i] = false;
		else
			cout << "literal not evaluated " << i << endl;
	}
	return model;
}
