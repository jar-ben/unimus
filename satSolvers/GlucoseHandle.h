#ifndef GLHANDLE_H
#define GLHANDLE_H

#include "glucose/Solver.h"
#include "glucose/SimpSolver.h"
#include "satSolvers/BooleanSolver.h"
#include <string>
#include <map>
#include <vector>
#include <unordered_map>


class GlucoseHandle: public BooleanSolver{
public:
	//CustomGlucose::Solver* solver;
	CustomGlucose::SimpSolver* solver;

	GlucoseHandle(std::string filename);
	~GlucoseHandle();
	bool solve(std::vector<bool> &f, bool shrink = false, bool grow = false);

	//model rotation
	std::vector<bool> get_model();
	std::vector<bool> model_extension(std::vector<bool> subset, std::vector<bool> model);
};


#endif
