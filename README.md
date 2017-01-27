# PLTB: phylogenetic likelihood (evaluator) & tree builder

This tool has been designed & developed as a student programming project
in the context of the module [bioinformatics](http://sco.h-its.org/exelixis/web/teaching/BioinformaticsModule.html)
at Karlsruhe Institute of Technology (KIT).
See the corresponding publication (["Does the choice of nucleotide substitution models matter topologically?"](http://bmcbioinformatics.biomedcentral.com/articles/10.1186/s12859-016-0985-x)) for further theoretical background.

Its main functionality is to find the best time-reversible substitution models
for a given dataset and to conduct a tree search using the chosen models afterwards.

For this, the maximum likelihood of every model under the given dataset is
evaluated and several information criteria (AIC, AICc, BIC) are calculated.
The evaluation uses a random but fixed tree to apply model parameter optimizations,
leading to the estimation of the maximum likelihood value.
For each information criterium, the optimal model over the 203 distinct models possible is determined.

## Install

To build pltb you have to make sure that [PLL](http://libpll.org/) is installed.
Furthermore an MPI implementation (either [MPICH](https://www.mpich.org/)
or [OpenMPI](https://www.open-mpi.org/)) is required.

### PLL

Assuming you do not wish to install PLL globally, consider the following short
snippet for creating a local installation of this library. Otherwise installation
comes down to `./configure && make all install` in the downloaded source folder.

```bash
mkdir pll
cd pll

# prepare local installation directory
mkdir install
PLL_INSTALL_DIR=`readlink -f install`
export CPATH=$CPATH:$PLL_INSTALL_DIR/include
export LIBRARY_PATH=$LIBRARY_PATH:$PLL_INSTALL_DIR/lib
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$PLL_INSTALL_DIR/lib

# get sources & install
wget http://libpll.org/Downloads/libpll-1.0.11.tar.gz
tar -xzf libpll-1.0.11.tar.gz
rm -Rf libpll-1.0.11.tar.gz
cd libpll-1.0.11
./configure --prefix=$PLL_INSTALL_DIR
make all install
```

### PLTB

For compilation, pltb only relies on a simple Makefile.

```bash
git clone https://github.com/team-pltb/pltb
cd pltb
make sse3 # creates pltb.out
```

#### Alternative make targets

- `avx` pltb built against the AVX version of PLL
- `avx-pthreads` pltb built against the AVX version of PLL parallelized with pthreads
- `sse3` pltb built against the SSE3 version of PLL
- `sse3-pthreads` pltb built against the SSE3 version of PLL parallized with pthreads
- `debug` pltb build without optimizations, with debug symbols and against the AVX version of PLL
- `debug-sse3` pltb build without optimizations, with debug symbols and against the SSE3 version of PLL
- `clang` pltb built against the AVX version of PLL; mainly used for syntactical and semantic checks
- `default` implies target `avx`
- `clean` standard cleanup

Note that the first 6 targets use gcc with optimization level `O3`, C language standard `gnu99` and very restrictive compiler warnings enabled.
For the exact flags take a look at the respective Makefile.

We further provide two additional Makefiles for static compilation against MPI and PLL.

## Usage

### Command-line interface

- `-f/--data <datafile>`  *mandatory* argument with file path to dataset (supported formats: PHYLIP or FASTA).
- `-b/--opt-freq` *optional* flag instructing PLL to use *optimized* base frequencies
- `-l/--lower-bound <index>` *optional* lower index bound for matrices to be checked. Index value will be *included*. (default = 0)
- `-u/--upper-bound <index>` *optional* upper index bound for matrices to be checked. Index value will be *excluded*. (default = 203)
- `-n/--npthreads <number>` *optional* number of threads used per process in model evaluation phase. (default = 1, pll-pthread required)
- `-s/--npthreads-tree <number>` *optional* number of threads used when conducting the tree search. (default = 1, pll-pthread required)
- `-r/--rseed <value>` *optional* random seed for model evaluation phase.
    Affects the starting tree on which model optimizations are applied. (default = 0x12345)
- `-c/--config` *optional* flag instructing the program configuration to be printed before starting execution of the main program
- `-p/--progress` *optional* flag instructing the program to show a progress bar in model evaluation phase. (requires MPI)
- `-g/--with-gtr` *optional* flag instructing the program to additionally conduct a tree search with the GTR-model

### Number of processes

The model evaluation phase comes with an MPI Master/Worker parallelization.
Running it with `mpirun -np <#processes> ./pltb.out args...`
will lead to the execution with one master process and `#processes - 1` worker processes.
As the pthread parallelization uses thread-to-core-pinning it is recommended to choose
`1 + (#processes - 1) * #npthreads` lower or equal the amount of cores available.

### Examples

Sequential processing of a dataset: `./pltb.out -f eval/res/datasets/lakner/027.phy`

```
-----------------------------------------------------------------------------------------------------------
   Model    |   Time [seconds]    |            |         I N F O R M A T I O N   C R I T E R I A
------------|---------------------| Max Log_e  |-----------------------------------------------------------
 Symm.  | K |   CPU    |  REAL    | Likelihood |  AIC      |  AICc-S   |  AICc-M   |  BIC-S    |  BIC-M
-----------------------------------------------------------------------------------------------------------
 000000 | 1 |    1.713 |    1.713 | -6588.2622 | 13280.524 | 13283.431 | 13280.629 | 13570.428 | 13741.812
 011111 | 2 |    1.544 |    1.544 | -6565.9016 | 13237.803 | 13240.824 | 13237.912 | 13533.282 | 13707.961
 010000 | 2 |    1.493 |    1.493 | -6586.6801 |  13279.36 | 13282.381 | 13279.469 | 13574.839 | 13749.518
 001000 | 2 |    1.493 |    1.493 | -6573.9321 | 13253.864 | 13256.885 | 13253.973 | 13549.343 | 13724.022
 [...                      generated line by line for all 203 symmetries                              ...]
 010234 | 5 |    2.852 |    2.853 | -6494.0466 | 13100.093 | 13103.467 | 13100.215 | 13412.297 | 13596.864
 001234 | 5 |    2.324 |    2.325 | -6497.0103 | 13106.021 | 13109.395 | 13106.142 | 13418.225 | 13602.791
 012345 | 6 |    2.320 |    2.321 | -6494.0462 | 13102.092 | 13105.589 | 13102.218 | 13419.871 | 13607.734
-----------------------------------------------------------------------------------------------------------
 Overview   |    317.0 |    317.1 |            | -> 010231 | -> 010231 | -> 010231 | -> 010231 | -> 000120
-----------------------------------------------------------------------------------------------------------
Tree search for best model(s)
# Model 010231 [newick] (AIC, AICc-S, AICc-M, BIC-S)
((2:0.00311516336819118176,(3:0.00975914895046898351,4:0.00399634810341891779):0.00342903915862048441):0.00475077117746713472,((27:0.02965217409617993544,(((20:0.02093478886616213439,21:0.01837197653679669557):0.00530856808243705680,(19:0.03153536954743572929,((22:0.02880956902588326443,23:0.02132645575132489923):0.00810484888629311312,(24:0.00809711785409484956,(15:0.01203104862151248462,(((11:0.00774541162459284165,(16:0.00551292038273126783,18:0.00907006883134331247):0.00473970110338340148):0.00569060429540000148,26:0.01662960759117076276):0.00314859756306718180,(14:0.01614026771604324881,((17:0.02891661158011149535,13:0.00000100000050002909):0.00296742829279573713,12:0.00469558734261936363):0.00665501228953481461):0.00656321107197094938):0.00474124492611427070):0.00150863413709100480):0.00894806821840240206):0.00313985345576722882):0.00694950645453319173):0.01542370893430455685,25:0.00351932062350289801):0.01294580452849471673):0.01766369674188553868,((5:0.00518606446723057259,6:0.01863683154620894913):0.02573161257121951706,((7:0.00254354613780962855,10:0.00744432245074153325):0.00324789129565538877,(9:0.01104571129723988000,8:0.00871519292809130874):0.00588216905247512366):0.00442465175830901794):0.01415350220353700679):0.03396510621183392031,1:0.00790720791899947838):0.0;
# Model 000120 [newick] (BIC-M)
((2:0.00317151528679812406,(3:0.00976747060355894793,4:0.00399305341116552512):0.00337445592786745447):0.00462563812257898825,((27:0.02955205705143536601,(((20:0.02081868043026207163,21:0.01827094843055333470):0.00531383731197832086,(19:0.03117109807002870631,((22:0.02851623100882911752,23:0.02164733127833819853):0.00808494395426412674,(24:0.00805635933853166356,(15:0.01197081833301927127,(((11:0.00769551267230825665,(16:0.00548297947929705931,18:0.00902418212253820566):0.00472498793789467419):0.00570147478035960050,26:0.01654790619016574191):0.00310442616463051926,(14:0.01603099455092662010,((17:0.02878328564843573092,13:0.00000100000050002909):0.00293395166033230030,12:0.00465602034000958242):0.00663996767235839388):0.00652208660943536574):0.00474665222902513270):0.00148319126995154508):0.00877687764364245421):0.00310879252558341704):0.00694051357246787230):0.01548991356577333195,25:0.00356463940021096385):0.01291416553517581688):0.01756800592496963673,((5:0.00506353628183923554,6:0.01864972619069475354):0.02542486591023484024,((7:0.00256720559378415588,10:0.00738678672852248735):0.00324074381585620153,(9:0.01096436788139128551,8:0.00868039515288153429):0.00585902840021318514):0.00453995073544391416):0.01407387492853648912):0.03388555523381287654,1:0.00802348079286598417):0.0;
```

Each line of the table represents the results for a single model tested under the given dataset.
Details like the amount of time taken for evaluation, the max log likelihood as "raw result" of
the process and the "scores" determined via different information criteria are presented to the user.
For each information criteria, the model with the best score is then selected in the last row.
Afterwards, tree searches are conducted for these models and the tree is printed in newick format.

Same task, but highly parallelized: `mpirun -np 16 ./pltb.out -f eval/res/datasets/lakner/027.phy -s 16`
While the sequential evaluation (inclusive tree search) took 389 seconds,
the parallel version reduced this time to 55 seconds.

## Evaluation

The subfolder `eval` contains evaluation scripts, datasets, precomputed results and other evaluation-specific files.

### Datasets

The datasets reside in `eval/res/datasets/*/*`.
The first wildcard represents the dataset source and the second the dataset name (usually the size is also encoded within the name).

We differentiate between 5 sources:
* Empirical datasets used for testing Bayesian inference programs
  * lakner
  * mrbayes
* Datasets from the <a href=http://mbe.oxfordjournals.org/content/21/6/1123.abstract>original paper</a> by John Huelsenbeck
  * originalPaper
* Test datasets from <a href=http://exelixis-lab.org>Exelixis Lab</a> designated for the programming practical
  * testDatasets
* Other not further categorized datasets
  * otherDatasets

### Precomputed Results

We evaluated each of our datasets several times with different configurations using the `eval/pltb_evaluate_dataset_folder.sh` script.
18 different random seeds 0x12345, 0x54321, 0x00000 ... 0xFFFFF, as well as 2 kinds of base frequencies (empirical and optimized) lead to 36 results per dataset file.

The results reside in `eval/res/results/*/*.result`, whereas the first wildcard represents the dataset source.
The second wildcard, the filename, conforms to the following naming pattern: `DATAFILE-RSEED[-opt].result`, where
* `DATAFILE` is the name of the dataset file,
* `RSEED` is the random seed used to initialize the PLL instance,
* `-opt` implies optimized base frequencies, otherwise empirical base frequencies are assumed.

### Scripts

In the folder `eval` you can find the scripts described in the following.
Note that the working directory has to be the root of this repository and _not_ the `eval` folder.
* `eval/pltb_evaluate_dataset_folder.sh` is used for processing whole dataset folders with pltb.
Every file in the given source folder is evaluated n times, where n equals the number of random seeds times 2 (empirical and optimized base frequencies).
The results are written to the given destination folder, using the above naming pattern.
For our evaluation we used four hex seeds (see above).
Note that you have to adapt the three variables `processes`, `threads_per_process` and `threads_for_search` to your
hardware capabilities.
* `eval/calculate_distances.py` is used for analyzing pltb results with RAxML (RF-distances).
The scripts' main function is to extract the trees from a pltb result and feed them into RAxML to retrieve the pairwise RF-distances.
The RAxML binary can be supplied with the optional command line argument `--raxml`.
If not supplied, the binary `raxmlHPC-SSE3` is assumed to be in the path.
Further processing can be triggered by optional command line arguments.
  * `ic-pairwise-distances` gathers the distances over all given pltb results grouped by information criteria pairs.
  For example, the pair AIC/BIC would yield a list of RF-distances between the trees generated by their respective models.
  These difference lists are then written to the directory `eval/res/histograms/data` using the naming pattern `IC1-IC2`.
  Note that the term `extra` stands for the GTR model.
* `eval/generate_histogram_plots.sh` uses the difference lists in `eval/res/histograms/data` to generate respective histograms in `eval/res/histograms/plots` formatted & controlled by the gnuplot file `eval/rf_histogram.plot`.
Note that this script requires the previous script to have written the difference lists first.

### Histograms

To generate the histogram plots using the precomputed results execute the following commands:

```bash
./eval/calculate_distances.py [--raxml <PATH_TO_YOUR_RAXML_BINARY>] ic-pairwise-distances eval/res/results/*/*.result
./eval/generate_histogram_plots.sh
```
