# MLsub 阶段 B 输入校验与诊断收紧

日期：2026-04-17

## 1. 本次范围

沿着 [logs/20260417-09-MLsubStageBRemainingPlan.md](/sn640/NotDec/logs/20260417-09-MLsubStageBRemainingPlan.md)
里的第三步，先把阶段 B 的入口校验和用户可读诊断补完整。

本次目标：

1. 启用 `NOTDEC_EXTRA_CONSTRAINTS` 时，只接受 `.ll/.bc`
2. 错误提示直接说明：
   - 当前输入后缀
   - 为什么不允许
   - 推荐的两阶段命令
3. 对“输入已经是 `.ll/.bc`，但没有显式阶段 B 标记”的情况，也给出更明确的诊断

当前仍未尝试解决的点：

1. 单靠输入文件本身还无法百分之百证明它真的是阶段 A 导出的冻结 IR
2. 这部分仍继续由后面的 `ir_anchor` 校验兜底

## 2. 代码修改

### 2.1 抽出阶段 B 提示文案 helper

文件：

- [src/NotDec.cpp:149](/sn640/NotDec/src/NotDec.cpp#L149)
  到 [src/NotDec.cpp:163](/sn640/NotDec/src/NotDec.cpp#L149)

涉及函数：

1. `describeInputSuffix()`
2. `printFrozenTRInputWorkflowHint()`

修改内容：

1. 新增 `describeInputSuffix()`：
   - 把空后缀打印成 `<no extension>`
2. 新增 `printFrozenTRInputWorkflowHint()`：
   - 统一输出两阶段工作流提示
   - 直接给出阶段 A / 阶段 B 示例命令

### 2.2 `main()` 中补阶段 B 专用输入校验

文件：

- [src/NotDec.cpp:167](/sn640/NotDec/src/NotDec.cpp#L167)
  到 [src/NotDec.cpp:220](/sn640/NotDec/src/NotDec.cpp#L167)

涉及函数：

- `main()`

修改内容：

1. 在组合校验前更早拿到：
   - `insuffix`
   - `InputSuffixDesc`
2. `NOTDEC_EXTRA_CONSTRAINTS + --emit-tr-input-ir` 现在会报：
   - 阶段 A / 阶段 B 不能混在一条命令里
   - 并给出两阶段 workflow hint
3. `NOTDEC_EXTRA_CONSTRAINTS + 非 .ll/.bc 输入` 现在会报：
   - 当前输入路径
   - 当前后缀
   - 为什么 extra constraints 只能绑定冻结 LLVM IR
   - 两阶段 workflow hint
4. `NOTDEC_EXTRA_CONSTRAINTS + .ll/.bc 但缺少 --frozen-tr-input-ir`
   现在会报：
   - 即使输入已经是 `.ll/.bc`，也仍需要显式阶段 B 标记
   - 原因是该标记负责保证不再重跑 pre-TR pipeline
   - 两阶段 workflow hint
5. `--frozen-tr-input-ir + 非 .ll/.bc 输入`
   的原有报错也升级为：
   - 带当前输入路径和后缀
   - 带“阶段 B 运行在冻结 LLVM IR 上”的原因说明
   - 带 workflow hint

### 2.3 文档同步

文件：

- [DEBUG.md:30](/sn640/NotDec/DEBUG.md#L30)
  到 [DEBUG.md:35](/sn640/NotDec/DEBUG.md#L30)

修改内容：

1. 明确补充：
   - `--frozen-tr-input-ir` 只接受 `.ll/.bc`

## 3. 验证

### 3.1 编译

执行：

```bash
cmake --build ./build --target notdec-decompile
```

结果：通过。

### 3.2 `.ll` 输入但缺少阶段 B 显式标记

执行：

```bash
NOTDEC_EXTRA_CONSTRAINTS=/tmp/dummy.json \
./build/bin/notdec test/type-recovery/llvm-ir/cases/01_Simple1.ll \
  --tr-level=2 -o /tmp/out.ll
```

结果：

1. 直接报：
   - `requires --frozen-tr-input-ir even when the input is .ll`
2. 同时解释：
   - 该标记用于保证不再重跑 pre-type-recovery pipeline
3. 并打印阶段 A / 阶段 B 示例命令

### 3.3 非 `.ll/.bc` 输入误进阶段 B

执行：

```bash
NOTDEC_EXTRA_CONSTRAINTS=/tmp/dummy.json \
./build/bin/notdec test/type-recovery/llvm-ir/cases/offset1.wat \
  --tr-level=2 --frozen-tr-input-ir -o /tmp/out.ll
```

结果：

1. 直接报：
   - 当前输入是 `.wat`
   - `NOTDEC_EXTRA_CONSTRAINTS` 只支持冻结 `.ll/.bc`
2. 同时解释：
   - selector / `ir_anchor` 绑定的是冻结 LLVM IR，而不是原始 frontend 输入
3. 并打印阶段 A / 阶段 B 示例命令

### 3.4 正确的 frozen `.ll` 路径仍能继续进入后续校验

执行：

```bash
NOTDEC_EXTRA_CONSTRAINTS=/tmp/dummy.json \
./build/bin/notdec /tmp/fortune-stagea.ll \
  --tr-level=2 --frozen-tr-input-ir -o /tmp/stageb-ok.ll
```

结果：

1. 成功越过新的入口校验
2. 继续进入后续 `MLsub` extra-constraints 解析
3. 最终因为 `/tmp/dummy.json` 不是合法 JSON 而在旧的解析校验点失败

这说明：

1. 本次收紧的是入口语义和报错
2. 没有误伤后面的 anchor / JSON 处理链路

## 4. 当前判断

这一步之后，阶段 B 至少在 CLI 层面已经更像一个正式入口：

1. 输入类型不对会被更早拒绝
2. 少显式标记时会说明原因
3. 报错里已经直接带了推荐的两阶段命令

下一步更自然的工作就是：

1. 补一条正式的两阶段端到端回归
2. 再考虑 howard `fortune` 的 manifest 提升
