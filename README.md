# HW1 Sokoban

[Slide](https://docs.google.com/presentation/d/1d7GsL0MOCHbiqQcBkbF1e8pVE5osJHiIf_jW5tnBmpI/edit#slide=id.g1173758fe74_0_1948)

[SPEC](https://hackmd.io/@ipc22/hw1)

## Compile
`g++ -std=c++17 -O3 -pthread -fopenmp hw1.cc -o hw1`

## Execute

`./hw1 /path/to/testcase`

`srun -n1 -c6 ./hw1 /home/ipc22/share/hw1/samples/01.txt`

`srun -c6 ./hw1 01.txt | /home/ipc22/share/hw1/validate.py 01.txt -`