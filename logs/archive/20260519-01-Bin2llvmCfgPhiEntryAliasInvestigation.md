# 2026-05-19 bin2llvm CFG / PHI entry 地址重复问题调研

## 用户原始 prompt

```text
调研一下CFG / PHI的问题，单独写一个logs/的文档
```

## 结论

`memcached --limit=20` 剩下的两个 CFG / PHI 失败，根因不是 PHI incoming 顺序本身，而是 block 地址反查选错了 block。

Ghidra 导出的失败函数里有两个 block 使用同一个函数入口地址：

1. `bb:0`：空入口跳板，没有 op，只跳到 `bb:1`。
2. `bb:1`：真实入口代码块，里面有 `MULTIEQUAL`，并且有循环回边。

native loader 当前用：

```cpp
program.BlockByStart.emplace(block.Start, &block);
```

`emplace` 遇到重复 key 会保留第一个 block，所以入口地址会映射到空的 `bb:0`。后面 lowering 遇到 `BRANCH` / `CBRANCH` 目标地址等于函数入口地址时，会跳到 `bb:0`，而不是应该跳到 `bb:1`。

结果：

1. LLVM `entry` block 被回边跳入，触发 verifier：

```text
Entry block to function must not have predecessors!
```

2. `bb:1` 的 PHI 仍按 Ghidra CFG 记录了来自回边 block 的 incoming，但实际 LLVM 分支已经跳到 `entry`，所以触发：

```text
PHINode should have one entry for each predecessor of its parent basic block!
```

## 复现样例

输入 JSON：

```text
/tmp/notdec-bench2-rerun-20260518/memcached20/module-limit20.json
```

验证命令：

```bash
/tmp/notdec-bin2llvm-build/bin/notdec-heritage-module-llvm \
  /tmp/notdec-bench2-rerun-20260518/memcached20/module-limit20.json \
  -o /tmp/notdec-return-fix-regress/memcached20/out.ll
```

当前剩余失败：

```text
failure: FUN_001066f0 ram:001066f0: Entry block to function must not have predecessors!
label %entry
PHINode should have one entry for each predecessor of its parent basic block!
  %"vn:3650" = phi i64 [ %"vn:3656", %"bb:6" ], [ %"vn:3741.mem", %entry ], !notdec.indirect !12

failure: FUN_00106740 ram:00106740: Entry block to function must not have predecessors!
label %entry
PHINode should have one entry for each predecessor of its parent basic block!
  %"vn:14210" = phi i64 [ %"vn:14207", %"bb:1" ], [ %"vn:14213", %"bb:65" ], !notdec.indirect !31
```

## 证据 1：重复入口地址

用 `jq` 查重复 `start`：

```bash
jq -r '
  .functions[]
  | select(.name=="FUN_001066f0" or .name=="FUN_00106740")
  | .name as $fn
  | [.blocks[].start]
  | group_by(.)[] | select(length>1)
  | [$fn,.[0],length] | @tsv
' /tmp/notdec-bench2-rerun-20260518/memcached20/module-limit20.json
```

结果：

```text
FUN_001066f0    ram:001066f0    2
FUN_00106740    ram:00106740    2
```

具体 block：

```text
FUN_001066f0:
  bb:0 start=ram:001066f0 in=[] out=[bb:1] ops=[]
  bb:1 start=ram:001066f0 in=[bb:6,bb:0] out=[bb:2,bb:8]

FUN_00106740:
  bb:0 start=ram:00106740 in=[] out=[bb:1] ops=[]
  bb:1 start=ram:00106740 in=[bb:65,bb:0] out=[bb:2]
```

## 证据 2：回边目标是函数入口地址

`FUN_001066f0`：

```text
bb:6 in=[bb:7,bb:2] out=[bb:7,bb:1]
op:ram:0010c77d:270:
  CBRANCH (ram, 0x1066f0, 1), ...
```

这里目标地址 `ram:001066f0` 应该对应 `bb:1`，因为 Ghidra CFG 的 `bb:6.out` 明确包含 `bb:1`。

`FUN_00106740`：

```text
bb:65 in=[bb:63,bb:64] out=[bb:1,bb:2]
op:ram:0010e620:458:
  CBRANCH (ram, 0x106740, 1), ...
```

