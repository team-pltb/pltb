# PLTB: Phylogenetic likelihood (evaluator) and tree builder

This tool has been designed & developed as a student programming project
in the context of the module "Bioinformatics" at the Karlsruhe Institute of Technology (KIT).

Its main functionality is to find the best time-reversible substitution models
for a given dataset and to conduct a tree search using the chosen models afterwards.

For this, the maximum likelihood of every model under the given dataset is
evaluated and several information criteria are calculated.
The evaluation uses a random but fixed tree to apply model parameter optimizations,
leading to the estimation of the maximum likelihood value.

## Make

To build pltb you have to make sure that PLL [http://libpll.org/] is installed.
Furthermore an MPI implementation (either MPICH or OMPI) is required.
Afterwards you can call `make` to generate the pltb variants suitable for your system.

Here is a list of available make targets:
- `avx` pltb built against the avx version of PLL
- `avx-pthreads` pltb built against the avx version of PLL parallelized with pthreads
- `sse3` pltb built against the sse3 version of PLL
- `sse3-pthreads` pltb built against the sse3 version of PLL parallized with pthreads
- `debug` pltb build without optimizations, with debug symbols and against the avx version of PLL
- `debug-sse3` pltb build without optimizations, with debug symbols and against the sse3 version of PLL
- `clang` pltb built against the avx version of PLL; mainly used for syntax and semantic checks
- `default` implies target `avx`
- `clean` standard cleanup

Note that the first 6 targets use gcc with optimization level `O3`, C language standard `gnu99` and very restrictive compiler warnings enabled.
For the exact flags take a look at the respective Makefile.
All builds use the wrapper `mpicc` to accomplish actual compilation against your installed MPI implementation.

We further provide two additional Makefiles for static compilation against MPI and PLL.

## Usage

### Command-line interface:
- `-f/--data <datafile>`  *mandatory* argument with file path to dataset.
  - Supported formats: PHYLIP or FASTA
- `-b/--opt-freq` *optional* flag instructing PLL to use *optimized* base frequencies
- `-l/--lower-bound <index>` *optional* lower index bound for matrices to be checked. Index value will be *included*.
  - default = 0
- `-u/--upper-bound <index>` *optional* upper index bound for matrices to be checked. Index value will be *excluded*.
  - default = 203
- `-n/--npthreads <number>` *optional* number of threads used per process in model evaluation phase.
  - default = 1 (requires a pthreads PLL variant)
- `-s/--npthreads-tree <number>` *optional* number of threads used when conducting the tree search.
  - default = 1 (requires a pthreads PLL variant)
- `-r/--rseed <value>` *optional* random seed for model evaluation phase. Affects the starting tree on which model optimizations are applied.
  - default = 0x12345
- `-c/--config` *optional* flag instructing the program configuration to be printed before starting execution of the main program
- `-p/--progress` *optional* flag instructing the program to show a progress bar in model evaluation phase.
  - only activatable when using MPI parallelization
- `-g/--with-gtr` *optional* flag instructing the program to additionally conduct a tree search with the GTR-model

### MPI
The model evaluation phase comes with an MPI Master/Worker parallelization.
Running it with `mpirun -np <#processes> ./pltb.out args...` will lead to the execution with one master process and `#processes - 1` worker processes.
As the pthread parallelization uses thread-to-core-pinning it is recommended to choose `1 + (#processes - 1) * #npthreads` lower or equal the amount of cores available.

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

```SHELL
./eval/calculate_distances.py [--raxml <PATH_TO_YOUR_RAXML_BINARY>] ic-pairwise-distances eval/res/results/*/*.result
./eval/generate_histogram_plots.sh
```
