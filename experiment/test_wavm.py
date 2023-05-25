import os
import sys
import subprocess
import re
import time

def wavm_fix_ir(ll_file):
    ir = open(ll_file).read()

    # remove llvm ir dbg info
    p = re.compile(r"[ ,]*!dbg !\d+")
    ir = p.sub("", ir)
    ir = ir[: ir.index("!llvm.dbg.cu")]

    # args rename
    ar = re.compile(r"\(.*\) #")

    def rename_arg(match) -> str:
        s = match.group()[:-3].split(",")
        d = ""
        for idx, v in enumerate(s):
            d += v + f" %a{idx},"
        return d[:-1] + ") #"

    ir = ar.sub(rename_arg, ir)

    # rename label
    def rename(match) -> str:
        s = match.group()
        return "%a" + s[1:]

    n = re.compile(r"%\d+")
    ir = n.sub(rename, ir)

    open(ll_file, "w").write(ir)


def do_compile_ll(src, out):
    command = ["clang-13", "-O0 -S -emit-llvm", "-g0", src, "-o", out]
    print(" ".join(command))
    return os.system(" ".join(command)) == 0


def do_decompile(wasm):
    command = [
        f"{cwd}/../../build/bin/WD40",
        "-i",
        data_dir + wasm,
        "-o",
        out_dir + wasm[:-5] + ".ll",
        "--test-mode",
        "--recompile",
    ]
    print(" ".join(command))
    return os.system(" ".join(command)) == 0


def getIR_WAVM(wasm, ll, isOPT=True):
    # f"wavm compile --format=optimized-llvmir {filename} {ll_file} --target-cpu generic")
    if isOPT:
        cmd = [
            "./wavm",
            "compile",
            "--format=optimized-llvmir",
            wasm,
            ll,
            "--target-cpu",
            "skylake",
        ]
    else:
        cmd = [
            "./wavm",
            "compile",
            "unoptimized-llvmir",
            wasm,
            ll,
            "--target-cpu",
            "skylake",
        ]
    return os.system(" ".join(cmd)) == 0


def run_saber(ll, result):
    st = time.time()
    cmd = ["./saber", ll, "-leak", "-stat=false", "-clock-type=wall", "2>", result]
    ret = os.system(" ".join(cmd))
    ed = time.time()
    return ed - st




init = time.time()
# traverse all files
cwd = os.path.dirname(os.path.realpath(__file__))
print(cwd)

data_dir = cwd + "/dataset/dataset-SAC-2022/wasm/"
out_dir = cwd + "/out"
result_dir = out_dir + "/wavm_result"
compile_dir = out_dir + "/wavm_output"


# remove result
for root, dirs, files in os.walk(result_dir):
    for f in files:
        os.remove(os.path.join(root, f))
    for d in dirs:
        os.rmdir(os.path.join(root, d))

for root, dirs, files in os.walk(compile_dir):
    for f in files:
        os.remove(os.path.join(root, f))
    for d in dirs:
        os.rmdir(os.path.join(root, d))


# compile
# for root, dirs, files in os.walk(data_dir):
# 	for f in files:
# 		now = os.path.join(root, f)
# 		ll =  os.path.join(compile_dir,f[:-2]) +  ".ll"
# 		do_compile_ll(now,ll)
# 		result = result_dir + "/" + f[:-2] + ".out"
# 		run_saber(ll,result)

for root, dirs, files in os.walk(data_dir):
    for f in files:
        now = os.path.join(root, f)
        ll = os.path.join(compile_dir, f[:-2]) + ".ll"
        getIR_WAVM(now, ll)
        wavm_fix_ir(ll)
        result = result_dir + "/" + f[:-2] + ".out"
        run_saber(ll, result)

# 统计结果

end = time.time()
print("[+] total time: ", end - init)

result = {}

for root, dirs, files in os.walk(result_dir):
    for f in files:
        res = open(os.path.join(root, f), "r")
        if res.read() != "":
            CWE_type = f.split("_")[0]
            result[CWE_type] = result.get(CWE_type, 0) + 1

print(result)
total_CWE = {'CWE122': 780, 'CWE127': 226, 'CWE121': 1605, 'CWE124': 401, 'CWE400': 104, 'CWE617': 59, 'CWE690': 103, 'CWE194': 173, 'CWE253': 123, 'CWE134': 175, 'CWE457': 120, 'CWE590': 288, 'CWE761': 91, 'CWE415': 12, 'CWE126': 299, 'CWE416': 17, 'CWE665': 55, 'CWE476': 24, 'CWE789': 50, 'CWE401': 120, 'CWE252': 70, 'CWE758': 66, 'CWE475': 18, 'CWE391': 17, 'CWE675': 46, 'CWE404': 23, 'CWE390': 35, 'CWE688': 15, 'CWE587': 17, 'CWE195': 25, 'CWE190': 8, 'CWE526': 17, 'CWE123': 21, 'CWE511': 17, 'CWE197': 2, 'CWE685': 14, 'CWE674': 1, 'CWE191': 3, 'CWE483': 1}
print("=====================================")
for i in result:
    print(i,result[i],"/",total_CWE[i] ,str(result[i]/total_CWE[i] *100)[:4] + "%")