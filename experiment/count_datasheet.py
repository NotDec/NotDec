import os
import sys
import subprocess
import re
import time

init = time.time()
# traverse all files
cwd = os.path.dirname(os.path.realpath(__file__))

data_dir = cwd + "/dataset/dataset-SAC-2022/wasm/"
result = {}

for root, dirs, files in os.walk(data_dir):
    for f in files:
        CWE_type = f.split("_")[0]
        if CWE_type not in result:
            result[CWE_type] = 1
        else:
            result[CWE_type] += 1

# 统计结果
print(result)
end = time.time()
print("[+] total time: ", end - init)
