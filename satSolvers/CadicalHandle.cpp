#include "CadicalHandle.h"
#include "core/misc.h"
#include <sstream>
#include <iostream>

using namespace std;


CadicalHandle::CadicalHandle(string filename):BooleanSolver(filename){
        solver = new CaDiCaL::Solver;
        parse(filename);

	//add soft clauses to the solver;
	for(auto &clause: clauses){
                for(auto l: clause){
			solver->add(l);
		}
		solver->add(0);
	}
	//add hard clauses to the solver;
	for(auto &clause: hard_clauses){
                for(auto l: clause){
			solver->add(l);
		}
		solver->add(0);
	}
}


CadicalHandle::~CadicalHandle(){
}

bool CadicalHandle::solve(std::vector<bool> &f, bool shrink, bool grow){
	checks++;
	for(int i = 0; i < dimension; i++){
		if(!f[i]){
//			solver->assume(vars + i + 1);
		}
		else{
			solver->assume((vars + i + 1) * -1);
		}
	}
	bool sat = solver->solve() == 10;
	if(sat && grow){
		int initSize = count_ones(f);
		for(int i = 0; i < f.size(); i++){
                        if(!f[i]){
                                for(int l = 0; l < clauses[i].size() - 1; l++){
	  				if(clauses[i][l] > 0){
						if(solver->val(clauses[i][l]) > 0){                  				
		  					f[i] = true;
                                                        break;
                                                }
                                        }
                                        else{
						if(solver->val(-1 * clauses[i][l]) < 0){
							f[i] = true;
                                                        break;
                                                }
                                        }
                                }
                        }
                }
		int finalSize = count_ones(f);
	}
	int coreSize = 0;
	int fSize = 0;
	if(!sat && shrink){
		vector<bool> unsatCore(dimension, false);
		for(int i = 0; i < dimension; i++){
			if(f[i]){
				fSize++;
				if(solver->failed((vars + i + 1) * -1)){
					unsatCore[i] = true;
					coreSize++;
				}
			}
		}
		f = unsatCore;
	}
	return sat;
}
