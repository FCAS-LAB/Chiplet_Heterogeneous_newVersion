export BENCHMARK_ROOT=$SIMULATOR_ROOT/benchmark/BEVfusion-code
ulimit -s unlimited
../../interchiplet/bin/interchiplet BEVfusion.yml -w 4 -f 2