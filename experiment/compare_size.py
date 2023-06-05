import os
import sys
import subprocess
import re
import time
import matplotlib.pyplot as plt
import numpy as np

# 生成数据

y1 = np.array([])
y2 = np.array([])
y3 = np.array([])
init = time.time()
# traverse all files
cwd = os.path.dirname(os.path.realpath(__file__))

data_dir = cwd + "/dataset/dataset-SAC-2022/wasm/"
wavm_dir = cwd + "/out/wavm_output/"
wamr_dir = cwd + "/out/wamr_output/"
notdec_dir = cwd + "/out/notdec_output/"

#get line count
def get_line(filename):
    f = open(filename, "r")
    lines = f.readlines()
    return len(lines)

    

result = {}
count = 0
for root, dirs, files in os.walk(data_dir):
    for f in files:
        llfile = f[:-2] + ".ll"
        f1 = os.path.join(wavm_dir, llfile)
        f2 = os.path.join(wamr_dir, llfile)
        f3 = os.path.join(notdec_dir, llfile)
        if os.path.exists(f1) and os.path.exists(f2) and os.path.exists(f3):
            y1 = np.append(y1, get_line(f1))
            y2 = np.append(y2, get_line(f2))
            y3 = np.append(y3, get_line(f3))
            count += 1
            print(count)
            if(count == 100):
                break
            

        

x = np.arange(0, len(y1), 1)



# 创建画布和子图
fig, ax = plt.subplots()

# 绘制折线图
ax.plot(x, y1, color="blue", label="WAVM")
ax.plot(x, y2, color="green", label="WAMR")
ax.plot(x, y3, color="red", label="notdec")

# 设置标题和坐标轴标签
ax.set_title("Line Chart")
ax.set_xlabel("testcases")
ax.set_ylabel("LOC")

# 设置图例
ax.legend()

# 保存为PNG图像
plt.savefig("line_chart.png")

# 显示图形
plt.show()
