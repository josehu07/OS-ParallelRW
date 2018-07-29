## File Tree Structure
```
  |_code
      |_bin
      |_input
          |_systor17-01.tar
      |_output
      |_result
      |_src
          |_analyze.c
          |_check.c
          |_opt_project.c
          |_raw_project.c
      |_Makefile
      |_run-project.sh
      |_README
  |_results
      |_R.csv (online)
      |_W.csv (online)
  |_report.pdf
```

## Usage of *Makefile* (optional)
- ```make raw_project```: compile unoptimized project executable *raw_project*.
- ```make opt_project```: compile optimized project executable *opt_project*.
- ```make check```: compile correctness checking executable *check*.
- ```make analyze```: compile performance analysis executable *analyze*.
- ```make (all)```: compile all four binaries.
- ```make clean```: remove all compiled binaries, decompressed inputs and generated outputs.

## IMPORTANT Prerequisites
- Please make sure that the **file tree structure is complete**.
- Please make sure that the **source archive file *systor17-01.tar*'* is placed under *input* folder**, and is named correctly.
- Execute ```make clean``` in advance, if you are running *raw_project* or *opt_project* by yourself, instead of using *run-project.sh* shell script.

## HOWTO run our project
- Execute `./run-project.sh' under `code/'. This shell script will do the following things:
  1. Clean all compiled binaries, decompressed inputs and generated outputs.
  2. Run unoptimized project & Check the correctness of results.
  3. Clear all intermediate files.
  4. Run optimized project & Check the correctness of results.
  5. Analyze performance statistics and show.
- An example of terminal output will be as follows:
```
  > Operating System I Project <张心瑜，胡冠洲>
  + Run & Check:
   #1.1 Running unoptimized project...finished.
   #1.2 Checking correctness of results...done.
   #2.1 Running optimized project.....finished.
   #2.2 Checking correctness of results...done.
  + Performance:
                     Unoptimized    Optimized
   Timespan (secs)        293.47       117.40
   % CPU Util Rate        10.840       41.012
   % I/O Time Used         2.825        6.640
   Bandwidth (kB/s)      45615.7     100176.1
```
