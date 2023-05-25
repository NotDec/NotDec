import os
import sys
import subprocess
import re
import time


init = time.time()
# traverse all files
cwd = os.path.dirname(os.path.realpath(__file__))
print(cwd)

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
d = {'CWE122': 35, 'CWE761': 39, 'CWE758': 30, 'CWE457': 34, 'CWE124': 125, 'CWE690': 20, 'CWE127': 120, 'CWE401': 20, 'CWE416': 17, 'CWE415': 6, 'std': 3, 'CWE126': 1}


total_CWE = {'CWE122': 780, 'CWE127': 226, 'CWE121': 1605, 'CWE124': 401, 'CWE400': 104, 'CWE617': 59, 'CWE690': 103, 'CWE194': 173, 'CWE253': 123, 'CWE134': 175, 'CWE457': 120, 'CWE590': 288, 'CWE761': 91, 'CWE415': 12, 'CWE126': 299, 'CWE416': 17, 'CWE665': 55, 'CWE476': 24, 'CWE789': 50, 'CWE401': 120, 'CWE252': 70, 'CWE758': 66, 'CWE475': 18, 'CWE391': 17, 'CWE675': 46, 'CWE404': 23, 'CWE390': 35, 'CWE688': 15, 'CWE587': 17, 'CWE195': 25, 'CWE190': 8, 'CWE526': 17, 'CWE123': 21, 'CWE511': 17, 'CWE197': 2, 'CWE685': 14, 'CWE674': 1, 'CWE191': 3, 'CWE483': 1}
for i in d:
    print(i,d[i],"/",total_CWE[i] ,str(d[i]/total_CWE[i] *100)[:4] + "%")