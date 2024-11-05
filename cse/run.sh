#!/bin/bash
 # g++ -I$CONDA_PREFIX/include -L$CONDA_PREFIX/lib cse.cpp -lsymengine -lflint -o cse && ./cse
 g++ cse.cpp -o cse && ./cse