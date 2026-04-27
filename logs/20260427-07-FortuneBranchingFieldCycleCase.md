# 2026-04-27 fortune branching field cycle case

## 结论

这轮继续看 `fortune.o3.wasm.ll` 之后，已经能更明确地说：

1. 不是单个 field 自己无限变长。
2. 也不只是“同一路径重复插入很多次”。
3. 更接近的形态是：
   - 同一个 loop-carried 指针 slot
   - 回边上同时存在多个 field 选择
   - 其中至少有两个不同 ptradd 语句，虽然 offset 一样，但 tag 不一样
   - 于是 PA 在同一个 root 下枚举越来越多的 field-path 组合

`fortune` 里最关键的一段就在 `main` 的 regex 构造循环：

- `%_local_3.11 -> %608` 是 `@1`
- `%_local_3.11 -> %615` 是一个 `@4`
- `%_local_3.11 -> %624` 是另一个 `@4`

而当前 `PAPathAtom` 相等性包含 `Tag`，所以这两个 `@4` 在 trace 里都打印成
`@4`，但在 PA key 里其实不是一个 atom。

这解释了为什么：

- 只靠“重复后缀折星号”的方案不够
- trace 里看上去像重复的 `@4`，实际仍会继续分叉

## 新增最小复现

### 修改文件与行号

1. `test/type-recovery/llvm-ir/cases/21_PointerAnalysisBranchingFieldCycle.ll`
   - `1-6`
     - 说明这个 case 抽的是 `fortune` 的哪种结构。
   - `12-55`
     - 新增 `main`
     - 只保留最小骨架：
       - `malloc` 根对象
       - loop-carried 指针 `%p`
       - 一个 `+1` 回边
       - 两个来自不同 branch site 的 `+4` 回边
       - 少量 `inttoptr/load/store`，保证 ptradd 不会被轻易优化掉

### 涉及函数

- `main` in
  `test/type-recovery/llvm-ir/cases/21_PointerAnalysisBranchingFieldCycle.ll`

## 验证

命令：

```bash
timeout 7s env NOTDEC_POINTER_ANALYSIS_MODE=shadow NOTDEC_BINARYSUB_TRACE=1 \
  ./build/bin/notdec \
  test/type-recovery/llvm-ir/cases/21_PointerAnalysisBranchingFieldCycle.ll \
  -o /tmp/21_branching_field_cycle.ll --tr-level=2 --frozen-tr-input-ir \
  -g --work-dir=/tmp/notdec-branching-field-cycle
```

关键 trace：

- `iter=10`
  - `locs=795`
  - `max_set=240`
- `iter=12`
  - `locs=1953`
  - `max_set=576`
- `iter=14`
  - `locs=4623`
  - `max_set=1344`

这和 `fortune` 热点段的增长节奏是同一类现象，不是偶然。

同时 trace 里还能直接看到“看起来一样，但其实不是同一个 atom”的症状：

- `iter=2`
  - `field@value(main::%wide.left.next)->main::heap(main::%base).@4[32]:1`
  - `field@value(main::%wide.right.next)->main::heap(main::%base).@4[32]:1`

这两条都打印成 `@4`，但它们来自不同 ptradd 语句，当前 key 里不会合并。

## 对 `fortune` 的判断

现在更像是下面这个问题：

1. 回边 field 不是单一循环，而是带分叉的网络。
2. 当前 path dedup 主要擅长压 `X X -> X*` 这种重复后缀。
3. 但对 `@1 / @4a / @4b` 这种分叉组合，它不会自动收成一个小摘要。
4. 再叠加全边全量扫描，偶数轮基本只是在吃 duplicate 成本。

## 后续可做的方向

1. 先把 trace 打印补全
   - `formatPAPathAtom()` 在调试模式下把 tag 也打出来
   - 否则现在很多“重复样本”只是字符串看起来一样
2. 尝试更粗的 field tag 归一化
   - 例如把“同 base、同 offset、同 SCC 内”的 ptradd 合到一个 virtual tag
   - 先专门验证能不能把这两个 `@4` 合掉
3. 求解器层面再补 delta/worklist
   - 当前 trace 里偶数轮常见 `field inserted=0`
   - 这说明很多轮只是全量重扫旧 loc
