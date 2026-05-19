# 2026-05-19 bin2llvm 动态库模块级重跑结果

## 用户原始 prompt

```text
尝试验证一下这个猜想，当前链路对于动态链接库的转IR可能更好更有效。但是对于这种独立二进制，可能会遇到一些底层一些的函数。现在先把那几个真实项目，如果是动态链接库的，尝试进一步分析更多函数，或者直接转出整个模块的IR，二进制的先放一放，看看是不是问题更少一些。
```

## 目标

这次只验证一个猜想：

1. 动态链接库的模块级导出和 lowering，可能比独立二进制更稳。
2. 如果成立，就优先把库样本往更大范围推，先把二进制样本放一放。

这次不改代码，只跑现有链路。

## 跑批范围

这次选了三类已经在 Bench2 里有的动态库：

1. `libuv.so.1.0.0`
2. `libicui18n.so.74.2`
3. `sockets.so`

其中：

- `libuv` 直接跑全量 `--all`
- `sockets` 直接跑全量 `--all`
- `libicui18n` 先跑 `--limit=20`

另外我也开始试了 `libwolfssl.so.42.0.0 --all`，但这个样本太大，当前还没收尾，所以这篇日志先不把它算进结论。

## 结果

### `libuv --all`

输入：

```text
/sn640/NotDec-Exp/Bench2/rootfs/usr/lib/x86_64-linux-gnu/libuv.so.1.0.0
```

结果：

1. Ghidra 导出成功。
2. attempted functions: `478`
3. succeeded functions: `478`
4. failed functions: `0`
5. `notdec-heritage-module-check` 通过。
6. `notdec-heritage-module-llvm` lowered `478` 个 body，failed `0` 个。
7. `llvm-as` 通过。
8. check 时间：`real 12.84s`
9. lowering 时间：`real 14.65s`

lower log 里有少量 warning：

- 少数参数没有 `varnode`
- 少数 `RETURN` 没有 value input

但这些都没有把函数体打断。

### `sockets --all`

输入：

```text
/sn640/NotDec-Exp/Bench2/rootfs/usr/lib/php/20230831/sockets.so
```

结果：

1. Ghidra 导出成功。
2. attempted functions: `192`
3. succeeded functions: `192`
4. failed functions: `0`
5. `notdec-heritage-module-check` 通过。
6. `notdec-heritage-module-llvm` lowered `192` 个 body，failed `0` 个。
7. `llvm-as` 通过。
8. check 时间：`real 12.46s`
9. lowering 时间：`real 13.20s`

lower log 里同样只有 warning，没有失败。

### `libicui18n --limit=20`

输入：

```text
/sn640/NotDec-Exp/Bench2/rootfs/usr/lib/x86_64-linux-gnu/libicui18n.so.74.2
```

结果：

1. Ghidra 导出成功。
2. attempted functions: `20`
3. succeeded functions: `20`
4. failed functions: `0`
5. `notdec-heritage-module-check` 通过。
6. `notdec-heritage-module-llvm` lowered `20` 个 body，failed `0` 个。
7. `llvm-as` 通过。
8. check 时间：`real 0.05s`
9. lowering 时间：`real 0.07s`

这组里也只有 warning，没有失败。

## 观察

### 1. 动态库样本整体确实更稳

这次跑出来的三个动态库样本里：

1. 没有像独立二进制那样出现 `parameter ... has no varnode` 直接打断函数体的情况。
2. 没有出现 entry block / PHI verifier 失败。
3. 没有出现 return type mismatch 这类 body 级失败。
4. `llvm-as` 都通过了。

和前面几个独立二进制样本比，动态库这边更接近“可以直接把整模块跑完”的状态。

### 2. warning 还在，但不是当前阻断点

动态库里还能看到几类 warning：

- `RETURN` 没有 value input
- 少数参数没有 `varnode`

但这些 warning 现在已经不会把整个函数体打断。

### 3. 这不代表二进制问题消失了

独立二进制里出现的几类问题，尤其是：

- CFG / PHI 入口别名
- 函数返回类型不一致
- 参数缺 varnode 导致的体级失败

在库样本里明显少得多。

这更像是：

1. 动态库本来就更少碰到那种“入口很底层、控制流很碎、局部语义被打散”的函数。
2. 当前链路对库样本的函数签名、寄存器状态和模块结构更容易对齐。

## 判断

当前这个猜想可以先算“基本成立”：

1. 动态库样本的模块级导出和 lowering，确实比几个独立二进制样本更稳。
2. 对于这批库样本，当前链路已经能直接把整模块跑完，至少在 `libuv`、`sockets`、`libicui18n` 上是这样。
3. 后续如果继续扩范围，优先顺序应该还是动态库。

## 风险

1. `libwolfssl --all` 这类更大的库还没收尾，不能拿来当最终结论。
2. 动态库更稳，不代表所有库都稳。像 C++ 体量大的库，还是可能有大量构造器、RTTI、TLS 相关的边角问题。
3. 这次只验证了模块级导出和 lowering，没检查语义是否完全对，只能说明“更容易跑通”。

