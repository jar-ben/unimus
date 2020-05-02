#include "core/Master.h"
#include "core/misc.h"
#include <algorithm>
#include <math.h>
#include <functional>
#include <random>

void Master::manthan_base(){
	Formula top(dimension, true); 
	Formula mus = manthan_shrink(top);

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
	return;
}

int Master::manthan_price(Formula f){
	BooleanSolver *msSolver = static_cast<BooleanSolver*>(satSolver);
	set<int> Yvars;
	set<int> Xvars;
	for(int c = 0; c < dimension; c++){
		if(!f[c]) continue;
		for(auto l: msSolver->clauses[c]){
			int var = (l > 0)? l: -l;
			if(find(msSolver->yVars.begin(), msSolver->yVars.end(),var) != msSolver->yVars.end()){
				Yvars.insert(var);
			}
			if(find(msSolver->xVars.begin(), msSolver->xVars.end(),var) != msSolver->xVars.end()){
				Xvars.insert(var);
			}
		}
	}
	int price = 0;
	for(auto var: Yvars)
		price += msSolver->yVarsPrice[var];
	for(auto var: Xvars)
		price += msSolver->xVarsPrice[var];
	return price;
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
Formula Master::manthan_shrink(Formula top){
	vector<Formula> cores;
	Formula core = top;
	if(is_valid(core, true, false)) print_err("top is valid");
	cores.push_back(core);
	if(count_ones(core) == count_ones(top)) return core;

	BooleanSolver *msSolver = static_cast<BooleanSolver*>(satSolver);
	Formula seed = top;
	Formula base(dimension, false);
	Formula pool(dimension, false);
	vector<int> value(dimension, 0);
	Formula critical(dimension, false);
	vector<vector<int>> parentMap(*max_element(msSolver->yVars.begin(), msSolver->yVars.end()) + 1, vector<int>());

	for(int c = 0; c < dimension; c++){
		if(!top[c]) continue;
		auto item = make_pair(c,0);
		for(auto l: msSolver->clauses[c]){
			int var = abs(l);
			parentMap[var].push_back(c);
			if(find(msSolver->yVars.begin(), msSolver->yVars.end(),var) != msSolver->yVars.end()){
				value[c] += msSolver->yVarsPrice[var];
			}
			if(find(msSolver->xVars.begin(), msSolver->xVars.end(),var) != msSolver->xVars.end()){
				value[c] += msSolver->xVarsPrice[var];
			}
		}
		if(value[c] == 0)
			base[c] = true;
		else
			pool[c] = true;	
	}


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
					if(find(msSolver->yVars.begin(), msSolver->yVars.end(),var) != msSolver->yVars.end()){
						value[c2] -= msSolver->yVarsPrice[var];
					}
					if(find(msSolver->xVars.begin(), msSolver->xVars.end(),var) != msSolver->xVars.end()){
						value[c2] -= msSolver->xVarsPrice[var];
					}
					if(value[c2] < 1) pool[c2] = false;
				}
			}
		}else{
			cores.push_back(core);
			trim_core(c, core, seed, base, value, pool);
		}
		ones = count_ones(pool);
	}

	cout << "cores: " << cores.size() << endl;
	Formula finalCore;
	int min_price = 1000000000;
	for(auto m: cores){
		auto price = manthan_price(m);
		if(price < min_price){
			min_price = price;
			finalCore = m;
		}
	}
	

	return finalCore;
}
