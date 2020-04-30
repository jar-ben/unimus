#include "Master.h"
#include "misc.h"
#include <algorithm>
#include <math.h>
#include <functional>
#include <random>


Master::Master(string filename, string alg, string ssolver){
        isValidExecutions = 0;
	algorithm = alg;
	sat_solver = ssolver;
	chrono::high_resolution_clock::time_point now = chrono::high_resolution_clock::now();

	if(ends_with(filename, "cnf")){
		cout << "solver: " << sat_solver << endl;
		if(sat_solver == "glucose"){
			satSolver = new GlucoseHandle(filename);
		}else if(sat_solver == "cadical"){
			satSolver = new CadicalHandle(filename);
		}else{
			satSolver = new MSHandle(filename);
		}
		domain = "sat";
	}
	else
		print_err("The input file has to have one of these extensions: .cnf. See example files in ./examples/ folder.");
	
	auto afterInit = chrono::high_resolution_clock::now();
	auto duration = chrono::duration_cast<chrono::microseconds>( afterInit - now ).count() / float(1000000);
	cout << "after satSolver init " << duration << endl;

	dimension = satSolver->dimension;	
	cout << "Number of constraints in the input set:" << dimension << endl;
        explorer = new Explorer(dimension);	
	//explorer->satSolver = satSolver;
        verbose = false;
	depthMUS = 0;
	dim_reduction = 0.5;
	output_file = "";
	validate_mus_c = false;
	current_depth = 0;
	unex_sat = unex_unsat = 0;
        hash = random_number();
	satSolver->hash = hash;
	bit = 0;
	guessed = 0;
	rotated_msses = 0;
	extended = 0;
	uni = Formula(dimension, false);
	couni = Formula(dimension, true);
	unimus_rotated = unimus_attempts = 0;
	critical_extension_saves = 0;
	unimus_refines = 0;
	satSolver->explorer = explorer;
	unex_unsat_time = unex_sat_time = total_shrink_time = 0;
	total_shrinks = 0;
	minimum_mus_value = 100000000;
}

Master::~Master(){
	delete satSolver;
	delete explorer;
}

void Master::write_mus_to_file(MUS& f){
	satSolver->exportMUS(f.bool_mus, output_file);
}

void Master::write_mss_to_file(MSS& f){
	satSolver->exportMSS(f.bool_mss, output_file);
}

// mark formula and all of its supersets as explored
void Master::block_up(Formula formula){
	explorer->block_up(formula);
}

// mark formula and all of its subsets as explored
void Master::block_down(Formula formula){
        explorer->block_down(formula);
}

// check formula for satisfiability
// core and grow controls optional extraction of unsat core and model extension (replaces formula)
bool Master::is_valid(Formula &formula, bool core, bool grow){
	chrono::high_resolution_clock::time_point start_time = chrono::high_resolution_clock::now();
	bool sat = satSolver->solve(formula, core, grow); 
	chrono::high_resolution_clock::time_point end_time = chrono::high_resolution_clock::now();
	auto duration = chrono::duration_cast<chrono::microseconds>( end_time - start_time ).count() / float(1000000);
	if(sat){ unex_sat++; unex_sat_time += duration; }
	else { unex_unsat++; unex_unsat_time += duration; }
	return sat;
}

//verify if f is a MUS
void Master::validate_mus(Formula &f){
	cout << "validationg MUS with size " << count_ones(f) << endl;
	if(is_valid(f))
		print_err("the mus is SAT");
	if(!explorer->isUnexplored(f))
		print_err("this mus has been already explored");
	for(int l = 0; l < f.size(); l++)
		if(f[l]){
			f[l] = false;
			if(!satSolver->solve(f, false, false))
				print_err("the mus has an unsat subset");	
			f[l] = true;
		}	
}

//verify if f is a MSS
void Master::validate_mss(Formula &f){
	if(!is_valid(f))
		print_err("the mss is UNSAT");
	if(!explorer->isUnexplored(f))
		print_err("this mss has been already explored");
	for(int l = 0; l < f.size(); l++)
		if(!f[l]){
			f[l] = true;
			if(satSolver->solve(f, false, false))
				print_err("the ms has a sat superset");	
			f[l] = false;
		}	
}