这里目标地址 `ram:00106740` 也应该对应 `bb:1`。

## 代码位置

### loader 建索引

文件：

```text
external/NotDec-bin2llvm/lib/HeritagePcode.cpp
```

函数：

```cpp
void indexHeritageProgram(HeritageProgram &program)
```

当前逻辑：

```cpp
for (const HeritageBlock &block : program.Blocks) {
  program.BlockById.emplace(block.Id, &block);
  program.BlockByStart.emplace(block.Start, &block);
}
```

问题：重复 `Start` 时保留第一个 block。对这两个失败函数，第一个是空 `bb:0`。

### branch lowering

文件：

```text
external/NotDec-bin2llvm/lib/HeritageToLLVM.cpp
```

相关函数：

1. `lowerBranch(...)`
2. `finalizePendingPhis(...)`

`CBRANCH` 当前先用 `Program.BlockByStart.find(target->Address)` 找 true block。重复地址时，这里会拿到 `bb:0`。

PHI incoming 则来自 `block->In`：

```cpp
PendingPhis.push_back(PendingPhi{phi, block->In, op.Inputs, output->Size});
```

所以 PHI 仍认为 incoming 是 `bb:6 -> bb:1` 或 `bb:65 -> bb:1`，但实际 branch 被 lower 成 `bb:6 -> bb:0` / `bb:65 -> bb:0`。

## 判断

这类问题应该在地址解析层修，不应该靠 PHI 兜底。

更合理的 branch target 解析顺序：

1. 如果目标地址在当前 block 的 `Out` 里能唯一匹配，优先从 `block.Out` 选对应 block。
2. 处理重复 start 时，不要盲目用 `BlockByStart` 的第一个结果。
3. 空入口跳板 `bb:0` 只应该用于函数外部入口，不应该成为函数内部回边目标。

## 修复方向

短期建议：

1. 增加一个 `resolveSuccessorByAddress(block, targetAddress)`。
2. 在 `block.Out` 的候选 successor 里查 `Start == targetAddress`。
3. 如果有多个候选，优先选择非空 block。
4. 如果没有候选，再退回全局 `BlockByStart`。

这样能避免函数内部回边跳到空入口跳板，也能保持普通 branch 行为不变。

还需要注意：

1. `BlockByStart` 以后最好改成 `Start -> vector<Block*>`，否则重复地址长期还是隐患。
2. `CBRANCH` 的 false block 也应该从 `block.Out` 中排除 true block 后得到。true block 选错时 false block 也会连带选错。
3. 修完后验证 `memcached --limit=20` 是否从 18/20 body 成功变为 20/20 body 成功。

## 风险

1. Ghidra 可能在别的函数里也导出重复 start，不一定只发生在函数入口。
2. 同一个地址多个 block 可能代表不同 p-code split，不一定总能靠“非空 block”解决。
3. 如果 `block.Out` 和 branch target 地址冲突，应该打印具体 error，不要静默猜。

## 2026-05-19 实施记录：修复重复 start 的 branch 解析

本次按上面的短期方案实现，没有改 PHI lowering。

修改文件：

1. `external/NotDec-bin2llvm/include/notdec-bin2llvm/HeritagePcode.h`
   - 第 81 行把 `BlockByStart` 从单个 `HeritageBlock*` 改为 `Start -> vector<HeritageBlock*>`。
2. `external/NotDec-bin2llvm/lib/HeritagePcode.cpp`
   - 第 320 行改为 `program.BlockByStart[block.Start].push_back(&block)`，保留重复地址的所有 block。
3. `external/NotDec-bin2llvm/lib/HeritageToLLVM.cpp`
   - 第 1523 行新增 `chooseBlockByAddress(...)`：重复地址里优先选唯一的非空 block；多个非空时直接报 ambiguous。
   - 第 1547 行新增 `resolveSuccessorByAddress(...)`：先在当前 `block.Out` 里按目标地址找 successor，再退回全局 `BlockByStart`。
   - 第 1584 行修改 `CBRANCH`，true block 使用新 resolver；false block 从 `block.Out` 中排除 true block 后得到。
   - 第 1629 行修改 `BRANCH`，也使用新 resolver。

实现判断：

