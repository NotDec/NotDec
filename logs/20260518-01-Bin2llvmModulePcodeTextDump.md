# 2026-05-18 bin2llvm 模块 P-Code 文本导出

## 背景

Bench2 的模块级输出目录里已经有 `module-limit5.json`、`module-limit5.ll`、`module-limit5.bc` 和日志。
调试 `vsftpd/FUN_00106740` 时，需要一个文本版 P-Code，顺序尽量和当前 LLVM IR lowering 一致，方便逐块对照。

这次不改 Ghidra 导出，也不改 LLVM lowering。只从已有 heritage module JSON 生成文本。

## 已改文件

1. `scripts/bin2llvm-dump-module-pcode.py`
   - 第 12 行 `default_output_path(...)`：默认把 `*.json` 输出成同目录 `*.pcode.txt`。
   - 第 18 行 `print_op(...)`：只打印原始 P-Code `text`，末尾用 `;` 注释补 op id、parent block 和 call target。
   - 第 30 行 `iter_lowering_order(...)`：按当前 LLVM lowering 顺序输出 block 内 op：先所有 `MULTIEQUAL`，再普通 op；遇到 `BRANCH`、`CBRANCH`、`BRANCHIND`、`RETURN` 后停止。
   - 第 41 行 `dump_function(...)`：按 module JSON 中的函数顺序和 block 顺序输出。
   - 第 68 行 `dump_module(...)`：输出 module 信息、函数、externals 和 failures。
   - 第 102 行 `main(...)`：提供 CLI，支持显式 `-o`。

## 生成结果

已生成：

1. `/sn640/NotDec-Exp/Bench2/bin2llvm-ir/vsftpd/module-limit5.pcode.txt`
   - 精简后大小约 4.7M。
   - 包含 `FUN_00106740`，能直接看到 `bb:0` 里 `RSP`、`FS_OFFSET` 的输入，也能看到 `bb:1` 里 `MULTIEQUAL` 先输出。
2. `/sn640/NotDec-Exp/Bench2/bin2llvm-ir/libuv/module-limit5.pcode.txt`
   - 精简后大小约 1.8K。
   - 用作干净小样例对照。

使用方式：

```bash
scripts/bin2llvm-dump-module-pcode.py \
  /sn640/NotDec-Exp/Bench2/bin2llvm-ir/vsftpd/module-limit5.json
```

或指定输出：

```bash
scripts/bin2llvm-dump-module-pcode.py \
  /sn640/NotDec-Exp/Bench2/bin2llvm-ir/vsftpd/module-limit5.json \
  -o /tmp/vsftpd.pcode.txt
```

## 验证

执行：

```bash
chmod +x scripts/bin2llvm-dump-module-pcode.py
scripts/bin2llvm-dump-module-pcode.py /sn640/NotDec-Exp/Bench2/bin2llvm-ir/vsftpd/module-limit5.json
scripts/bin2llvm-dump-module-pcode.py /sn640/NotDec-Exp/Bench2/bin2llvm-ir/libuv/module-limit5.json
```

检查：

```bash
sed -n '1,120p' /sn640/NotDec-Exp/Bench2/bin2llvm-ir/vsftpd/module-limit5.pcode.txt
rg -n "function FUN_00106740|block bb:1|op:ram:00106783:65110|vn:210521" \
  /sn640/NotDec-Exp/Bench2/bin2llvm-ir/vsftpd/module-limit5.pcode.txt
```

结果：

1. `module-limit5.pcode.txt` 和 `module-limit5.ll` 在函数/block 层面可以按同一顺序对照。
2. `bb:1` 中 `MULTIEQUAL` 会排在普通 op 前，和 `lowerBlock(...)` 当前逻辑一致。
3. 文本里保留了原始 P-Code `text`，右侧 `;` 注释包含 op id、block 和 call target。

## 注意

1. 当前文本顺序是“LLVM lowering 顺序”，不是 Ghidra JSON 原始 op 顺序。
2. 如果 block 中 terminator 后还有 op，当前脚本会和 LLVM lowering 一样停止，不再打印后面的 op。
3. 这个脚本只读已有 JSON，不会触发 Ghidra，也不会重新 lower IR。

## 性能

这次只是 JSON 到文本的离线导出，不影响 NotDec 主 pass pipeline，也不需要跑 `fortune.o3.wasm.ll`。

`vsftpd module-limit5.json` 约 53M，精简文本约 4.7M，当前运行时间小于 1 秒。

## 评分

实现效果：8/10。能把 P-Code 和 LLVM IR 放在同一目录，并按当前 lowering 顺序对照。

复杂度：2/10。只是 JSON 格式化，没有改核心链路。

维护成本：3/10。脚本依赖 heritage module JSON 字段；如果后续 lowering 顺序变化，需要同步更新 `iter_lowering_order(...)`。
