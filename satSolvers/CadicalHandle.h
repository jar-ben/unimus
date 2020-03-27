#ifndef CADICALHANDLE_H
#define CADICALHANDLE_H

#include "BooleanSolver.h"
#include "cadical.hpp"
#include <string>
#include <vector>
#include <unordered_map>

class CadicalHandle: public BooleanSolver{
public:

	CaDiCaL::Solver* solver;
	CadicalHandle(std::string filename);
	~CadicalHandle();
	bool solve(std::vector<bool> &f, bool shrink = false, bool grow = false);

};

#endif
