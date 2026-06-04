# EVM 类型恢复下一步计划

## 原始 prompt

接下来该做什么，规划一下

把当前的这个规划写到一个新的logs/下的文件

## 背景

当前 EVM 类型恢复已经接入 MLsub 主流程，普通 LLVM 操作、native load/store、allocation helper
和 private helper 多返回都已经能进入同一套约束生成和 HType 输出。

最近完成的关键点：

- EVM private 多返回不再跳过 aggregate 本体，而是在 binarysub function result 位置用
  `record{"0": ..., "1": ...}` 表示 tuple，HType 转换时拆成多个返回值。
- LLVM IR 仍保留 `{ i256, i256 }` 这类 aggregate Value，不改底层 IR 形状。
- `SubNodeCons::Rules` 补上 `{'P', 'I', 'p'}`，让
  `Unknown - Unknown = Pointer` 正常推成 `Pointer - Number = Pointer`。
- apehex native 30 样本和 EVM type recovery ctest 已通过。

现在需要从“能跑通”推进到“结果稳定、可回归、能继续接字段”。

## 目标

1. 固定刚完成的多返回和 PNDiff 规则，避免后续回退。
2. 用已经筛好的 80 个 apehex pilot 样本做 type-recovery-only 验证，不急着全量重跑
   Gigahorse。
3. 先确认 HType 质量：`extractvalue` 多返回字段正常工作，再确认类型推理能否分析出内存类型。

## 路线

### 1. 补最小回归测试

先补一个 frozen LLVM IR case，覆盖两个点：

- callee 返回 `{ i256, i256 }`，caller 用 `extractvalue` 取两个字段。
- 构造一个 `sub` 场景，让 PNDiff 需要使用 `Unknown - Unknown = Pointer` 到
  `Pointer - Number = Pointer` 的规则。

判断标准：

- `ctest --test-dir build -R notdec.type_recovery.evm.tr_level_2 --output-on-failure`
  继续通过。
- oracle 或 dump 检查能看到函数 HType 是多返回。
- 这个 case 不依赖 apehex 数据集。

### 2. 跑 selected-apehex-80 type-recovery-only 样本

使用已经筛好的 80 个 pilot 样本：

```text
/sn640/NotDecChainExp/evm2llvm_apehex_pilot/selected-apehex-80/manifest.csv
```

先复用已有 `outputs/*.ll`，只跑：

```bash
./build/bin/notdec sample.ll -o out.ll --tr-level=2 --dump-htypes sample.htypes
```

不重跑 Gigahorse，不重新 evm2llvm。这样能快速覆盖类型恢复本身。

观察点：

- 是否还有 assert / abort。
- HType 里 private helper 多返回是否稳定。
- 普通 memory record 是否没有被误拆成多返回。

判断标准：

- 80 个 pilot 样本全部 `notdec_tr` ok。
- 多返回样本里能看到类似 `((ret0, ret1) (*)(...))*` 的函数类型。

### 3. 确认 HType 质量和内存类型

先从验证样本里挑 3-5 个有 private 多返回的样本，确认 `extractvalue` 相关结果：

- 函数类型是否输出多返回。
- `extractvalue` 后的字段是否拿到对应类型。
- `ReturnValue::<ret>` 和 `::<ret:N>` 是否分开。
- 普通 struct/memory record 没有被当 tuple return 拆掉。

然后再看类型推理是否已经能分析出内存类型。第一轮只看高置信度情况：

- allocation base 能确定。
- offset 是常量。
- load/store 是 32 字节 word 或能明确换算到 bit size。
- 不处理动态 ABI head/tail，不猜 range copy。

目标是让 HType 或 debug 输出里开始出现：

- return buffer 字段。
- event data 字段。
- external call input/output buffer 字段。

判断标准：

- `extractvalue` 多返回字段在 HType 中能稳定对应到字段返回值。
- 80 个 pilot 样本中能定位出已有内存类型恢复效果较好的样本。
- 常量 offset 字段在 HType/debug 输出中可见。
- 低置信度 object 绑定先不写约束，只记录或跳过。

## 风险

- 最小回归测试如果只看不崩，无法保证 HType 真的是多返回，所以需要 oracle 或明确检查 dump。
- `record{"0","1"}` 目前是 function result 位置的 tuple 约定，不能扩散到普通 memory record。
- memory object offset 字段如果绑定错 base，会污染类型结果。第一版必须只做高置信度 constant offset。

## 暂不做

- 暂不改 binarysub 核心 `UFunctionType` 为原生多返回。
- 暂不重跑完整 apehex。
- 暂不做动态 ABI bytes/string/array。
- 暂不让类型结果驱动 ABI rewrite 或 cleanup。
