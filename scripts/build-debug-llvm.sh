#!/bin/bash
set -euo pipefail

SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )

# include util functions
source "$SCRIPT_DIR/utils.sh"

CODE_HOME=`dirname ${SCRIPT_DIR}` # parent folder of scripts folder
echo "project is at ${CODE_HOME}"
cd $CODE_HOME
LLVMBuildHome="$CODE_HOME/llvm-build"
LLVMInstallHome="$CODE_HOME/llvm-14.0.6.obj"

# SourceLLVM="https://github.com/llvm/llvm-project/archive/refs/tags/llvmorg-14.0.6.zip"

build_type="${BUILD_TYPE:-}"
if [[ -z "${build_type}" ]]; then
    if [[ -t 0 ]]; then
        read -r -p "Do you have tons of memory and disk space (>40GB) for Debug info? [y/N] " response
        if [[ "$response" =~ ^([yY][eE][sS]|[yY])$ ]]; then
            build_type=Debug
        else
            build_type=Release
        fi
    else
        build_type=Debug
    fi
fi
# echo "Downloading LLVM source..."
# generic_download_file "$SourceLLVM" llvm.zip

if [ ! -e "llvm-source" ]; then
    echo "Cloning LLVM source..."
    git clone --depth 1 https://github.com/llvm/llvm-project.git -b llvmorg-14.0.6 llvm-source
fi

echo "Building LLVM..."
rm -rf "$LLVMBuildHome"
mkdir -p "$LLVMBuildHome"
cd "$LLVMBuildHome"

cmake \
    -G Ninja \
    -DCMAKE_BUILD_TYPE="${build_type}" \
    -DCMAKE_INSTALL_PREFIX="$LLVMInstallHome" \
    -DLLVM_OPTIMIZED_TABLEGEN=ON \
    -DLLVM_ENABLE_PROJECTS="clang;compiler-rt" \
    -DLLVM_BUILD_LLVM_DYLIB=ON \
    -DLLVM_LINK_LLVM_DYLIB=ON \
    ../llvm-source/llvm
# 直接多线程编译可以出现内存不足的情况，后面链接时减少并行数量。经测试，32G内存在后期链接也只能并行数量1。
cmake --build . -j `nproc` || cmake --build . -j 1
cmake --build . --target install

cd ..
# 暂时编译好了手动删吧。
# rm -r llvm-source llvm-build
# rm llvm.zip

# Package the folder for delivery, use the following command
# echo "compressing llvm-14.0.6.obj to llvm-14.0.6.obj.tar.xz"
# tar -I 'xz -0' -cf llvm-14.0.6.obj.tar.xz llvm-14.0.6.obj
