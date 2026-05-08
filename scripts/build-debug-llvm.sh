#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )
CODE_HOME="$(dirname "${SCRIPT_DIR}")" # parent folder of scripts folder
echo "project is at ${CODE_HOME}"
cd "$CODE_HOME"

# Keep the default on the target migration version, but leave the script usable
# for quick version bisects by overriding these variables in the environment.
LLVM_VERSION="${LLVM_VERSION:-22.1.0}"
LLVM_TAG="${LLVM_TAG:-llvmorg-${LLVM_VERSION}}"
LLVM_SOURCE_HOME="${LLVM_SOURCE_HOME:-$CODE_HOME/llvm-source}"
LLVM_BUILD_HOME="${LLVM_BUILD_HOME:-$CODE_HOME/llvm-build}"
LLVM_INSTALL_HOME="${LLVM_INSTALL_HOME:-$CODE_HOME/llvm-${LLVM_VERSION}.obj}"
LLVM_ENABLE_PROJECTS="${LLVM_ENABLE_PROJECTS:-clang}"
LLVM_ENABLE_RUNTIMES="${LLVM_ENABLE_RUNTIMES:-compiler-rt}"
BUILD_JOBS="${BUILD_JOBS:-$(nproc 2>/dev/null || echo 1)}"

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

if [ ! -e "$LLVM_SOURCE_HOME" ]; then
    echo "Cloning LLVM source ${LLVM_TAG}..."
    git clone --depth 1 --branch "$LLVM_TAG" https://github.com/llvm/llvm-project.git "$LLVM_SOURCE_HOME"
elif [ ! -d "$LLVM_SOURCE_HOME/.git" ]; then
    echo "$LLVM_SOURCE_HOME exists but is not a git checkout."
    exit 1
else
    # Existing source trees are accepted only when they match the requested tag.
    # This avoids accidentally rebuilding LLVM 14 after the migration rename.
    current_tag="$(git -C "$LLVM_SOURCE_HOME" describe --tags --exact-match 2>/dev/null || true)"
    if [[ "$current_tag" != "$LLVM_TAG" ]]; then
        echo "$LLVM_SOURCE_HOME is at '${current_tag:-untagged HEAD}', expected '$LLVM_TAG'."
        echo "Rename it away, remove it, or override LLVM_TAG/LLVM_SOURCE_HOME intentionally."
        exit 1
    fi
fi

echo "Building LLVM ${LLVM_TAG}..."
rm -rf "$LLVM_BUILD_HOME"

cmake_args=(
    -G Ninja
    -DCMAKE_BUILD_TYPE="${build_type}"
    -DCMAKE_INSTALL_PREFIX="$LLVM_INSTALL_HOME"
    -DLLVM_OPTIMIZED_TABLEGEN=ON
    -DLLVM_BUILD_LLVM_DYLIB=ON
    -DLLVM_LINK_LLVM_DYLIB=ON
)

if [[ -n "$LLVM_ENABLE_PROJECTS" ]]; then
    cmake_args+=(-DLLVM_ENABLE_PROJECTS="$LLVM_ENABLE_PROJECTS")
fi

if [[ -n "$LLVM_ENABLE_RUNTIMES" ]]; then
    cmake_args+=(-DLLVM_ENABLE_RUNTIMES="$LLVM_ENABLE_RUNTIMES")
fi

cmake "${cmake_args[@]}" -S "$LLVM_SOURCE_HOME/llvm" -B "$LLVM_BUILD_HOME"

# 直接多线程编译可以出现内存不足的情况，后面链接时减少并行数量。经测试，32G内存在后期链接也只能并行数量1。
cmake --build "$LLVM_BUILD_HOME" -j "$BUILD_JOBS" || cmake --build "$LLVM_BUILD_HOME" -j 1
cmake --build "$LLVM_BUILD_HOME" --target install

# 暂时编译好了手动删吧。
# rm -r llvm-source llvm-build
# rm llvm.zip

# Package the folder for delivery, use the following command
# echo "compressing llvm-${LLVM_VERSION}.obj to llvm-${LLVM_VERSION}.obj.tar.xz"
# tar -I 'xz -0' -cf llvm-${LLVM_VERSION}.obj.tar.xz llvm-${LLVM_VERSION}.obj
