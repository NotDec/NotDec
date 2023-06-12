
if [ -d "./dataset-SAC-2022" ]; then
  echo "dataset existed"
else
  echo "unzipping dataset-SAC-2022.tar.xz"
  tar xvf ./dataset-SAC-2022.tar.xz
fi

svn co https://github.com/arichardson/juliet-test-suite-c/trunk/testcasesupport
cp -r ./testcasesupport/* ./dataset-SAC-2022/src/
rm -rf ./testcasesupport