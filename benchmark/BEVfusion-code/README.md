# BEVfusion-code

## 1. 环境配置

### 1.1 安装依赖

1. 安装libtorch
```bash
wget https://download.pytorch.org/libtorch/nightly/cpu/libtorch-shared-with-deps-latest.zip
unzip libtorch-shared-with-deps-latest.zip
```

2. 安装onnxruntime(版本v1.21.0)
参考[onnxruntime安装教程](https://blog.csdn.net/m0_46303486/article/details/131681105?ops_request_misc=%257B%2522request%255Fid%2522%253A%2522a93c511fac4512b1e4db900e3bb84c61%2522%252C%2522scm%2522%253A%252220140713.130102334.pc%255Fblog.%2522%257D&request_id=a93c511fac4512b1e4db900e3bb84c61&biz_id=0&utm_medium=distribute.pc_search_result.none-task-blog-2~blog~first_rank_ecpm_v1~rank_v31_ecpm-8-131681105-null-null.nonecase&utm_term=ubuntu%20onnxruntime%20%E7%BC%96%E8%AF%91cpu&spm=1018.2226.3001.4450)

3. 下载BEVfusion代码
```bash
git clone https://github.com/superQCman/BEVfusion_code.git # 放在Chiplet_Heterogeneous_newVersion/benchmark目录下
```

### 1.2 CMakeLists.txt配置

1. 配置libtorch路径
```bash
set(CMAKE_PREFIX_PATH "/home/ting/SourceCode/libtorch")  # 替换为你的LibTorch路径
```

2. 配置onnxruntime路径
```bash
set(ONNXRUNTIME_INCLUDE_DIR "/usr/local/include/onnxruntime") # 替换为你的onnxruntime路径
set(ONNXRUNTIME_LIB_DIR "/usr/local/lib") # 替换为你的onnxruntime路径
```

## 2. 编译
```bash
python setup.py
```

## 3. 运行
```bash
./run.sh
```

## 4. 清空仿真信息
```bash
./clean.sh
```


