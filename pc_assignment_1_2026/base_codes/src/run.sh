#!/bin/bash
mkdir -p output
echo "------------ Fractal is starting ---------------"
./fractal |& tee terminal.out
echo "------------- Fractal is done ------------------"
echo
echo "Results saved to terminal.out"
echo "Images saved to output/"
