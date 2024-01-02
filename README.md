# MimoSketch: A Framework for Frequency-based Mining Tasks on Multiple Nodes with Sketches



Source codes of MimoSketch: An unbiased and accurate sketch with scheduling policy in distributed data stream mining, and four baselines: WavingSketch, Count Sketch, DHS, Cuckoo Counter, including single-node version and MIMO version.

## Descriptions



### *single-node-version*
**mimosketch/wavingsketch/countsketch/dhs/cuckoocounter.cpp**: single-node versions of the above five algorithms used for performance comparision.

**demo.dat**: a demo dataset which can be used for reproducibility (refer to our paper for detailed datasets).

Each algorithm can be built and run by the following commands: 

```
g++ -std=c++11 *.cpp -O3 -o *
./*
```

### *MIMO-version*
**mimosketch/wavingsketch/countsketch/dhs/cuckoocounter.h**: source codes of the above five algorithms used in distributed scenario experiments.

**func.h**: hash functions used in MIMO-scenario experiments.

**main.cpp**: evaluation in a prototype of the MIMO scenario --
1. comparing the above five algorithms.
2. comparing the optimizing scheduling policy with three baselines: randomly selecting just one node/selecting all the nodes/randomly selecting the same number of nodes as that of scheduling policy.

**demo.dat**: a demo dataset which can be used for reproducibility (refer to our paper for detailed datasets).

Distributed evaluation can be built and run by the following commands: 

```
g++ -std=c++11 main.cpp -O3 -m64 -g -L (gurobi_lib_PATH) -lgurobi_c++ -lgurobi95 -lm -o main
./main
```

##
***WavingSketch/DHS/CuckooCounter***: these directories are original codes open-sourced by their authors. 


