#!/bin/bash
SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )

# include util functions
source "$SCRIPT_DIR/utils.sh"

CODE_HOME=`dirname ${SCRIPT_DIR}` # parent folder of scripts folder
echo "project home: ${CODE_HOME}"

LLVM_OBJ="llvm-14.0.6.obj"
echo "try to get ${LLVM_OBJ}"

# 如果文件夹已经存在
if [ -e "${CODE_HOME}/${LLVM_OBJ}" ]; then
    echo "${LLVM_OBJ} already exists. exiting"
    exit 0
fi

# 如果有压缩包直接解压，没有压缩包就询问下载链接
if [ -e "${CODE_HOME}/${LLVM_OBJ}.tar.xz" ]; then
    download_url="${CODE_HOME}/${LLVM_OBJ}.tar.xz"
else
    read -p "Enter local file path or download url (default to github release TODO): " download_url
    if [[ $download_url =~ https?://.* ]]; then
        echo "download to ${CODE_HOME}/${LLVM_OBJ}.tar.xz"
        generic_download_file "$download_url" "${CODE_HOME}/${LLVM_OBJ}.tar.xz" || (echo "download failed !!!"; exit 1)
        download_url="${CODE_HOME}/${LLVM_OBJ}.tar.xz"
        # less than 10M then fail it.
        if [[ $(wc -c <"$download_url") -lt 10000000 ]]; then 
            echo "download failed !!!: file too small: please check your url."
            rm "${download_url}"
            exit 1
        fi
    else
        echo "local path: ${download_url}"
    fi
fi

# 解压得到文件夹
echo "extracting ${download_url} to ${LLVM_OBJ}"
tar xf ${download_url}
