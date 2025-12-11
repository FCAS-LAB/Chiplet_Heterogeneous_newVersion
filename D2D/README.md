1.	编译

1.1	编译boost库

先下载boost库，再编译。

mkdir 3rd_party

cd 3rd_party/

git clone https://github.com/boostorg/boost.git --recurse-submodules

cd boost

./bootstrap.sh

./b2

cd ../../

1.2	 编译UCIE CPP代码

./build.sh

2.	运行测试用例

./run.sh

./build/test/run_tests --gtest_filter=ProtocolLayerTest.TestProtocolLayerStallReqAck

gdb ./build/test/run_tests
r --gtest_filter=ProtocolLayerTest.TestProtocolLayerStallReqAck

