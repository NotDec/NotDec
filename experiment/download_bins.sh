mkdir bin

#retdec-
if [ -e ./bin/retdec-llvmir2hll ]; then
  echo retdec存在
else
  echo retdec不存在 
  url=https://github.com/avast/retdec/releases/download/v4.0/retdec-v4.0-debian-64b.tar.xz
  wget $url
  tar -xvf retdec-v4.0-debian-64b.tar.xz retdec/bin/retdec-llvmir2hll --strip-component=2 -C ./bin
fi

#wamr
if [ -e ./bin/wamrc ]; then
  echo wamrc存在
else
  echo wamrc不存在 
  url=https://github.com/bytecodealliance/wasm-micro-runtime/releases/download/WAMR-1.1.2/wamrc-1.1.2-x86_64-ubuntu-20.04.tar.gz
  wget $url
  tar -xvf  wamrc-1.1.2-x86_64-ubuntu-20.04.tar.gz -C ./bin

fi


#wavm
if [ -e ./bin/wavm ]; then
  echo wavm存在
else
  echo wavm不存在 
  #url=https://github.com/WAVM/WAVM/releases/download/nightly%2F2022-05-14/wavm-0.0.0-prerelease-linux.tar.gz
  url=https://github.com/WAVM/WAVM/releases/download/nightly%2F2021-05-10/wavm-0.0.0-prerelease-linux.tar.gz
  wget $url
  tar -xvf wavm-0.0.0-prerelease-linux.tar.gz bin/wavm -C ./bin
fi