int Master::rotateMSS(Formula mss){
	int rots = 0;
	BooleanSolver *msSolver = static_cast<BooleanSolver*>(satSolver);
	for(int c = 0; c < dimension; c++){
		if(!mss[c]){
			msSolver->compute_flip_edges(c); //TODO: better encansulape
			for(auto &lit_group: msSolver->flip_edges[c]){
				vector<int> flip_c;
				for(auto c2: lit_group){
					if(mss[c2]) flip_c.push_back(c2);
				}
				if(flip_c.size() < 10){
					bool ok = true;
					Formula copy = mss;
					copy[c] = true;
					for(auto c2: flip_c){
						if(!explorer->is_critical(c2, copy)) { copy[c2] = false; }
						else{ ok = false; break; }
					}
					if(ok){
						if(satSolver->grow_alg == "fixpoint") grow_fixpoint(copy);
						else grow_combined(copy);
						mark_MSS_executive(MSS(copy, -1, msses.size(), count_ones(copy)));
						rotated_msses++;
						rots++;
					}
				}
			}	
		}
	}
	return rots;
}

void Master::critical_extension(Formula &f, Formula &crits){
	int initSize = count_ones(crits);
	queue<int> S;
	for(int i = 0; i < dimension; i++){
		if(crits[i]){
			S.push(i);
		}
	}
	
	BooleanSolver *msSolver = static_cast<BooleanSolver*>(satSolver);
	while(!S.empty()){
		int c = S.front();
		S.pop();
		for(auto l: msSolver->clauses[c]){
			vector<int> hits;
			int var = (l > 0)? l - 1 : (-l) - 1;
			if(var >= msSolver->vars) break; //these are the control variables
			if(l > 0){
				for(auto c2: msSolver->hitmap_neg[var]){
					if(f[c2])
						hits.push_back(c2);
				}
			}else{
				for(auto c2: msSolver->hitmap_pos[var]){
					if(f[c2])
						hits.push_back(c2);
				}
			}
			if(hits.size() == 1 && !crits[hits[0]]){
				S.push(hits[0]);	
				crits[hits[0]] = true;
			}
		}
	}
	int finalSize = count_ones(crits);
	critical_extension_saves += (finalSize - initSize);
}

MUS& Master::shrink_formula(Formula &f, Formula crits){
	int f_size = count_ones(f);
	chrono::high_resolution_clock::time_point start_time = chrono::high_resolution_clock::now();
	if(verbose) cout << "shrinking dimension: " << f_size << endl;
	f_size = count_ones(f);
	if(crits.empty()) crits = explorer->critical;
	if(get_implies){ //get the list of known critical constraints	
		explorer->getImplied(crits, f);
		//if(algorithm == "unimus")	
		if(verbose) cout << "# of known critical constraints before shrinking: " << count_ones(crits) << endl;	
		if(criticals_rotation && domain == "sat"){
			int before = count_ones(crits);
			BooleanSolver *msSolver = static_cast<BooleanSolver*>(satSolver);
			msSolver->criticals_rotation(crits, f);
			if(verbose) cout << "# of found critical constraints by criticals rotation: " << (count_ones(crits) - before) << endl;
		}

		BooleanSolver *bSolver = static_cast<BooleanSolver*>(satSolver);
		critical_extension_saves += bSolver->critical_extension(f, crits);
		float c_crits = count_ones(crits);
		if(int(c_crits) == f_size){ // each constraint in f is critical for f, i.e. it is a MUS 
			muses.push_back(MUS(f, -1, muses.size(), f_size)); //-1 duration means skipped shrink
			if(verbose) cout << "crits.size() = f.size()" << endl;
			return muses.back();
		}
		if((f_size - c_crits < 3) && !is_valid(crits, false, false)){		
		//if(!is_valid(crits, false, false)){		
			muses.push_back(MUS(crits, -1, muses.size(), f_size));//-1 duration means skipped shrink
			if(verbose) cout << "crits are unsat on themself" << endl;
			return muses.back();
		}
	}
	if(verbose) cout << "calling satSolver->shrink with " << count_ones(crits) << " crits" << endl;
	
	if(DBG){
		if(is_valid(f)) print_err("shrink_formula: the seed is valid");
		cout << "crits: " << count_ones(crits) << endl;
		for(int c = 0; c < dimension; c++){
			if(crits[c]){
				f[c] = false;
				if(!is_valid(f)) print_err("shrink_formula: spurious crit");
				f[c] = true;
			}
		}
	}	
	
	Formula mus = satSolver->shrink(f, crits);
	chrono::high_resolution_clock::time_point end_time = chrono::high_resolution_clock::now();
	auto duration = chrono::duration_cast<chrono::microseconds>( end_time - start_time ).count() / float(1000000);
	muses.push_back(MUS(mus, duration, muses.size(), f_size));
	if(verbose) cout << "shrunk via satSolver->shrink" << endl;
		
	total_shrinks++;
	total_shrink_time += duration;
	
	//If the average time of performing sat. checks with result "satisfiable" is relatively low compared 
	//to the average time of shrinking, we attempt to collect singleton MCSes from the mus_intersection
	float avg_shrink = total_shrink_time / total_shrinks;
	float avg_sat_check = (unex_sat == 0)? 10000000 : (unex_sat_time/unex_sat);
	//cout << "avg_shrink " << avg_shrink << ", avg sat: " << avg_sat_check << endl;
	//cout << "ratio: " << (avg_shrink / avg_sat_check) << endl;
	bool cheap_sat_checks = (avg_shrink / avg_sat_check) > 5;
	if(muses.size() > 2 && cheap_sat_checks ){
		int limit = count_ones(explorer->mus_intersection) / 10;	
		//cout << "limit: " << limit << endl;
		for(int i = 0; i < dimension; i++){
			if(explorer->mus_intersection[i] && !explorer->testedForCriticality[i] && !explorer->critical[i] && mus[i]){
				Formula seed (dimension, true);
				seed[i] = false;
				explorer->testedForCriticality[i] = true;
				if(is_valid(seed, false, false)){
					explorer->block_down(seed);
				}
				if(limit-- < 0) break;
			}
		}
	}
		
	return muses.back();
}


