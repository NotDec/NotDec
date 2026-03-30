# rr 调试速查

这份笔记只整理 `rr` 的使用方式和外部资料，不涉及当前仓库本身。

## 1. rr 是什么

`rr` 是 Linux 上的 record/replay 调试器：先把一次运行录下来，再用 `gdb`（以及部分场景下的 `lldb`）对这次运行做可重复、可倒放的调试。核心思路是：

1. 用 `rr record ...` 录制一次程序执行
2. 用 `rr replay` 进入调试
3. 在 replay 里正常打断点、单步、继续执行
4. 必要时反向执行，或借助硬件 watchpoint 倒查写入点

## 2. 基本工作流

最小工作流：

```bash
rr record /path/to/program arg1 arg2
rr replay
```

也可以省略 `record` 这个词：

```bash
rr /path/to/program arg1 arg2
```

回放最近一次 trace：

```bash
rr replay
```

回放指定 trace：

```bash
rr replay /path/to/trace-dir
```

## 3. 最常用的调试姿势

### 3.1 先录制，再回放

如果程序能稳定复现：

```bash
rr record ./your_program --flag
rr replay
```

进入 `gdb` 后就和普通调试很像：

```gdb
break main
run
next
step
continue
bt
frame 3
print some_var
```

但和普通 `gdb` 最大的不同是：`run` 会重新从同一份录制数据开头开始回放，而且每次路径都一致。这意味着：

- 断点会保留
- 地址和执行路径通常保持一致
- 你可以多次尝试不同观察方法，而不用重新复现 bug

### 3.2 从崩溃点直接开始

如果 trace 很长，但你只想从“程序退出/崩溃前”开始调：

```bash
rr replay -e
```

然后在 `gdb` 里往回走。

### 3.3 反向执行

`rr` 最有价值的一组命令通常是：

```gdb
reverse-continue
reverse-step
reverse-next
reverse-finish
```

常见缩写：

```gdb
rc
rs
rn
rf
```

典型用法：

1. 在错误状态处停下
2. `print` 看见某个值已经坏了
3. 对这个地址/变量下 watchpoint
4. `reverse-continue` 倒退到最后一次写入它的位置

### 3.4 用 watchpoint 倒查“谁改坏了变量”

这是 `rr` 最经典的用法：

```gdb
watch -l my_var
reverse-continue
```

注意这里推荐 `watch -l`，而不是裸 `watch my_var`。`-l` 会用 location watchpoint，通常更稳定，反向调试时也更靠谱。

很适合排查：

- 某个字段何时被写坏
- 容器大小何时变错
- 指针何时被改成野指针

## 4. 精确定位到某个进程或事件

### 4.1 调试子进程

`rr` 会把 fork/exec 出来的进程一起录下来。查看 trace 里的进程：

```bash
rr ps
```

在某个进程第一次 `exec` 后接入调试：

```bash
rr replay -p <pid>
```

在某个进程 `fork` 后立刻接入：

```bash
rr replay -f <pid>
```

也可以用可执行文件名：

```bash
rr replay -p my_binary_name
```

### 4.2 用事件号跳到问题附近

`rr` 在录制过程中维护 event number。回放时可以让标准输出附带事件号：

```bash
rr replay -M
```

如果你想在某个事件号处直接进入调试：

```bash
rr replay -g <event>
```

在 `gdb` 里还能查看当前事件号：

```gdb
when
```

以及从指定事件重新开始：

```gdb
run 32818
```

这对“日志里看到异常发生在某个 event 附近”非常有用。

## 5. Trace 管理

默认 trace 会存到 rr 的 trace 目录里，常见位置是：

- `$HOME/.local/share/rr`
- 老版本兼容场景下可能是 `$HOME/.rr`

相关环境变量：

```bash
export _RR_TRACE_DIR=/path/to/store/traces
export RR_TMPDIR=/path/to/tmp
```

注意：

- `RR_TMPDIR`/`TMPDIR` 需要在一个可执行、空间足够的文件系统上
- 如果系统把 `/tmp` 挂成了 `noexec`，需要换一个目录

如果你希望 trace 能脱离本机原始二进制继续使用，可以考虑：

```bash
rr pack
```

`pack` 会把依赖文件打进 trace，便于迁移或归档；代价是 trace 会更大。

## 6. rr + IDE

`rr` 本质上能以 `gdb`/`gdbserver` 形式工作，所以可以接 IDE。

常见方式：

