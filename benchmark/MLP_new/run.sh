#!/bin/bash

set -e  # 遇到错误就退出

# 测试配置列表
configs=(
    "2 mesh"
    "4 mesh"
    "2 meshll"
    "4 meshll"
    "2 torus"
    "4 torus"
    "2 star"
    "4 star"
    "2 NVL"
    "4 NVL"
)


run_test() {
    local flit_size=$1
    local topology=$2
    local result_file="result_${topology}_flit_${flit_size}.log"
    local bottleneck_file="bottleneck_${topology}_flit_${flit_size}.log"
    
    echo "Testing: ${topology} with flit_size ${flit_size}"
    
    python modify.py --flit_size "${flit_size}" --topology "${topology}"
    make
    make run > "${result_file}" 2>&1
    python result.py --input "${result_file%.log}"
    python heatMap.py --topology "${topology}" --flit_size "${flit_size}"
    python bottleneck_analyse.py > "${bottleneck_file}" 2>&1
    make clean
    
    echo "Completed: ${result_file}"
}


echo "Starting batch tests..."

for config in "${configs[@]}"; do
    read -r flit_size topology <<< "$config"
    run_test "$flit_size" "$topology"
done

echo "All tests completed!"