vector<MSS> Master::grow_formulas(Formula &f, Formula conflicts, int limit){
	chrono::high_resolution_clock::time_point start_time = chrono::high_resolution_clock::now();
	if(conflicts.empty()) conflicts.resize(dimension, false);
	explorer->getConflicts(conflicts, f);	
	int f_size = count_ones(f);
	float c_conflicts = count_ones(conflicts);
	if(int(c_conflicts) == (dimension - f_size)){ // each constraint in f is critical for f, i.e. it is a MUS 
		guessed++;
		msses.push_back(MSS(f, -1, muses.size(), f_size)); //-1 duration means skipped shrink
		return vector<MSS> (1, msses.back());
	}		
	int diff = (dimension - f_size) - c_conflicts;
	float ratio = c_conflicts / (dimension - f_size);	
	if(ratio > 0.95 || diff < 3){
		Formula extended = f;
		for(int i = 0; i < dimension; i++) if(!conflicts[i]) extended[i] = true;
		if(is_valid(extended, true, false)){ 
			msses.push_back(MSS(extended, -1, msses.size(), f_size));//-1 duration means skipped shrink
			guessed++;
			return vector<MSS> (1, msses.back());
		}else{
			block_up(extended);
		}
	}
	
	vector<Formula> resMSSes = satSolver->growMultiple(f, conflicts, limit);
	chrono::high_resolution_clock::time_point end_time = chrono::high_resolution_clock::now();
	auto duration = chrono::duration_cast<chrono::microseconds>( end_time - start_time ).count() / float(1000000);
	duration = duration / resMSSes.size();
	vector<MSS> result;
	for(auto &m: resMSSes){		
		result.push_back(MSS(m, duration, msses.size(), f_size));
	}
	return result;
}

//grow formula into a MSS
MSS Master::grow_formula(Formula &f, Formula conflicts){
	if(satSolver->grow_alg == "fixpoint" && count_zeros(f) < 100){
	 	Formula mss = f;
	 	grow_fixpoint(mss);
		return MSS(mss, -1, msses.size(), -1);
	}
	int f_size = count_ones(f);
	chrono::high_resolution_clock::time_point start_time = chrono::high_resolution_clock::now();
	if(verbose) cout << "growing dimension: " << f_size << endl;
	if(conflicts.empty())
		conflicts.resize(dimension, false);
	if(get_implies){ //get the list of known critical constraints	
		explorer->getConflicts(conflicts, f);	
		float c_conflicts = count_ones(conflicts);
		if(verbose) cout << "# of known conflicting constraints before growing: " << int(c_conflicts) << endl;	
		if(int(c_conflicts) == (dimension - f_size)){ // each constraint in f is critical for f, i.e. it is a MUS 
			return MSS(f, -1, muses.size(), f_size);
		}		
		int diff = (dimension - f_size) - c_conflicts;
		float ratio = c_conflicts / (dimension - f_size);	
		if(ratio > 0.95 || diff < 3){
			Formula extended = f;
		 	for(int i = 0; i < dimension; i++)
				if(!conflicts[i]) extended[i] = true;
			if(is_valid(extended, true, false)){ 
				return MSS(extended, -1, msses.size(), f_size);
			}else{
				block_up(extended);
			}
		}
	}

	Formula mss;
	if(satSolver->grow_alg == "combined"){
		grow_combined(f, conflicts);
		mss = f;
	}else if(satSolver->grow_alg == "fixpoint"){
		satSolver->grow_alg = "cmp";
		mss = satSolver->grow(f, conflicts);
		satSolver->grow_alg = "fixpoint";
	
	}else{
		mss = satSolver->grow(f, conflicts);
	}
	chrono::high_resolution_clock::time_point end_time = chrono::high_resolution_clock::now();
	auto duration = chrono::duration_cast<chrono::microseconds>( end_time - start_time ).count() / float(1000000);
	return MSS(mss, duration, msses.size(), f_size);
}

