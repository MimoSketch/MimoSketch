# MimoSketch: A Framework for Frequency-Based Mining Tasks on Multiple Nodes With Sketches



Source codes of MimoSketch: An unbiased and accurate sketch with scheduling policy in distributed data stream mining, and six baselines: WavingSketch, Count Sketch, DHS, Cuckoo Counter, Count-Min Sketch, and BitMatcher, including single-node version and MIMO version.

## Descriptions



### *single-node-version*
**mimosketch/countsketch/cmsketch.cpp**: single-node versions of the corresponding algorithms used for performance comparision.

**demo.dat**: a demo dataset which can be used for reproducibility (refer to our paper for detailed datasets).

Each algorithm can be built and run by the following commands: 

```
g++ -std=c++11 *.cpp -O3 -o *
./*
```

### *MIMO-version*
**mimosketch/countsketch/cmsketch.h**: source codes of the corresponding algorithms used in distributed scenario experiments.

**func.h**: hash functions used in MIMO-scenario experiments.

**main.cpp**: evaluation in a prototype of the MIMO scenario --
1. comparing the above algorithms.
2. comparing the optimizing scheduling policy with three baselines: randomly selecting just one node/selecting all the nodes/randomly selecting the same number of nodes as that of scheduling policy.

**demo.dat**: a demo dataset which can be used for reproducibility (refer to our paper for detailed datasets).

Distributed evaluation can be built and run by the following commands: 

```
g++ -std=c++11 main.cpp -O3 -m64 -g -L (gurobi_lib_PATH) -lgurobi_c++ -lgurobi95 -lm -o main
./main
```

##
***WavingSketch/DHS/CuckooCounter/BitMatcher***: these directories offer open-sourced codes released by their authors. 