1. 直接把 `BlockByStart` 改成 vector 是有必要的。否则重复地址的信息在 loader 阶段就丢了，后面无法判断该选空入口跳板还是真实代码块。
2. `CBRANCH` 不能只靠全局地址表选 true block。当前 block 的 `Out` 是更强的 CFG 信息，应该优先用它。
3. 这次没有给 PHI 做特殊兜底，因为 PHI 的 `block.In` 本来是对的，错的是 branch target 被解析到了别的 block。

验证命令：

```bash
cmake --build /tmp/notdec-bin2llvm-build \
  --target notdec-heritage-module-check notdec-heritage-module-llvm notdec-heritage-llvm -j4

/tmp/notdec-bin2llvm-build/bin/notdec-heritage-module-check \
  /tmp/notdec-bench2-rerun-20260518/memcached20/module-limit20.json

/tmp/notdec-bin2llvm-build/bin/notdec-heritage-module-llvm \
  /tmp/notdec-bench2-rerun-20260518/memcached20/module-limit20.json \
  -o /tmp/notdec-cfg-phi-fix/memcached20/out.ll

/sn640/NotDec/llvm-22.1.0.obj/bin/llvm-as \
  /tmp/notdec-cfg-phi-fix/memcached20/out.ll \
  -o /tmp/notdec-cfg-phi-fix/memcached20/out.bc
```

回归也跑了：

```bash
/tmp/notdec-bin2llvm-build/bin/notdec-heritage-module-llvm \
  /tmp/notdec-bench2-rerun-20260518/vsftpd10/module-limit10.json \
  -o /tmp/notdec-cfg-phi-fix/vsftpd10/out.ll

/tmp/notdec-bin2llvm-build/bin/notdec-heritage-module-llvm \
  /tmp/notdec-bench2-rerun-20260518/libuv20/module-limit20.json \
  -o /tmp/notdec-cfg-phi-fix/libuv20/out.ll

/tmp/notdec-bin2llvm-build/bin/notdec-heritage-llvm \
  /sn640/NotDec-Exp/Bench2/bin2llvm-ir/python/one-_PyPegen_fill_token.cold.json \
  -o /tmp/notdec-cfg-phi-fix/python/out.ll

/tmp/notdec-bin2llvm-build/bin/notdec-heritage-llvm \
  /sn640/NotDec-Exp/Bench2/bin2llvm-ir/wrk/one-parse_url_char.cold.json \
  -o /tmp/notdec-cfg-phi-fix/wrk/out.ll
```

验证结果：

1. `memcached --limit=20` checker 通过。
2. `memcached --limit=20` lowering 从 18/20 body 成功变为 20/20 body 成功，`llvm-as` 通过，lowering `real 1.72s`。
3. `vsftpd --limit=10` 仍是 10/10 body 成功，`llvm-as` 通过，lowering `real 14.44s`。
4. `libuv --limit=20` 仍是 20/20 body 成功，`llvm-as` 通过，lowering `real 0.03s`。
5. `python/one-_PyPegen_fill_token.cold` 和 `wrk/one-parse_url_char.cold` 仍通过 `llvm-as`。

性能：

1. `memcached --limit=20`：`real 1.72s`，和上一轮 `real 1.77s` 基本一致。
2. `vsftpd --limit=10`：`real 14.44s`，和上一轮 `real 14.70s` 基本一致。
3. 这次只改 bin2llvm branch 解析，不接 NotDec 主 pass pipeline，不跑 `fortune.o3.wasm.ll`。

剩余风险：

1. 如果同一个 `block.Out` 里有多个非空 block 使用同一个 start，当前会报 ambiguous；这是有意的，避免静默猜错 CFG。
2. 如果 Ghidra 的 `block.Out` 本身不可信，仍然可能选错。但比全局 `BlockByStart` 更贴近当前 branch 语义。

评分：

1. 实现效果：9/10。直接解决当前 `memcached` 剩余 verifier 失败。
2. 复杂度：4/10。引入了 `Start -> vector<Block*>` 和一个解析 helper，但影响面集中。
3. 维护成本：4/10。后续如果发现更多重复 start 形态，可以扩展 `chooseBlockByAddress(...)`，不用改 PHI。