//grow formula into a MSS
void Master::grow_fixpoint(Formula &f){
	if(verbose) cout << "growing dimension: " << count_ones(f) << endl;
	Formula conflicts(dimension, false);
	explorer->getConflicts(conflicts, f);
	int f_size = count_ones(f);	
	int c_conflicts = count_ones(conflicts);
	if(c_conflicts == (dimension - f_size)){ 
		return;
	}		

	while(true){
		int g = 0;
		for(int c = 0; c < dimension; c++){
			if(conflicts[c])
				g += grow_hitting_extension(f, c);
		}
		if(g == 0) break;
		explorer->getConflicts(conflicts, f);
	}
	if(verbose) cout << "grown by: " << (count_ones(f) - f_size) << ", new conflicts: " << (count_ones(conflicts) - c_conflicts) << endl;
	
	for(int i = 0; i < dimension; i++){
		if(!f[i] && !conflicts[i] && explorer->is_available(i, f)){
			if(verbose) cout << "iteration: " << i << endl;
			f[i] = true;
			Formula copyMss = f;
			bool sat = satSolver->solve(copyMss, true, true);
			if(!sat){
				f[i] = false;
				block_up(copyMss);
				if(grow_hitting_extension(f, i) > 0){
					while(true){
						Formula prevConflicts = conflicts;
						explorer->getConflicts(conflicts, f);
						int g = 0;
						for(int i = 0; i < dimension; i++){
							if(conflicts[i] && !prevConflicts[i])
								g += grow_hitting_extension(f, i);
						}
						if(g == 0) break;
					}
				}
			}else{
				f = copyMss;
			}
		}
	}
	if(verbose) cout << "end of grow_fixpoint" << endl;
}


int Master::grow_hitting_extension(Formula &mss, int c1){
	BooleanSolver *msSolver = static_cast<BooleanSolver*>(satSolver);
	vector<int> co;
	int m = 0;
	for(int i = 0; i < dimension; i++)
		if(!mss[i] && i != c1 && is_hitting_pair(msSolver->clauses[c1], msSolver->clauses[i])){ 
			mss[i] = true;
			m++;
		}
	extended += m;
	return m;
}

void Master::grow_combined(Formula &f, Formula conflicts){
	if(conflicts.empty())
		conflicts.resize(dimension, false);
	vector<int> cnfs;
	//if(conflicts_negation && count_ones(conflicts) == 0)
		explorer->getConflicts(conflicts, f);
	for(int i = 0; i < dimension; i++){
		if(conflicts[i]) 
			cnfs.push_back(i);
	}
	int f_size = count_ones(f);
	for(int c = 0; c < dimension; c++){
		if(conflicts[c])
			grow_hitting_extension(f, c);
	}
	extended += count_ones(f) - f_size;

	satSolver->grows++;
	Formula mss = f;
	for(int i = 0; i < dimension; i++){
		if(!mss[i] && !conflicts[i] && explorer->is_available(i, mss)){
			mss[i] = true;
			Formula copyMss = mss;
			bool sat = (conflicts_negation)? satSolver->solve(copyMss, cnfs, true, true) : satSolver->solve(copyMss, true, true);
			if(!sat){
				cnfs.push_back(i);
				mss[i] = false;
				block_up(copyMss);
				grow_hitting_extension(mss, i);
			}else{
				mss = copyMss;
			}
		}
	}
	f = mss;
}

