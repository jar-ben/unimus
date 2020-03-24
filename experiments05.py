import glob
import subprocess as sp
import multiprocessing as mp
from math import ceil


###################################
##  basic experimental settings  ##
###################################

## Set the timeout (in seconds) for individual benchmarks. We used 3600 seconds (1 hour) in our experiments.
timeout = 1800 
timeout = 3600

## You can run the experiments in paralell on multiple cores if you want and if you can. We runned it on 2 cores while using a machine with 4 cores. 
cores = 32

## Choose a constraints domain (actually, you are choosing the benchmarks to be used). Available options are "sat" and "smt".
domain = "sat"


######################################
##  advanced experimental settings  ##
######################################

## These are paths to the benchmarks for individual domains that we used in the experimental evaluation in our paper. 
## If you want to use other benchmarks, modify the paths. 
filesDict = {}
filesDict["sat"] = '/home/xbendik/mustool/Formulae/all/*.cnf'
filesDict["smt"] = '../benchmarks/smt/intractable/*.smt2'
#filesDict["sat"] = '/home/xbendik/newmus/mustool/benchmakrs/selected_intractable/*.cnf'

## Choose an algorithm to used. Options are "remus", "marco", "tome". The first one, "remus", is what we have used in our experiments. 
algorithm = "unimus"

## If you want to process just part of the benchmarks, modify the variables ss and ee. There are 291 and 184 benchmarks in the SAT and SMT domains, respectively.
## ss defines the index of the first benchmark to be used, and ee the index of the last index of the benchmark to be used. 
## if you set ss to 0 and ee to 291 and you work with the SAT domain, all benchmarks will be processed. If you set ss to 50 and ee to 110, only 60 benchmarks in this range will be processed. 
## Therefore, set the range based on the constraint domain and based on the number of benchmarks that you want to evaluate.
ss = 0
ee = 292
#ee = 113 

## this is a partial name of files that will contain results of the experiments. 
## In particular, there will be produced two files. One of them contains detailed results including statistics about each identified MUS.
## The other file contains just a summary for each benchmark. 
## The whole names of the two files are created as:
## 	 "results_{}_t{}_v{}_{}.data".format(domain, timeout, algorithm, name) 
## 	 "sumup_results_{}_t{}_v{}_{}.data".format(domain, timeout, algorithm, name) 
## the files will be stored in this directory (./mustool_release/mustool)

name = ""


######################################
##  code that runs the experiments  ##
######################################

files = filesDict[domain]

def run(cmd, timeout):
    proc = sp.Popen([cmd], stdout=sp.PIPE, stderr=sp.PIPE, shell=True)
    try:
        (out, err) = proc.communicate(timeout = int(timeout * 1.1) + 1)
    except sp.TimeoutExpired:
        proc.kill()
        try:
            (out, err) = proc.communicate()
        except ValueError:
            out = "".encode("utf-8")
    return out


def sumup(filepath, target):
    result = ""
    with open(filepath, "r") as f:
        for line in f.readlines():
            if "Benchmark" in line:
                line = line.rstrip().split(";")
                result += ";".join(line[1:-1]) + "\n"

    with open(target, "w") as f:
        f.write("benchmark;identified MUSes\n")
        f.write(result)

def get_stats(out):
	out = out.decode("utf-8")
	bench_stats = []
	for line in out.splitlines():
		if line[:9] == "Found MUS":
			line = line.split(',')
			stats = {}
			for record in line[1:]:
				record = record.split(': ')
				if len(record) < 2:
					continue #invalid record
				record_name = record[0].strip()
				try:
					record_value = float(record[1])
				except ValueError:
					record_value = -2
				stats[record_name] = record_value
			bench_stats.append(stats)
	return bench_stats


def save_stats(stats, benchmark, stats_file, completed):
	with open(stats_file, "a") as f:
		muses = len(stats)
		f.write( "Benchmark;{};{};{}\n".format( benchmark, muses, "completed" if completed else "incompleted" ) )
		if muses > 0:
			#print the header
			keys = [ str(record_name) for  record_name in stats[0] ]
			f.write( ";".join(keys) + "\n" )
			
			#print stats for individual muses
			for mus in stats:
				values = [ (str(mus[key]) if key in mus else "-2")  for key in keys ]					
				f.write( ";".join( values ) + "\n" )

def work(range):
    print(range)
    start = range[0]
    end = range[1]

    #clear the stats_file
    stats_file = "results_{}_t{}_v{}_start{}_end{}_{}.csv".format(domain, timeout, algorithm, start, end, name)
    open(stats_file, "w").close()

    iteration = 0
    for file in sorted(glob.glob(files))[start:end]:
        iteration += 1
        print("({}, {}), iteration: {}".format(start, end, iteration))
        cmd = 'timeout {} ./must --grow cmp -a {} {}'.format(timeout, algorithm, file)
        print(cmd)
        out = run(cmd, timeout)        
        stats = get_stats(out)		
        completed = "Enumeration completed" in out.decode("utf-8")
        save_stats(stats, file, stats_file, completed)

import os
def process():
        tasks = glob.glob(files)
        n = len(tasks)
        n = min(n, ee - ss)
        ranges = []
        for i in range(cores):
            start = int(ceil(n / cores) * i + ss)
            end = int(ceil(n / cores)  * (i+1) + ss)
            ranges.append((start,end))	
        #run the cores
        pool = mp.Pool(processes = cores)
        pool.map(work, ranges)

        #merge the output files into one
        final_stats_file = "results_{}_t{}_v{}_{}.csv".format(domain, timeout, algorithm, name)
        with open(final_stats_file, "w") as f:
                for r in ranges:
                        start = r[0]
                        end = r[1]
                        stats_file = "results_{}_t{}_v{}_start{}_end{}_{}.csv".format(domain, timeout, algorithm, start, end, name)
                        with open(stats_file, "r") as f2:
                                f.write(f2.read())
                        os.remove(stats_file)
        final_sumup = "sumup_" + final_stats_file
        sumup(final_stats_file, final_sumup)

if __name__ == '__main__':
        process()


