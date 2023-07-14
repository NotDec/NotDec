import os
import sys
import subprocess
import re
import time

cwd = os.path.dirname(os.path.dirname(os.path.realpath(__file__)))
bin_dir = cwd + "/bin"
data_dir = cwd + "/dataset/dataset-SAC-2022/wasm/"
out_dir = cwd + "/SVF/output"
result_dir = out_dir + "/clang_result"
compile_dir = out_dir + "/clang_liftedIR"


# 统计结果
print("===================Lift Result==================")
ll_count = 0
for root, dirs, files in os.walk(compile_dir):
    for f in files:
        #file size
        if(os.path.getsize(os.path.join(root, f)) == 0):
            continue
        ll_count += 1
print("total ll: ", ll_count)


print("===================Saber Result==================")
result_dir = os.path.join(cwd, "dataset", "out_dataset-SAC-2022", "src")
result_compile = {}
result_dec = {}
for file in os.listdir(result_dir):
    if file.endswith(".dec.ll.svf"):
        res = open(os.path.join(result_dir, file), "r")
        res = res.read()
        if res != "":
            print(f"{os.path.join(result_dir, file)}: {res}")
            CWE_type = file.split("_")[0]
            result_dec[CWE_type] = result_dec.get(CWE_type, 0) + 1
    if file.endswith(".com.ll.svf"):
        res = open(os.path.join(result_dir, file), "r")
        res = res.read()
        if res != "":
            print(f"{os.path.join(result_dir, file)}: {res}")
            CWE_type = file.split("_")[0]
            result_compile[CWE_type] = result_compile.get(CWE_type, 0) + 1

result_dec = {k:result_dec[k] for k in sorted(result_dec)}
result_compile = {k:result_compile[k] for k in sorted(result_compile)}

print(result_compile)
print(result_dec)
total_CWE = {
    "CWE122": 780,
    "CWE127": 226,
    "CWE121": 1605,
    "CWE124": 401,
    "CWE400": 104,
    "CWE617": 59,
    "CWE690": 103,
    "CWE194": 173,
    "CWE253": 123,
    "CWE134": 175,
    "CWE457": 120,
    "CWE590": 288,
    "CWE761": 91,
    "CWE415": 12,
    "CWE126": 299,
    "CWE416": 17,
    "CWE665": 55,
    "CWE476": 24,
    "CWE789": 50,
    "CWE401": 120,
    "CWE252": 70,
    "CWE758": 66,
    "CWE475": 18,
    "CWE391": 17,
    "CWE675": 46,
    "CWE404": 23,
    "CWE390": 35,
    "CWE688": 15,
    "CWE587": 17,
    "CWE195": 25,
    "CWE190": 8,
    "CWE526": 17,
    "CWE123": 21,
    "CWE511": 17,
    "CWE197": 2,
    "CWE685": 14,
    "CWE674": 1,
    "CWE191": 3,
    "CWE483": 1,
}

for i in result_compile:
    if("std" in i):
        continue
    print(
        i, result_compile[i], "/", total_CWE[i], str(result_compile[i] / total_CWE[i] * 100)[:4] + "%"
    )
print("=====decompile======")
for i in result_dec:
    if("std" in i):
        continue
    print(
        i, result_dec[i], "/", total_CWE[i], str(result_dec[i] / total_CWE[i] * 100)[:4] + "%"
    )
    
