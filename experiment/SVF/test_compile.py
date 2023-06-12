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

def do_compile_ll(src, out):
    command = ["clang-13", "-O0 -S -emit-llvm", "-g0", src, "-o", out]
    print(" ".join(command))
    return os.system(" ".join(command)) == 0


def run_saber(ll, result):
    #st = time.time()
    cmd = [bin_dir + "/saber", ll, "-leak", "-stat=false", "-clock-type=wall", "2>", result]
    ret = os.system(" ".join(cmd))
    #ed = time.time()
    return ret


init = time.time()

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

# compile
for root, dirs, files in os.walk(data_dir):
    for f in files:
        now = os.path.join(root, f)
        ll = os.path.join(compile_dir, f[:-2]) + ".ll"
        do_compile_ll(now, ll)
        result = result_dir + "/" + f[:-2] + ".out"
        run_saber(ll, result)

# 统计结果
end = time.time()
print("[+] total time: ", end - init)
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
    
