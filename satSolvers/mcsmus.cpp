#ifdef UMCSMUS

#include <errno.h>
#include <zlib.h>
#include <iostream>
#include <fstream>
#include <string>
#include "satSolvers/BooleanSolver.h"
#include "mcsmus/minisat-wrapper.hh"
#include "mcsmus/glucose-wrapper.hh"
#include "mcsmus/minisolver.hh"
#include "mcsmus/parseutils.hh"
#include "mcsmus/options.hh"
#include "mcsmus/mcsmus.hh"
#include "mcsmus/dimacs.hh"
#include "mcsmus/system.hh"


using namespace mcsmus;

std::vector<Lit> intToLitG(std::vector<int> cls){
	std::vector<Lit> lits;
	//we are taking the clasue from the MSHandle class, and each clause contain additional control literal
	//here we have to exclude the control literal
	//TODO: we should not store the control literals in MSHandle.clauses, get rid of them
	for(int i = 0; i < cls.size(); i++){
		auto l = cls[i];
		int var = abs(l)-1;
		lits.push_back( mkLit(var, l<0) );
	}
	return lits;
}

std::vector<Lit> intToLit(std::vector<int> cls){
	std::vector<Lit> lits;
	//we are taking the clasue from the MSHandle class, and each clause contain additional control literal
	//here we have to exclude the control literal
	//TODO: we should not store the control literals in MSHandle.clauses, get rid of them
	for(int i = 0; i < cls.size() - 1; i++){
		auto l = cls[i];
		int var = abs(l)-1;
		lits.push_back( mkLit(var, l<0) );
	}
	return lits;
}

std::vector<bool> BooleanSolver::shrink_mcsmus(std::vector<bool> &f, std::vector<bool> crits){
    std::vector<std::vector<bool>> models;
    return shrink_mcsmus(f, models, crits);
}

std::vector<bool> BooleanSolver::shrink_mcsmus(std::vector<bool> &f, std::vector<std::vector<bool>> &models, std::vector<bool> crits){
	setX86FPUPrecision();
	Wcnf wcnf;
	std::unique_ptr<BaseSolver> s;
	s = make_glucose_simp_solver();
	MUSSolver mussolver(wcnf, s.get());
	
	Control* control = getGlobalControl();;
	control->verbosity = 0;
	control->status_interval = 10;
	mussolver.single_mus = true;
	mussolver.initializeControl();
	control->force_exit_on_interrupt();

	//add the clauses
	std::vector<int> constraintGroupMap;
	std::vector<int> indexOfClause (f.size(), -1);
	int cnt = 0;
	int counter = 0;
	for(auto &cl: hard_clauses){
		wcnf.addClause(intToLitG(cl), 0);
	}
	for(int i = 0; i < f.size(); i++){
		if(f[i]){
			if(crits[i]){
				wcnf.addClause(intToLit(clauses[i]), 0);
			}else{
				indexOfClause[i] = counter++;
				constraintGroupMap.push_back(i);
				cnt++;
				wcnf.addClause(intToLit(clauses[i]), cnt);
			}
		}
	}

    //possibly memorize models identified by mcsmus (in Master, we then build model extensions and block them from unexplored)
    mussolver.mcsmus_store_models = shrinkStoreModels;

	//add the blocks from Explorer
	if(shrinkMining){
		mussolver.conflictMining = true;
		for(auto &mcs: explorer->mcses){
			vector<int> trimmed_mcs;
			bool forFree = false;
			for(auto c: mcs){
				if(f[c] && !crits[c]){
					trimmed_mcs.push_back(indexOfClause[c]);
				}
				if(crits[c]){
					forFree = true;
					break;
				}
			}
			if(!forFree)
				mussolver.addMinableBlockDown(trimmed_mcs);
		}
	}
	
	std::vector<Lit> mus_lits;
	wcnf.relax();
	mussolver.find_mus(mus_lits, false);
	
	shrinkMinedCrits += mussolver.minedCriticals;

    for(auto m: mussolver.mcsmus_models){
        vector<bool> m2;
        for(int i = 0; i < m.size(); i++){
            m2.push_back(m[i] == l_True);
        }
        models.push_back(m2);
    }
    std::cout << "mcsmus models: " << models.size() << std::endl;

	std::vector<bool> mus(f.size(), false);
	for(auto b : mus_lits){
		int gid = wcnf.bvarIdx(b);
		if(gid > 0){
			mus[constraintGroupMap[gid - 1]] = true;
		}	
	}
	for(int i = 0; i < f.size(); i++){
		if(crits[i]){
			mus[i] = true;
		}
	}
	return mus;
}
#endif
