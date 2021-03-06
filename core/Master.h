#ifndef REMUS_MASTER_H
#define REMUS_MASTER_H

#include "satSolvers/MSHandle.h"
#include "satSolvers/GlucoseHandle.h"
#include "satSolvers/CadicalHandle.h"
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
	int verbose;
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
	MUS& shrink_formula(Formula& f, Formula crits = Formula());
	void write_mus_to_file(MUS& f);
	void validate_mus(Formula &f);
	void validate_mss(Formula &f);
	void enumerate();

	int rotateMSS(Formula mss);

	//reMUS algorithm functions
	int depthMUS;
	float dim_reduction;
	int current_depth;
	void find_all_muses_duality_based_remus(Formula subset, Formula crits, int depth);
	void extend_mus(Formula &top, Formula &mus);

	//unimusRec algorithm functions
	int mssRotationLimit;
	int unimus_refines;
	int critical_extension_saves;
	std::stack<int> unimus_rotation_stack;
	int unimus_rotated;
	int unimus_attempts;
	void unimusRecMain();
	bool unimusRecRefine();
	void unimusRec(Formula subset, Formula crits, int depth);
	void unimusRec_add_block(MUS &m1, int mid, vector<vector<int>> &blocks, vector<vector<int>> &blocks_hitmap);
	void unimusRec_rotate_mus(int mid, Formula cover, Formula subset, vector<int> &localMuses);
	void unimusRec_mark_mus(MUS &mus, Formula cover, Formula subset);
	Formula unimusRec_propagateToUnsat(Formula base, Formula cover, vector<pair<int,int>> implied, 
		vector<vector<int>> &blocks, vector<vector<int>> &blocks_hitmap);
	Formula unimusRec_propagateRefine(Formula &conflict, Formula &base, vector<pair<int,int>> &implied);
	bool unimusRec_isAvailable(int c, Formula &subset, vector<vector<int>> &blocks, vector<vector<int>> &blocks_hitmap);
	Formula currentSubset;
	int unimusRecDepth;

	void critical_extension(Formula &f, Formula &crits);

	//TOME algorithm functions
	void find_all_muses_tome();
	pair<Formula, Formula> local_mus(Formula bot, Formula top, int diff);

	//MARCO algorithm functions
	void marco_base();
};
#endif
