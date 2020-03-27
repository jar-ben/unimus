#ifndef BOOLEANSOLVER_H
#define BOOLEANSOLVER_H

#include "satSolvers/SatSolver.h"
#include <string>
#include <map>
#include <vector>
#include <unordered_map>

class BooleanSolver: public SatSolver{
public:
	BooleanSolver(std::string filename);
    	~BooleanSolver();
	std::vector<std::vector<int>> clauses;
	int vars;
	std::vector<std::string> clauses_str;
	std::unordered_map<std::string, int> clauses_unique_map;
	std::map<std::vector<int>,int> clauses_map;

	bool parse(std::string filename);
	void add_clause(std::vector<int> cl);
	std::string toString(std::vector<bool> &f);

        //model rotation
        int rotated_crits;
        std::vector<std::vector<int>> hitmap_pos;
        std::vector<std::vector<int>> hitmap_neg;
        std::vector<bool> flip_edges_computed;
        std::vector<std::vector<std::vector<int>>> flip_edges; // flip_edges[i][j][k] - i-th clause, j-th literal in the clause, k-th edge from i-th clause under j-th literal in the flip grap
        std::vector<std::vector<int>> flip_edges_flatten;
        void compute_flip_edges(int c);
	void criticals_rotation(std::vector<bool>& criticals, std::vector<bool> subset);
	std::vector<int> get_implied(std::vector<bool> mus, int c);
	int critical_extension(std::vector<bool> &f, std::vector<bool> &crits);
	std::vector<int> critical_extension_clause(std::vector<bool> &f, std::vector<bool> &crits, int c);
	int critical_propagation(std::vector<bool> &f, std::vector<bool> &crits, int cl);
	std::vector<bool> propagateToUnsat(std::vector<bool> base, std::vector<bool> cover, std::vector<int> implied);
	
	std::vector<bool> shrink(std::vector<bool> &f, std::vector<bool> crits);
	std::vector<bool> shrink_mcsmus(std::vector<bool> &f, std::vector<bool> crits = std::vector<bool>());
	bool lit_occurences(std::vector<bool> subset, int c2);
	std::vector<bool> shrink(std::vector<bool> &f, Explorer *e, std::vector<bool> crits);
	std::vector<int> export_formula_wcnf(std::vector<bool> f, std::vector<bool> &conflicts, std::string filename);
	std::vector<bool> grow_uwrmaxsat(std::vector<bool> &f, std::vector<bool> &conflicts);
	std::vector<bool> grow_cmp(std::vector<bool> &f, std::vector<bool> &conflicts);
	std::vector<bool> grow(std::vector<bool> &f, std::vector<bool> conflicts);
	std::vector<bool> shrink_muser(std::string input, int hash2);
	int muser_output(std::string filename);
	std::vector<bool> import_formula_crits(std::string filename);
	void export_formula_crits(std::vector<bool> f, std::string filename, std::vector<bool> crits);
	std::vector<bool> satisfied(std::vector<int> &valuation);

	std::vector<int> convert_clause(std::string clause);
};
#endif
