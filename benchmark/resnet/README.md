# Benchmark移植指引
## 运行：
```shell
make # 编译
make run # 运行
make clean # 清空
```
## 注意：
resnet.cu是之前没有pytorch-GPGPUSim的时候自己实现的算子，现在有pytorch-GPGPUSim后直接用pytorch就行了。
