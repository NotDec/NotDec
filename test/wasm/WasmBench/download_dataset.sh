url="https://github.com/sola-st/WasmBench/releases/download/v1.0/filtered-binaries-metadata.7z"


#download dataset
if [ -e "./filtered-binaries-metadata.7z" ]; then
  echo filtered-binaries-metadata.7z existed
else
  echo filtered-binaries-metadata.7z not exist 
  wget $url 
fi

if [ -e "./filtered" ]; then
  echo dir existed
else
  echo dir not exist
  7z x filtered-binaries-metadata.7z 
fi


