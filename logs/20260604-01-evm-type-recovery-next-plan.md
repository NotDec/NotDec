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
2. 扩大 apehex type-recovery-only 验证，不急着全量重跑 Gigahorse。
3. 开始接 memory object 的 constant offset 字段，让 HType 里能稳定看到 return/event/call buffer 字段。
4. 把 `PNDiff.warn.txt` 纳入批跑观察，不只看 `notdec_tr` 是否成功。

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

### 2. 跑 50-100 个 apehex type-recovery-only 样本

先复用已有 `outputs/*.ll`，只跑：

```bash
./build/bin/notdec sample.ll -o out.ll --tr-level=2 --dump-htypes sample.htypes
```

不重跑 Gigahorse，不重新 evm2llvm。这样能快速覆盖类型恢复本身。

观察点：

- 是否还有 assert / abort。
- `PNDiff.warn.txt` 里 residual Add/Sub 数量是否异常增多。
- HType 里 private helper 多返回是否稳定。
- 普通 memory record 是否没有被误拆成多返回。

判断标准：

- 50-100 个样本全部 `notdec_tr` ok。
- 多返回样本里能看到类似 `((ret0, ret1) (*)(...))*` 的函数类型。
- 没有新的高频 residual PNDiff 模式。

### 3. 人工抽查 HType 质量

从验证样本里挑 3-5 个有 private 多返回的样本，人工看：

- 函数类型是否输出多返回。
- `extractvalue` 后的字段是否拿到对应类型。
- `ReturnValue::<ret>` 和 `::<ret:N>` 是否分开。
- 普通 struct/memory record 没有被当 tuple return 拆掉。

这一步的重点不是覆盖率，而是确认建模方向没有偏。

### 4. 接 memory object constant offset 字段

多返回稳定后，再回到 native memory 类型恢复主线。

第一版只做高置信度情况：

- allocation base 能确定。
- offset 是常量。
- load/store 是 32 字节 word 或能明确换算到 bit size。
- 不处理动态 ABI head/tail，不猜 range copy。

目标是让 HType 或 debug 输出里开始出现：

- return buffer 字段。
- event data 字段。
- external call input/output buffer 字段。

判断标准：

- 新增字段约束不会造成现有 30/50/100 样本崩溃。
- 常量 offset 字段在 HType/debug 输出中可见。
- 低置信度 object 绑定先不写约束，只记录或跳过。

### 5. 把 PNDiff warning 用起来

当前 `workdir/PNDiff.warn.txt` 会记录 solve 后仍残留的 Add/Sub 约束。
后续批跑要统计：

- residual Add/Sub 数量。
- 新增 residual 的样本。
- residual 的指令形态和 PNDiff state。

这比只看 `notdec_tr ok` 更有用。类型恢复成功但残留大量 PNDiff，说明结果可能不稳定。

## 风险

- 最小回归测试如果只看不崩，无法保证 HType 真的是多返回，所以需要 oracle 或明确检查 dump。
- `record{"0","1"}` 目前是 function result 位置的 tuple 约定，不能扩散到普通 memory record。
- memory object offset 字段如果绑定错 base，会污染类型结果。第一版必须只做高置信度 constant offset。
- `PNDiff.warn.txt` 可能已有历史 residual，需要按同口径比较，不要把旧问题当新回归。

## 暂不做

- 暂不改 binarysub 核心 `UFunctionType` 为原生多返回。
- 暂不重跑完整 apehex。
- 暂不做动态 ABI bytes/string/array。
- 暂不让类型结果驱动 ABI rewrite 或 cleanup。

