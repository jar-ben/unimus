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
	if(ends_with(filename, "cnf")){
		if(sat_solver == "glucose"){
			cout << "using glucose" << endl;
			satSolver = new GlucoseHandle(filename);
		}else if(sat_solver == "cadical"){
			cout << "using cadical" << endl;
			satSolver = new CadicalHandle(filename);
		}else{
			cout << "using minisat" << endl;
			satSolver = new MSHandle(filename);
		}
		domain = "sat";
	}
	else
		print_err("The input file has to have one of these extensions: .cnf. See example files in ./examples/ folder.");
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
}

Master::~Master(){
	delete satSolver;
	delete explorer;
}

void Master::write_mus_to_file(MUS& f){
	satSolver->exportMUS(f.bool_mus, output_file);
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
	print_formula(f);
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

	if(output_file != "")
		write_mus_to_file(f);
}

void Master::enumerate(){
	initial_time = chrono::high_resolution_clock::now();
	cout << "running algorithm: " << algorithm << endl;
	Formula whole(dimension, true);
	if(is_valid(whole))
		print_err("the input instance is satisfiable");

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
	}
	return;
}

