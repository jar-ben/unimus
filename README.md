# UNIMUS
UNIMUS is a  tool for online enumeration of minimal unsatisfiable subsets (MUSes) of a given unsatisfiable Boolean CNF formula in the dimacs format. The tool implements several MUS enumeration algorithms including UNIMUS (called same as the tool), MARCO [9], TOME [2], and ReMUS [3]. 

We distribute this source code under the MIT licence. See ./LICENSE for mode details.

## Installation
Before building UNIMUS, you have to first get the zlib library and cadical. 

### Zlib
To install zlib, use the following:
```
sudo apt install zlib1g-dev
```

### Cadical
Clone and build cadical from https://github.com/arminbiere/cadical (it does not matter where you put it). 
Subsequently, copy the file libcadical.a that is located in the cadical/build/ directory into the main directory of UNIMUS (the one with this README.md file). 
Finally, open the Makefile of UNIMUS (in this directory) and modify the variables CADICAL_SRC and CADICAL_BUILD so that they point to the source and build directories of cadical (in depence on when you have put cadical). The two variables are defined at the the beginning of the Makefile. 

### UNIMUS
To build UNIMUS, run in the main directory (the one where is this README.md file):
```
make
```

## Running our tool
In the main directory, run "./unimus _file_" where _file_ is an input file of constraints. You can use one of our examples, e.g.:
```
./unimus examples/test.cnf
./unimus examples/bf1355-228.cnf
```
To run the tool with a time limit (always recommended), use e.g.:
```
timeout 20 ./unimus examples/bf1355-228.cnf
```
To save the identified MUSes into a file, run:
```
timeout 10 ./unimus -o output_file examples/test.cnf
```
To see all the available parameters, run:
```
./unimus -h
```

## Other Third-Party Tools
Besides mniSAT, we also use two single MUS extraction algorithms: muser2 [5] and mcsmus [8]. You do not have to install these. Muser2 is presented in our repo in a binary form, and mcsmus is compiled with our code. 

## Related Tools
The tool UNIMUS originates from our another MUS numeration tool called MUST (https://github.com/jar-ben/mustool). Contrary to UNIMUS, MUST is a domain agnostic tool which means that it can be used in various constraint domains such as SAT, SMT, or LTL. If you want to enumerate MUSes in the SAT domain, we recommend to use UNIMUS. If you are interested in other domain than SAT, use MUST.

## References

* [1] Jaroslav Bendík and Ivana Černá: MUST: Minimal Unsatisfiable Subsets Enumeration Tool. TACAS 2020.
* [2] Jaroslav Bendík, Nikola Beneš, Ivana Černá, Jiří Barnat: Tunable Online MUS/MSS Enumeration. FSTTCS 2016.
* [3] Jaroslav Bendík, Ivana Černá, Nikola Beneš: Recursive Online Enumeration of All Minimal Unsatisfiable Subsets. ATVA 2018.
* [4] http://minisat.se/
* [5] https://bitbucket.org/anton_belov/muser2
* [8] https://bitbucket.org/gkatsi/mcsmus
* [9] Mark H. Liffiton, Alessandro Previti, Ammar Malik, João Marques-Silva: Fast, flexible MUS enumeration. Constraints 21(2), 2016.

## Citation
If you use UNIMUS in your research, please cite the paper that presented UNIMUS: 
```
@inproceedings{DBLP:conf/cp/BendikC20,
  author    = {Jaroslav Bend{\'{\i}}k and
               Ivana Cern{\'{a}}},
  title     = {Replication-Guided Enumeration of Minimal Unsatisfiable Subsets},
  booktitle = {{CP}},
  series    = {Lecture Notes in Computer Science},
  volume    = {12333},
  pages     = {37--54},
  publisher = {Springer},
  year      = {2020}
}
```

## Contact
In case of any troubles, do not hesitate to contact me, Jaroslav Bendik, the developer of the tool, at xbendik=at=fi.muni.cz.
