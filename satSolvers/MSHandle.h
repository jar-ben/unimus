#ifndef MSHANDLE_H
#define MSHANDLE_H

#include "custom_minisat/Solver.h"
#include "satSolvers/BooleanSolver.h"
#include <string>
#include <map>
#include <vector>
#include <unordered_map>


class MSHandle: public BooleanSolver{
public:
	CustomMinisat::Solver* solver;

	MSHandle(std::string filename);
	~MSHandle();
	bool solve(std::vector<bool> &f, bool shrink = false, bool grow = false);
	bool solve(std::vector<bool> &f, std::vector<int> conflicts, bool shrink = false, bool grow = false);

	//model rotation
	std::vector<bool> get_model();
	std::vector<bool> model_extension(std::vector<bool> subset, std::vector<bool> model);
};


#endif
