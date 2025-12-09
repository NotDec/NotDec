#!/bin/bash
SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )

# include util functions
source "$SCRIPT_DIR/utils.sh"

CODE_HOME=`dirname ${SCRIPT_DIR}` # parent folder of scripts folder
echo "project is at ${CODE_HOME}"
cd $CODE_HOME
LLVMHome="$CODE_HOME/llvm-14.0.6.obj"

# SourceLLVM="https://github.com/llvm/llvm-project/archive/refs/tags/llvmorg-14.0.6.zip"


read -r -p "Do you have tons of memory and disk space (>40GB) for Debug info? [y/N] " response
if [[ "$response" =~ ^([yY][eE][sS]|[yY])$ ]]
then
    # build_type=RelWithDebInfo
    build_type=Debug
else
    build_type=Release
fi


mkdir "$LLVMHome"
# echo "Downloading LLVM source..."
# generic_download_file "$SourceLLVM" llvm.zip

if [ ! -e "llvm-source" ]; then
    echo "Cloning LLVM source..."
    git clone https://github.com/NotDec/llvm-project.git llvm-source
    git -C llvm-source checkout llvmorg-14.0.6
fi

echo "Building LLVM..."
mkdir llvm-build
cd llvm-build

# 出错了直接退出
set -e

cmake -DCMAKE_BUILD_TYPE=${build_type} -DCMAKE_INSTALL_PREFIX="$LLVMHome" -DLLVM_OPTIMIZED_TABLEGEN=ON -DLLVM_ENABLE_PROJECTS="clang;compiler-rt" ../llvm-source/llvm
# 直接多线程编译可以出现内存不足的情况，后面链接时减少并行数量。经测试，32G内存在后期链接也只能并行数量1。
cmake --build . -j `nproc` || cmake --build . -j 1
# cmake --install .
cmake --build . --target install

cd ..
# 暂时编译好了手动删吧。
# rm -r llvm-source llvm-build
# rm llvm.zip

# Package the folder for delivery, use the following command
# echo "compressing llvm-14.0.6.obj to llvm-14.0.6.obj.tar.xz"
# tar -I 'xz -0' -cf llvm-14.0.6.obj.tar.xz llvm-14.0.6.obj
