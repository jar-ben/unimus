#include "core/Master.h"
#include "core/misc.h"
#include <algorithm>
#include <math.h>
#include <functional>
#include <random>

void Master::manthan_base(){
	BooleanSolver *msSolver = static_cast<BooleanSolver*>(satSolver);

	vector<Formula> all_muses;
	Formula top(dimension, true);
	for(int i = 0; i < 1; i++){
		Formula mus = manthan_shrink(top);
		all_muses.push_back(mus);

		int maxC = -1;
		int maxV = 0;
		for(int c = 0; c < dimension; c++){
			if(!mus[c]) continue;
			int price = 0;
			for(auto l: msSolver->clauses[c]){
				int var = abs(l);
				if(find(msSolver->yVars.begin(), msSolver->yVars.end(),var) != msSolver->yVars.end())
					price += msSolver->yVarsPrice[var];
			}
			if(price > maxV){
				maxV = price;
				maxC = c;
			}		
		}
		cout << "MUS price: "<< manthan_price(mus) << ", checks: " << msSolver->checks << ", max price: " << maxV << endl;
		if(maxC == -1) break;
		top[maxC] = false;
		if(is_valid(top, false, false))
			break;
	}

	
	int min_price = 1000000;
	Formula mus;
	for(auto m: all_muses){
		auto price = manthan_price(m);
		if(price < min_price){
			min_price = price;
			mus = m;
		}
	}

	//export variables that appear in the MUS
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
	int price = 0;
	for(int c = 0; c < dimension; c++){
		if(!f[c]) continue;
		for(auto l: msSolver->clauses[c]){
			int var = (l > 0)? l: -l;
			if(find(msSolver->yVars.begin(), msSolver->yVars.end(),var) != msSolver->yVars.end()){
				price += msSolver->yVarsPrice[var];
			}
		}
	}
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

	vector<vector<int>> parentMap(*max_element(msSolver->yVars.begin(), msSolver->yVars.end()) + 1, vector<int>());

	for(int c = 0; c < dimension; c++){
		if(!top[c]) continue;
		auto item = make_pair(c,0);
		for(auto l: msSolver->clauses[c]){
			int var = abs(l);
			if(find(msSolver->yVars.begin(), msSolver->yVars.end(),var) != msSolver->yVars.end()){
				parentMap[var].push_back(c);
				value[c] += msSolver->yVarsPrice[var];
			}
		}
		if(value[c] == 0)
			base[c] = true;
		else
			pool[c] = true;	
	}

	Formula critical(dimension, false);

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
					value[c2] -= msSolver->yVarsPrice[var];
					if(value[c2] < 1) pool[c2] = false;
				}
			}
			print_values(seed, value);
		}else{
			cores.push_back(core);
			trim_core(c, core, seed, base, value, pool);
		}
		ones = count_ones(pool);
	}

	cout << "cores: " << cores.size() << endl;
	Formula finalCore;
	int min_price = 1000000;
	for(auto m: cores){
		auto price = manthan_price(m);
		if(price < min_price){
			min_price = price;
			finalCore = m;
		}
	}
	

	return finalCore;
}
