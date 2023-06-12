mkdir bin

retdec_url=https://github.com/avast/retdec/releases/download/v4.0/retdec-v4.0-debian-64b.tar.xz
wamrc_url=https://github.com/bytecodealliance/wasm-micro-runtime/releases/download/WAMR-1.1.2/wamrc-1.1.2-x86_64-ubuntu-20.04.tar.gz
wavm_url=https://github.com/WAVM/WAVM/releases/download/nightly%2F2021-05-10/wavm-0.0.0-prerelease-linux.tar.gz

#retdec-llvmir2hll
if [ -e ./bin/retdec-llvmir2hll ]; then
  echo retdec exist
else
  echo retdec not exist
  if [ -e retdec-v4.0-debian-64b.tar.xz ]; then
    echo retdec-v4.0-debian-64b.tar.xz exist
  else
    echo retdec-v4.0-debian-64b.tar.xz not exist
    wget $retdec_url
  fi
  tar -xvf  retdec-v4.0-debian-64b.tar.xz retdec/bin/retdec-llvmir2hll --strip-component=2 
  mv ./retdec-llvmir2hll ./bin/retdec-llvmir2hll
fi

#wamr
if [ -e ./bin/wamrc ]; then
  echo wamrc exist
else
  echo wamrc not exist
  wget $wamrc_url
  tar -xvf  wamrc-1.1.2-x86_64-ubuntu-20.04.tar.gz -C ./bin

fi


#wavm
if [ -e ./bin/wavm ]; then
  echo wavm exist
else
  echo wavm not exist

  wget $wavm_url
  tar -xvf wavm-0.0.0-prerelease-linux.tar.gz bin/wavm -C ./bin
fi





