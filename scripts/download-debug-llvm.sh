#!/bin/bash
SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )

# include util functions
source "$SCRIPT_DIR/utils.sh"

CODE_HOME=`dirname ${SCRIPT_DIR}` # parent folder of scripts folder
echo "project is at ${CODE_HOME}"
LLVMHome="$CODE_HOME/llvm-14.0.6.obj"


read -p "Enter local file path or download url (default to github release): " download_url

