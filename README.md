
For the detail documents in English, please access [Document for LegoSim](https://fcas-lab.github.io/Chiplet_Heterogeneous_newVersion/).

# 安装

## 下载仓库并设置环境

1. 从github上下载仓库。

    ```
    git clone --single-branch --branch master_v2 https://github.com/FCAS-LAB/Chiplet_Heterogeneous_newVersion.git
    ```

    进入仿真器根目录，以下的示例命名都假设从仿真器根目录开始执行。

2. 初始化并更新submodule。

    ```
    git submodule init
    git submodule update
    ```

3. 运行脚本，初始化环境变量

    ```
    source setup_env.sh
    ```

    运行成功应出现：setup_environment succeeded

4. 对于snipersim和gpgpu-sim代码进行修改。

    ```
    ./apply_patch.sh
    ```

    更多细节参见下文“打包和应用Patch”章节。

5. 编译安装snipersim。新版本的snipersim提供了非常自动化的编译脚本，直接执行make即可。

    ```
    cd snipersim
    make -j4
    ```

6. 编译安装Gem5。请查看Gem5文档获取详细安装指南。LegoSim中可以运行X86和ARM架构仿真器：

    ```
    cd gem5
    scons build/X86/gem5.opt
    ```

    或者

    ```
    cd gem5
    scons build/ARM/gem5.opt
    ```

7. 编译安装GPGPUSim。GPGPUsim安装有前置条件：

    1. GPGPUSim需要安装cuda。新版本的gpgpusim可以支持cuda4到cuda11的任意版本，详细信息请参见GPGPUSim的README。
    2. GPGPUSim对于编译版本有要求，建议使用GCC7。

    配置好Cuda和编译器，可以直接执行make。

    ```
    cd gpgpu-sim
    make -j4
    ```

8. 编译安装popnet

    ```
    cd popnet_chiplet
    mkdir build
    cd build
    cmake ..
    make -j4
    ```

9.  编译安装芯粒间通信程序。interchiplet提供了芯粒间通信所需要的API和实现代码。

    ```
    cd interchiplet
    mkdir build
    cd build
    cmake ..
    make
    ```

    编译完成后应在interchiplet/bin下找到record_transfer和zmq_pro，在interchiplet/lib下找到libinterchiplet_app.a。

    zmq_pro需要安装zmq环境。通常会在cmake步骤被忽略。

# 验证安装

正确执行上述过程后，可以使用benchmark/matmul验证环境设置是否正确。

1. 设置仿真器环境

    ```
    source setup_env.sh
     ```

2. 编译可执行文件

    ```
    cd benchmark/matmul
    make
    ```

3. 执行可执行文件。示例包含4个进程，分别是1个CPU进行和3个GPU进程。必须在benchmark/matmul进程执行。

    ```
    ../../interchiplet/bin/interchiplet ./matmul.yml
    ```

    执行后，可以在benchmark/matmul文件下找到一组proc_r{R}_p{P}_t{T}的文件夹，对应于第R轮执行的第P阶段的第T个线程。
    在文件夹中可以找到下列文件：

    1. GPGPUSim仿真的临时文件和日志文件gpgpusim_X_X.log。
    2. Sniper仿真的临时文件和sniper仿真的日志文件sniper.log。
    3. Popnet的日志文件popnet.log。

4. 清理可执行文件和输出文件。

    ```
    make clean
    ```

# 打包和应用Patch

由于sniper和GPGPUSim是用submodule方式引入的，对于snipersim和gpgpu-sim的修改不会通过常规的git流程追踪。因此，工程提供了patch.sh和apply_patch.sh两个脚本通过Patch管理sniper和gpgpu-sim的修改。

patch.sh脚本用来生成Patch：

```
./patch.sh
```

1. 使用patch.sh脚本将snipersim和gpgpu-sim的修改分别打包到snipersim.diff和gpgpu-sim.diff文件中。diff文件保存在interchiplet/patch下面。diff文件会被git追踪。
2. patch.sh脚本还会将被修改的文件按照文件层次结构保存到.changed_files文件夹中，用于在diff文件出错时进行查看和参考。

apply_patch.sh脚本用来应用Patch：

```
./apply_patch.sh
```

1. 使用apply_patch.sh脚本将snipersim.diff和gpgpu-sim.diff文件应用到snipersim和gpgpu-sim，重现对于文件的修改。
2. 当apply出错时，可以参考.changed_files中的文件手动修改snipersim和gpgpu-sim的文件。

需要说明的是：不建议用.changed_files直接覆盖snipersim和gpgpu-sim文件夹。因为snipersim和gpgpu-sim本身的演进可能会与芯粒仿真器修改相同的文件。使用Patch的方式会报告修改的冲突。如果直接覆盖，则会导致不可预见的错误。

# 添加测试程序

测试程序统一添加到benchmark路径下，每一个测试文件有独立的文件夹。

测试程序的文件管理推荐按照matmul组织，并且使用类似的Makefile。但是并不绝对要求。

运行测试程序需要编写YAML配置文件。

## YAML配置文件格式

```
# Phase 1 configuration.
phase1:
  # Process 0
  - cmd: "$BENCHMARK_ROOT/bin/matmul_cu"
    args: ["0", "1"]
    log: "gpgpusim.0.1.log"
    is_to_stdout: false
    pre_copy: "$SIMULATOR_ROOT/gpgpu-sim/configs/tested-cfgs/SM2_GTX480/*"
  # Process 1
  - cmd: "$BENCHMARK_ROOT/bin/matmul_cu"
    args: ["1", "0"]
    log: "gpgpusim.1.0.log"
    is_to_stdout: false
    pre_copy: "$SIMULATOR_ROOT/gpgpu-sim/configs/tested-cfgs/SM2_GTX480/*"
  ......

# Phase 2 configuration.
phase2:
  # Process 0
  - cmd: "$SIMULATOR_ROOT/popnet/popnet"
    args: ["-A", "2", "-c", "2", "-V", "3", "-B", "12", "-O", "12", "-F", "4", "-L", "1000", "-T", "10000000", "-r", "1", "-I", "../bench.txt", "-R", "0"]
    log: "popnet.log"
    is_to_stdout: false

```

YAML配置文件的第一层支持的关键字是：

- `phase1`：配置第一阶段的仿真器进程。
- `phase2`：配置第二阶段的仿真器进程。

这两个关键字下面都是数组，每项对应于一个并发的仿真器进程。`phase1`和`phase2`都可以支持多个仿真进程。

仿真器进程的配置支持如下关键字：

- `cmd`：表示仿真器的命令。字符串表示。支持环境变量`$BENCHMARK_ROOT`和`$SIMULATOR_ROOT`。
- `args`：表示仿真器的参数。字符串数组表示。支持环境变量`$BENCHMARK_ROOT`和`$SIMULATOR_ROOT`。
- `log`：表示日志的名称。不能使用相对路径或绝对路径。
- `is_to_stdout`：表示是否将仿真器的标准输出/错误输出重定向到interchiplet的标准输出。
- `pre_copy`：有些仿真器需要一些额外的文件才能启动仿真。这个关键字是字符串。如果需要复制多个文件，则用空格隔开，用引号包围。

在YAML里面使用相对路径时，以当前路径作为基础。推荐使用环境变量构成绝对路径。

- `$BENCHMARK_ROOT`表示测试程序的路径，根据YAML文件的位置决定。
- `$SIMULATOR_ROOT`表示仿真器的路径，通过setup_env.sh决定。

## 运行InterChiplet

仿真器的主程序是InterChiplet。在运行路径下执行下面的命令：

```
$SIMULATOR_ROOT/interchiplet/bin/interchiplet $BENCHMARK_ROOT/bench.yml
```

InterChiplet命令格式如下：

```
interchiplet <bench>.yml [--cwd <string>] [-t|--timeout <int>] [-e|--error <float>] [-h]
```

命令参数如下：

- `<bench>.yml`指定测试程序的配置文件。
- `--cwd <string>`指定执行仿真的路径。
- `-t <int>`和`--timeout <int>`指定仿真退出的轮次。不论结果是否收敛，都会结束仿真。
- `e <float>`和`--error <float>`指定仿真退出的条件。当仿真误差小于这个比例时，结束仿真。

