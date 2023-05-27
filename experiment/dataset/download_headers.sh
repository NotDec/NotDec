
if [ -d "./dataset-SAC-2022" ]; then
  echo "文件夹已存在，跳过解压缩步骤"
else
  echo "文件夹不存在，解压缩 dataset-SAC-2022.tar.xz"
  tar xvf ./dataset-SAC-2022.tar.xz
fi

svn co https://github.com/arichardson/juliet-test-suite-c/trunk/testcasesupport
cp -r ./testcasesupport/* ./dataset-SAC-2022/src/
rm -rf ./testcasesupport