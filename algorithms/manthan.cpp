#include "core/Master.h"
#include "core/misc.h"
#include <algorithm>
#include <math.h>
#include <functional>
#include <random>

void Master::manthan_base(){
	Formula mus = manthan_shrink();

	//export variables that appear in the MUS
	BooleanSolver *msSolver = static_cast<BooleanSolver*>(satSolver);
	set<int> yVars;
	set<int> xVars;
	set<int> allVars;
	for(int c = 0; c < dimension; c++){
		if(!mus[c]) continue;
		for(auto l: msSolver->clauses[c]){
			int var = (l > 0)? l: -l;
			allVars.insert(var);
			if(msSolver->xVars.find(var) != msSolver->xVars.end())
				xVars.insert(var);
			if(find(msSolver->yVars.begin(), msSolver->yVars.end(),var) != msSolver->yVars.end()){
				yVars.insert(var);
			}
		}
	}
	ofstream file;
	file.open(output_file);
	for(auto var: allVars)
		file << var << endl;
/*	for(auto var: yVars)
		file << var << endl;
	for(auto var: xVars)
		file << var << endl;
*/
	file.close();
	cout << "price: " << manthan_price(mus) << endl;
	return;
}

int Master::manthan_price(Formula f){
	BooleanSolver *msSolver = static_cast<BooleanSolver*>(satSolver);
	set<int> yVars;
	set<int> xVars;
	set<int> allVars;
	//collect the variables that appear in the MUS
	for(int c = 0; c < dimension; c++){
		if(!f[c]) continue;
		for(auto l: msSolver->clauses[c]){
			int var = (l > 0)? l: -l;
			if(msSolver->xVars.find(var) != msSolver->xVars.end())
				xVars.insert(var);
			if(find(msSolver->yVars.begin(), msSolver->yVars.end(),var) != msSolver->yVars.end()){
				yVars.insert(var);
			}
		}
	}

	//compute the value of the MUS
	int price = xVars.size();
	for(auto v: yVars)
		price += (10 * msSolver->yVarsDependents[v]) + msSolver->yVarsDependsOn[v];	
	return price;
}

void print_values(vector<bool> &seed, vector<int> &values){
	return;
	vector<int> value_copy;
	for(int i = 0; i < seed.size(); i ++){
		if(seed[i]) value_copy.push_back(values[i]);
	}
	sort(value_copy.begin(), value_copy.end());
	for(auto v: value_copy)
		if(v > 0) cout << v << " ";
	cout << endl << endl;
}

void trim_core2(int c, Formula &core, Formula &seed, Formula &base, vector<int> &value, Formula &pool){
	bool useCore = true;
	for(int i = 0; i < core.size(); i++){
		if(core[i] && i != c && (value[i] * 2.2) > value[c]){
			useCore = false;
			break;
		}
	}
	//if(useCore) seed = core;
	for(int i = 0; i < core.size(); i++){
		if(!seed[i]) pool[i] = false;
		if(base[i]) seed[i] = true;
	}
}

void trim_core(int c, Formula &core, Formula &seed, Formula &base, vector<int> &value, Formula &pool){
	vector<pair<int,int>> pair_values;

	for(int i = 0; i < core.size(); i++){
		if(seed[i])
			pair_values.push_back(make_pair(value[i],i));
	}
	sort(pair_values.rbegin(), pair_values.rend());
	for(auto e: pair_values){
		auto d = e.second;
		if(!seed[d] || !pool[d]) continue;
		//cout << "E:" << e.first << endl;
		if(core[d]) break;
		seed[d] = pool[d] = false;		
	}
}

// preferences driven shrinking
Formula Master::manthan_shrink(){
	BooleanSolver *msSolver = static_cast<BooleanSolver*>(satSolver);
	Formula seed(dimension, true);
	Formula base(dimension, false);
	Formula pool(dimension, false);
	vector<int> value(dimension, 0);

	vector<vector<int>> parentMap(*max_element(msSolver->yVars.begin(), msSolver->yVars.end()) + 1, vector<int>());

	for(int c = 0; c < dimension; c++){
		auto item = make_pair(c,0);
		for(auto l: msSolver->clauses[c]){
			int var = abs(l);
			if(find(msSolver->yVars.begin(), msSolver->yVars.end(),var) != msSolver->yVars.end()){
				parentMap[var].push_back(c);
				value[c] += (10 * msSolver->yVarsDependents[var]) + msSolver->yVarsDependsOn[var];
			}
		}
		if(value[c] == 0)
			base[c] = true;
		else
			pool[c] = true;	
	}

	Formula critical(dimension, false);
	cout << "base: " << count_ones(base) << ", pool: " << count_ones(pool) << endl;

	print_values(seed, value);

	int ones = count_ones(pool);
	while(ones > 0){
		int c = -1;
		int cVal = 0;
		for(int i = 0; i < dimension; i++){
			if(pool[i] && value[i] >= cVal){
				c = i;
				cVal = value[i];
			}
		}
		pool[c] = false;
		seed[c] = false;		
		Formula core = seed;
		if(msSolver->solve(core, true, false)){
			seed[c] = true;
			critical[c] = true;

			//update costs of the other clauses (we know that variables of c will be in the MUS)
			for(auto l: msSolver->clauses[c]){
				auto var = abs(l);
				if(var > msSolver->vars) break; //skip the activation variable
				if(find(msSolver->yVars.begin(), msSolver->yVars.end(),var) == msSolver->yVars.end()) continue;
				for(auto c2: parentMap[var]){
					value[c2] -= (10 * msSolver->yVarsDependents[var]) + msSolver->yVarsDependsOn[var];
					if(value[c2] < 1) pool[c2] = false;
				}
			}
			print_values(seed, value);
		}else{
			trim_core(c, core, seed, base, value, pool);
		}
		ones = count_ones(pool);
	}

	//just trim the hard clauses from the output
	is_valid(seed, true, false);
	return seed;
}
