# Hello message.
printf "> Operating System I Project <xxx，xxx>\n"

# Process running through
printf "+ Run & Check:\n"
## Clean and recompile
make clean -s && make -s
## Run raw (unoptimized) project
printf " #1.1 Running unoptimized project..."
iostat -x 10 30 > result/raw-perf &
./bin/raw_project > result/raw-time
printf "finished.\n"
printf " #1.2 Checking correctness of results..."
./bin/check > result/raw-check
printf "done.\n"
make clear -s
## Run optimized project
printf " #2.1 Running optimized project....."
iostat -x 10 15 > result/opt-perf &
./bin/opt_project > result/opt-time
printf "finished.\n"
printf " #2.2 Checking correctness of results..."
./bin/check > result/opt-check
printf "done.\n"
make sweep -s

# Show analysis data
printf "+ Performance:\n"
./bin/analyze
