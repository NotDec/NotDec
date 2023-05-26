mkdir bin
#retdec-llvm2irhll
url=https://github.com/avast/retdec/releases/download/v4.0/retdec-v4.0-debian-64b.tar.xz
wget $url
tar -xvf retdec-v4.0-debian-64b.tar.xz retdec/bin/retdec-llvmir2hll --strip-component=2 -C ./bin

#wamr
url=https://github.com/bytecodealliance/wasm-micro-runtime/releases/download/WAMR-1.1.2/wamrc-1.1.2-x86_64-ubuntu-20.04.tar.gz
wget $url
tar -xvf  wamrc-1.1.2-x86_64-ubuntu-20.04.tar.gz -C ./bin

#wavm
url=https://github.com/WAVM/WAVM/releases/download/nightly%2F2022-05-14/wavm-0.0.0-prerelease-linux.tar.gz
wget $url
tar -xvf wavm-0.0.0-prerelease-linux.tar.gz bin/wavm --strip-component=1 -C ./bin



