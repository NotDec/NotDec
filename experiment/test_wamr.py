import os
import sys
import subprocess
import re

# get pwd
pwd = os.path.dirname(os.path.realpath(__file__))


def getIR_WAMR(wasm, ll, opt=0):
    # ./wamrc --target=i386 --bounds-checks=0 --format=llvmir-opt -o test.ll test.wasm
    cmd = [
        "./bin/wamrc",
        "--target=i386",
        "--bounds-checks=0",
        "--format=llvmir-unopt",
        "-o",
        ll,
        wasm,
    ]
    return os.system(" ".join(cmd)) == 0


def run_saber(ll, result):
    #st = time.time()
    cmd = ["./bin/saber", ll, "-leak", "-stat=false", "-clock-type=wall", "2>", result]
    ret = os.system(" ".join(cmd))
    #ed = time.time()
    return ret


import time

init = time.time()
# traverse all files
cwd = os.path.dirname(os.path.realpath(__file__))
print(cwd)

data_dir = cwd + "/dataset/dataset-SAC-2022/wasm/"
out_dir = cwd + "/out"
result_dir = out_dir + "/wamr_result"
compile_dir = out_dir + "/wamr_output"

#if not exist, create
if not os.path.exists(out_dir):
    os.mkdir(out_dir)
if not os.path.exists(result_dir):
    os.mkdir(result_dir)
if not os.path.exists(compile_dir):
    os.mkdir(compile_dir)


# remove result
for root, dirs, files in os.walk(result_dir):
    for f in files:
        os.remove(os.path.join(root, f))

for root, dirs, files in os.walk(compile_dir):
    for f in files:
        os.remove(os.path.join(root, f))

for root, dirs, files in os.walk(data_dir):
    for f in files:
        now = os.path.join(root, f)
        ll = os.path.join(compile_dir, f[:-2]) + ".ll"
        getIR_WAMR(now, ll)
        result = result_dir + "/" + f[:-2] + ".out"
        run_saber(ll, result)


# 统计结果
end = time.time()
print("[+] total time: ", end - init)

print("==================Lift result===================")

lifted_count = 0
for root, dirs, files in os.walk(compile_dir):
    for f in files:
        res = open(os.path.join(root, f), "r")
        if res.read() != "":
            lifted_count += 1

print("lifted_count: ", lifted_count)


print("==================Saber result===================")
result = {}
for root, dirs, files in os.walk(result_dir):
    for f in files:
        res = open(os.path.join(root, f), "r")
        if res.read() != "":
            CWE_type = f.split("_")[0]
            result[CWE_type] = result.get(CWE_type, 0) + 1

print(result)
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

for i in result:
    if("std" in i):
        continue
    print(
        i, result[i], "/", total_CWE[i], str(result[i] / total_CWE[i] * 100)[:4] + "%"
    )
