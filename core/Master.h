#ifndef REMUS_MASTER_H
#define REMUS_MASTER_H

#include "satSolvers/MSHandle.h"
#include "Explorer.h"
#include "types_h.h"
#include <set>
#include <list>
#include <string>
#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <utility>
#include <ctime>
#include <chrono>	
#include <unordered_map>
#include <queue>
#include <stack>

using namespace std;

class Master{
public:
	int bit;
	int dimension; //the number of constraints
	string algorithm; //MUS enumeration algorithm to be used
	int isValidExecutions;
	bool verbose; //TODO: make it int based 
	string output_file;
	bool validate_mus_c;
	float mus_approx;
	bool get_implies;
	bool criticals_rotation;
	bool mss_rotation;
	string domain;
	bool useMixedHeuristic;
	int hash;
	int unex_unsat;
	int unex_sat;
	float unex_unsat_time;
	float unex_sat_time;
	float total_shrink_time;
	int total_shrinks;
	int minimum_mus_value;
	bool minimum_mus;
	int target_mus_score;

	int guessed;
	int rotated_msses;
	chrono::high_resolution_clock::time_point initial_time;
	vector<MUS> muses;
	vector<MSS> msses;
	Explorer* explorer;
	SatSolver* satSolver; 
	string sat_solver;
	std::vector<Formula> rotation_queue;
	bool conflicts_negation;
	int extended; //number of saved satisfiability checks via the grow extension
	Formula uni;
	Formula couni;
	bool DBG;

	Master(string filename, string alg, string ssolver);
	~Master();
	bool is_valid(Formula &f, bool core = false, bool grow = false);
	void block_down(Formula f);
	void block_up(Formula f);
	void mark_MUS(MUS& m, bool block = true);
	void mark_MSS(MSS m, bool block = true);
	void mark_MSS(Formula m, bool block = true);
	void mark_MSS_executive(MSS m, bool block = true);
	MUS& shrink_formula(Formula& f, Formula crits = Formula());
	MSS grow_formula(Formula& f, Formula conflicts = Formula());
	vector<MSS> grow_formulas(Formula& f, Formula conflicts = Formula(), int limit = 1);
	void grow_combined(Formula &f, Formula conflicts = Formula());
	int grow_hitting_extension(Formula &mss, int c1);
	void grow_fixpoint(Formula &f);
	void write_mus_to_file(MUS& f);
	void write_mss_to_file(MSS& f);
	void validate_mus(Formula &f);
	void validate_mss(Formula &f);
	void enumerate();

	int rotateMSS(Formula mss);

	void critical_extension(Formula &f, Formula &crits);


	void manthan_base();
	Formula manthan_shrink(Formula top);
	int manthan_price(Formula f);
};
#endif