void Master::mark_MSS_executive(MSS f, bool block_unex){	
	Formula mcs = complement(f.bool_mss);
	uni = union_sets(uni, mcs);
	couni = complement(uni);
	msses.push_back(f);
	if(validate_mus_c) validate_mss(f.bool_mss);		
	explorer->block_down(f.bool_mss);
	chrono::high_resolution_clock::time_point now = chrono::high_resolution_clock::now();
	auto duration = chrono::duration_cast<chrono::microseconds>( now - initial_time ).count() / float(1000000);
        bool printMSSes = false;
	if(printMSSes){
		cout << "Found MSS #" << msses.size() <<  ", mss dimension: " << f.dimension;
		cout << ", checks: " << satSolver->checks << ", time: " << duration;
		cout << ", unex sat: " << unex_sat << ", unex unsat: " << unex_unsat << ", criticals: " << explorer->criticals;
		cout << ", intersection: " << count_ones(explorer->mus_intersection);
		cout << ", union: " << count_ones(uni) << ", dimension: " << dimension;
		cout << ", seed dimension: " << f.seed_dimension << ", grow duration: " << f.duration;
		cout << ", grows: " << satSolver->grows << ", depth: " << current_depth;
		cout << ", sats: " << explorer->mcses.size() << ", unsats: " << explorer->muses.size() << ", bit: " << bit << ", guessed: " << guessed;
		cout << ", exp calls: " << explorer->calls << ", rotated msses: " << rotated_msses << ", extended: " << extended;
		cout << endl;
	}

	if(output_file != "" && !minimum_mus)
		write_mss_to_file(f);
	if(mss_rotation)
		rotation_queue.push_back(f.bool_mss);
}

void Master::mark_MSS(Formula f, bool block_unex){
	MSS mss (f, -1, msses.size(), -1);
	mark_MSS(mss, block_unex);
}

void Master::mark_MSS(MSS f, bool block_unex){
	mark_MSS_executive(f, block_unex);
	int i = 0;
	while(!rotation_queue.empty()){
		Formula mss = rotation_queue.back();
		rotation_queue.pop_back();
		rotateMSS(mss);
		if(++i > mssRotationLimit){
			rotation_queue.clear();
			return;
		}
	}
}

void Master::mark_MUS(MUS& f, bool block_unex){	
	uni = union_sets(uni, f.bool_mus);
	couni = complement(uni);
	if(validate_mus_c) validate_mus(f.bool_mus);		
	explorer->block_up(f.bool_mus);
	//explorer->block_down(f.bool_mus);

	chrono::high_resolution_clock::time_point now = chrono::high_resolution_clock::now();
	auto duration = chrono::duration_cast<chrono::microseconds>( now - initial_time ).count() / float(1000000);
        if(algorithm == "unibase2") return;
	cout << "Found MUS #" << muses.size() << ", msses: " << msses.size() <<  ", mus dimension: " << f.dimension;
	cout << ", checks: " << satSolver->checks << ", time: " << duration;
	cout << ", unex sat: " << unex_sat << ", unex unsat: " << unex_unsat << ", criticals: " << explorer->criticals;
	cout << ", intersections: " << std::count(explorer->mus_intersection.begin(), explorer->mus_intersection.end(), true);
	cout << ", union: " << count_ones(uni) << ", dimension: " << dimension;
	cout << ", seed dimension: " << f.seed_dimension << ", shrink duration: " << f.duration;
	cout << ", shrinks: " << satSolver->shrinks << ", unimus rotated: " << unimus_rotated << ", unimus attempts: " << unimus_attempts;
	cout << ", bit: " << bit;
	cout << ", stack size: " << unimus_rotation_stack.size();
	
	cout << ", critical_extension_saves: " << critical_extension_saves;
	cout << ", unimus_refines: " << unimus_refines;
	cout << ", mcsmus saves: " << satSolver->shrinkMinedCrits;
	//cout << ", unimusRecDepth: " << unimusRecDepth;
	cout << endl;

	if(output_file != ""){
		if(true || minimum_mus){
			BooleanSolver *msSolver = static_cast<BooleanSolver*>(satSolver);
			set<int> yVars;
			set<int> xVars;
			set<int> allVars;
			//collect the variables that appear in the MUS
			for(auto c: f.int_mus){
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

			//compute the value of the MUS
			int value = 0;
			for(auto v: yVars)
				value += (10 * msSolver->yVarsDependents[v]) + msSolver->yVarsDependsOn[v];

			//minimize value
			if(value < minimum_mus_value){
				minimum_mus_value = value;
				ofstream file;
				file.open(output_file);	
				for(auto var: yVars)
					file << var << endl;
				for(auto var: xVars)
					file << var << endl;
				file.close();
			}
		}else{
			write_mus_to_file(f);
		}
	}
	//if(muses.size() > 100) exit(0);
}

void Master::enumerate(){
	cout << "running algorithm: " << algorithm << endl;

	if(algorithm == "remus"){
		find_all_muses_duality_based_remus(Formula (dimension, true), Formula (dimension, false), 0);
	}
	else if(algorithm == "tome"){
		find_all_muses_tome();
	}
	else if(algorithm == "marco"){
		marco_base();
	}
	else if(algorithm == "unimus"){
		unimusRecMain();
	}else if(algorithm == "manthan"){
		manthan_base();
	}
	return;
}

