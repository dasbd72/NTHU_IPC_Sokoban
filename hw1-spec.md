# HW1: Sokoban

Due: <span style="color:red;">Sat, 2022/3/19 23:59</span>

[toc]

# Hint
For BFS algorithm
1. [Intel TBB](https://oneapi-src.github.io/oneTBB/) provide thread-safe concurrent access containers. You may find them useful for parallelizing your code. They are installed on apollo31. To use them, include the correspond header files and compile with `-ltbb`.
    > Note: In the oneAPI document, it use `#include "oneapi/tbb/xxx.h` to inlclude TBB container. On our server, replace it with `#include <tbb/xxx.h>`.
1. `boost::functional::hash` can be used to hash more STL containers.
https://www.techiedelight.com/use-std-pair-key-std-unordered_map-cpp/
1. (easy) Try to avoid/detect dead states, e.g.:
    ```
    ######       ######
    # .###       # .###
    #  ###       #  ###
    #X   #       #X   #
    #xo  #       #  ox#
    #  ###       #  ###
    ######       ######
    ```
1. (medium-hard) Reduce the number of states by combining person move only states.
1. (hard) Use an efficient state representation to reduce memory usage

# Problem Description

> "Sokoban (倉庫番, sōko-ban, "warehouse keeper") is a puzzle video game in which the player pushes crates or boxes around in a warehouse, trying to get them to storage locations." -- [Wikipedia](https://en.wikipedia.org/wiki/Sokoban)

You are asked to implement a solver for Sokoban and parallelize it with threads
(either pthread or OpenMP; you can use `std::thread` as a pthread wrapper).

## Input Format

Input is given in an ASCII text file. The filename is given from the command line
in `argv[1]`.

Every line ends with a newline character `\n`.
Each line represents a row of tiles; every character in the line represents a tile
in the game.

The tiles are listed as follows:

* `o`: The player stepping on a regular tile.
* `O`: The player stepping on a target tile.
* `x`: A box on a regular tile.
* `X`: A box on a target tile.
* ` ` (space): Nothing on a regular tile.
* `.`: Nothing on a target tile.
* `#`: Wall.
* `@`: A fragile tile where only the player can step on. The boxes are not allowed to be put on it.
* `!`: The player stepping on a fragile tile.

Your program only need to deal with valid inputs. It is guranteed that:

* There is only one player, stepping on either a regular tile, a target tile or a fragile tile.
* The number of target tiles are equal to the number of boxes.
* All tiles other than wall tiles are within an enclosed area formed by wall tiles.
* There is at least one solution.
* The size of the map is less than <span style="color:red;">256 pixels</span>

## Output Format

You program should output a sequence of actions that pushes all the boxes to
the target tiles, to `stdout`. (Do not output anything else to `stdout`, otherwise
your output may be considered invalid. For debugging purposes, please output to
`stderr`. Please also remove the debug output from your
submission as they may harm performance)

The output sequence should end with a newline character `\n`, and contain
only uppercase `WASD` characters:

* `W`: move the player up
* `A`: move the player left
* `S`: move the player down
* `D`: move the player right

Your solution is not required to have the least number of moves.
Any sequence that solves the problem is valid.

## Input Example
1. This is a valid input:

   ~~~
   #########
   #  xox..#
   #   #####
   #########
   ~~~


2. This input is invalid because there are tiles outside of the wall-enclosed area:

   ~~~
   #########
   #  xox..#
   #   #####
   #####   #
   ~~~


3. This input is invalid because there are fewer target tiles than the boxes:

   ~~~
   #########
   #  xox .#
   #   #####
   #########
   ~~~

4. This input is invalid because there exists no solution:

   ~~~
   #########
   # ox x..#
   #   #####
   #########
   ~~~

## Output Example

Consider the following problem:

~~~
#########
#  xox..#
#   #####
#########
~~~

A valid output is:

~~~
DDAAASAAWDDDD
~~~

Another valid output is:

~~~
DDAAADASAAWDDDD
~~~

<span style="color: red;">Although the second solution takes more steps then the first one, both output are considered correct and will be accepted.</span>

# Execution

Your code will be compiled with the following command:

~~~
g++ -std=c++17 -O3 -pthread -fopenmp hw1.cc -o hw1
~~~

We will use `make` to build your code. The default `Makefile` for this homework is provided at `/home/ipc22/share/hw1/Makefile`.
If you wish to change the compilation flags, please include `Makefile` in your submission.

To build your code by `make`, make sure that both the Makefile and your hw1.cc are in the working directory. Running the `make` command line will build `hw1`
for you. To remove the built files, please execute the `make clean` command.

Your code will be executed with a command equalviant to:

~~~
srun -n1 -c${threads} ./hw1 ${input}
~~~

where:

* `${threads}` is the number of threads
* `${input}` is the path to the input file

The time limit for each input test case is 30 seconds. `${threads}=6` for all test cases.

# Report

Answer the following questions, in either English or Traditional Chinese.

1. Briefly describe your implementation.

2. What are the difficulties encountered in this homework? How did you solve them?
   (You can discuss about hard-to-optimize hotspots, or synchronization problems)

3. What are the strengths and weaknesses of pthread and OpenMP?

4. (Optional) Any suggestions or feedback for the homework are welcome.

# Submission

Please upload the following files to EEClass:

* `hw1.cc` -- the source code of your implementation.
* `report.pdf` -- your report.
* `Makefile` -- optional. Submit this file if you want to change the build command.
* `ipc22<uid>.txt`: optional. A <span style="color:red;">valid</span> map to be added to hidden test cases. The size of the map should less than <span style="color:red;">256 pixels</span>.

Please follow the naming listed above carefully. Failing to adhere to the names
above will result to points deduction. Here are a few bad examples: `hw1.c`,
`HW1.cc`, `hw1.cpp`, `report.docx`, `report.pages`.

# Grading

1. (40%) Correctness. Propotional to the number of test cases solved.
2. (30%) Performance. Based on the total time you solve all the test cases (including hidden test cases). For a failed test case, 75 seconds is added to your total time.
3. (30%) Report.

# Appendix

Please note that in this spec, the sample test cases and programs might contain bugs.
If you spotted a potential one and are not quite sure about it, please raise a question on EEClass.

## Sample Testcases

The sample test cases are located at `/home/ipc22/share/hw1/samples`.

## Playing the game interactively

You can play sokoban in the terminal with `/home/ipc22/share/hw1/play.py`.

For example, run `/home/ipc22/share/hw1/play.py /home/ipc22/share/hw1/samples/01.txt`
to play the first sample input.

## Output validation

`/home/ipc22/share/hw1/validate.py` can be used to validate your output.

For example, the following command validates your answer for `01.txt` when running
6 threads:

~~~
srun -c6 -o answer.txt ./hw1 01.txt
/home/ipc22/share/hw1/validate.py 01.txt answer.txt
~~~

You can also use:

~~~
srun -c6 ./hw1 01.txt | /home/ipc22/share/hw1/validate.py 01.txt -
~~~

Here, `-` instructs `validate.py` to read your output from stdin.

## Automatic Judge

The `hw1-judge` command can be used to automatically judge your code against all sample test cases. It also submits your execution time to the scoreboard so that you can compare your performance with the others in the class.


The scoreboard is [here](https://apollo.cs.nthu.edu.tw/ipc22/scoreboard/hw1/).

To use it, run `hw1-judge` in the directory that contains your code `hw1.cc`.
It will automatically search for `Makefile` and use it to compile your code,
or fallback to the TA provided `/home/ipc22/share/hw1/Makefile` otherwise.
If code compiliation is successful, it will then run all the sample test cases, and show you the results as well as update the scoreboard.

> Note: `hw1-judge` and the scoreboard has nothing to do with grading.
> Only the code submitted to eeclass is considered for grading purposes.

The judge supports `-xpattern` and `-ipattern` filters to exclude or include
test cases. These filters are applied in the order they are specified.
These filters support [glob(7)](http://man7.org/linux/man-pages/man7/glob.7.html)
patterns. When multiple filters match a given test case, the later filter takes priority.
For example, to skip `05.txt` and `06.txt`, you can run:

~~~
hw1-judge -x05.txt -x06.txt
~~~

To run only the case `02.txt`:

~~~
hw1-judge '-x*' -i02.txt
~~~

### Judge Verdict Table

| Verdict | Explaination |
|--|--|
| internal error | there is a bug in the judge |
| time limit exceeded+ | execution time > time limit + 10 seconds |
| time limit exceeded | execution time > time limit |
| runtime error | your program didn't return 0 or is terminated by a signal |
| no output | your program did not produce an output file |
| wrong answer | your output is incorrect |
| accepted | you passed the test case |