- VS Code: `rr replay -s 50505 -k` 后，让 IDE 连接到这个 gdb server
- VS Code + Midas: 社区里有专门为 RR 优化的扩展
- gdbgui: `gdbgui --gdb-cmd "rr replay --"`
- emacs/NeoVim/QtCreator/Eclipse/Seer: 官方 wiki 都给了接入思路

如果你只是先上手，建议先用纯命令行 `rr + gdb`，把核心工作流跑通，再接 IDE。

## 7. 使用时的重要限制和坑

### 7.1 只适合 Linux，且对硬件/内核有要求

官方仓库当前说明里，`rr` 主要要求：

- Linux kernel >= 4.7
- CPU 需要支持 rr 依赖的性能计数器/微架构特性
- Intel Nehalem 及之后一般更常见
- 某些 AMD Zen 及 AArch64 也支持，但要看具体型号

虚拟机里也不是完全不行，但前提是虚拟机要正确虚拟化硬件性能计数器。

### 7.2 先确认你的机器是否支持 rr

官方 wiki 建议用 `perf stat` 检查关键 counter 是否可用，例如：

```bash
perf stat -e br_inst_retired.conditional true
```

如果是 AMD Ryzen，可能是：

```bash
perf stat -e ex_ret_cond true
```

如果 counter 不可用，`rr` 往往没法正常工作。

### 7.3 可能需要调整 `perf_event_paranoid`

官方 wiki 提到录制时通常至少需要把：

```bash
kernel.perf_event_paranoid=1
```

否则 `rr record` 可能直接失败或权限不足。

### 7.4 二进制不要在录制后被改掉

录制后的可执行文件、库文件如果变化，回放很容易出问题。官方明确建议在这种场景下用 `rr pack` 把依赖打进去。

### 7.5 rr 不适合拿来安全地运行恶意代码

官方 usage 文档明确提醒：`rr` 不是面向恶意代码隔离设计的。尤其录制依赖 `seccomp`/namespace 沙箱的程序时，`rr` 为了录制会主动打洞，不应把它当成安全沙箱。

### 7.6 多线程程序会变慢

`rr` 为了保证可重放性，会把 tracee 的线程调度到单核语义附近，多线程程序的实际执行速度可能明显变慢。官方文档给的经验值大致是：

- 轻量 CPU bound 开销大约 1.1x 到 1.2x
- 更常见情况约 1.2x 到 1.4x
- syscall 很密集时可能更高

## 8. 一个推荐排障模板

如果你遇到“结果错了，但不知道哪里开始错”的 bug，可以直接按这个套路：

```bash
rr record ./app ...
rr replay -e
```

进 `gdb` 后：

```gdb
bt
frame N
print suspicious_var
watch -l suspicious_var
reverse-continue
```

如果变量是结构体成员：

```gdb
watch -l obj->field
reverse-continue
```

如果问题出在某个子进程：

```bash
rr ps
rr replay -p <pid>
```

如果你已经从日志知道大概事件号：

```bash
rr replay -M -g <event>
```

## 9. 一个推荐学习顺序

建议按下面顺序熟悉：

1. 跑通 `rr record` / `rr replay`
2. 学会在 replay 里正常打断点和 `run`
3. 学会 `reverse-continue` / `reverse-step`
4. 学会 `watch -l` + 反向执行
5. 学会 `-p` / `-f` 调子进程
6. 学会 `-M`、`-g`、`when` 用 event 精确定位
7. 需要图形界面时再接 IDE

## 10. 相关资源

优先看这些：

1. 官方站点: <https://rr-project.org/>
2. 官方仓库 README: <https://github.com/rr-debugger/rr>
3. 官方 Usage wiki: <https://github.com/rr-debugger/rr/wiki/Usage>
4. wiki 镜像（更容易直接阅读）: <https://github-wiki-see.page/m/rr-debugger/rr/wiki/Usage>
5. 机器兼容性检查: <https://github.com/rr-debugger/rr/wiki/Will-rr-work-on-my-system>
6. IDE 接入: <https://github.com/rr-debugger/rr/wiki/Using-rr-in-an-IDE>
7. 发行说明（可查看新能力，如 `lldb` 支持）: <https://github.com/rr-debugger/rr/releases>

## 11. 我对 rr 的实际建议

如果你第一次用 `rr`，不要一开始就追求 IDE 集成。最值得先练熟的是三件事：

- `rr record` / `rr replay`
- `reverse-continue`
- `watch -l`

只要这三件事顺手了，`rr` 对排查“偶发错误”和“变量被谁改坏”这两类问题的收益就已经很高了。
