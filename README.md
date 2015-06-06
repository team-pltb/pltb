# PLTB: Phylogenetic likelihood (evaluator) and tree builder

This tool has been designed & developed as a studentical practical project
in the context of the module bioinformatics at KIT.

It's main functionality is to find the best time-reversible substitution models
for a given dataset and to conduct a tree search using the chosen models afterwards.

For this, the maximum likelihood of every model under the given dataset is
evaluated and several information criteria are calculated.
The evaluation uses a random but fixed tree to apply model parameter optimizations,
leading to the estimation of the maximum likelihood value.

## Make

To build pltb you have to make sure that PLL [http://libpll.org/] is installed.
Furthermore a MPI implementation (either MPICH or OMPI) is required.
Afterwards you can call `make` to generate the pltb variants suitable for your system.

Here is a list of available make targets:
- `avx`: pltb built against the avx version of PLL
- `avx-pthreads`: pltb built against the avx version of PLL parallelized with pthreads
- `sse3`: pltb built against the sse3 version of PLL
- `sse3-pthreads`: pltb built against the sse3 version of PLL parallized with pthreads
- `debug`: pltb build without optimizations, with debug symbols and against the avx version of PLL
- `debug-sse3`: pltb build without optimizations, with debug symbols and against the sse3 version of PLL
- `clang`: pltb built against the avx version of PLL; mainly used for syntax and semantic checks
- `default`: implies target `avx`
- `clean`: standard cleanup

Note, that the first 6 targets use gcc with optimization level `O3`, C language standard gnu99 and very restrictive compiler warnings enabled.
All builds use mpicc to accomplish the actual compilation against your installed MPI implementation.

## Usage

### Command-line interface:
- `-f/--data <datafile>;`:  *mandatory* argument with filepath to dataset. Supported formats: PHYLIP or FASTA
- `-b/--opt-freq`: *optional* flag instructing pll to use *optimized* base frequencies
- `-l/--lower-bound <index>;`: *optional* lower index bound for matrices to be checked. Index value will be *included*. default = 0
- `-u/--upper-bound <index>;`: *optional* upper index bound for matrices to be checked. Index value will be *excluded*. default = 203
- `-n/--npthreads <number>;`: *optional* number of threads used per process in model evaluation phase. Default = 1
- `-s/--npthreads-tree <number>;`: *optional* number of threads used when conducting the tree search. Default = 1
- `-r/--rseed <value>;`: *optional* random seed for model evaluation phase. Affects the starting tree on which model optimizations are applied. Default = 0x12345
- `-c/--config`: *optional* flag instructing the program configuration to be printed before starting execution of the main program
- `-p/--progress`: *optional* flag instructing the program to show a progress bar in model evaluation phase. Only effective in (MPI) parallel version
- `-g/--with-gtr`: *optional* flag instructing the program to additionally conduct a tree search with the GTR-model

### MPI
The model evaluation phase comes with a MPI Master/Worker parallelization.
Running it with `mpirun -np <#processes> ./pltb args...` will lead to the execution with one master process and (#processes - 1) worker processes.
As the pthread parallelization uses thread-to-core-pinning it is recommend to choose ((#processes - 1) * #npthreads + 1) lower or equal the amount of cores to use.
