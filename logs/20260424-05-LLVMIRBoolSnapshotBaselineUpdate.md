# 2026-04-24 llvm-ir bool snapshot baseline 更新

## 背景

`test/type-recovery/llvm-ir` 的 `tr-level=2` suite 在当前代码下失败了 6 个 case。

核对 `expected` 和实际输出后，差异都不是类型恢复结构变化，而是 snapshot 文本里：

- 旧 baseline：`i1`
- 当前实际输出：`bool`

这和今天已经落地的 `HType` 整数打印规则一致，所以这次只更新 baseline，不改代码。

## 代码修改

1. `test/type-recovery/llvm-ir/expected/tr-level-2/09_OffsetLoop.htypes:26`
   - 修改内容：
     - `main::%cmp` 从 `i1` 改成 `bool`

2. `test/type-recovery/llvm-ir/expected/tr-level-2/12_Stack1.htypes:10-12`
   - 修改内容：
     - `main::%eq1`
     - `main::%eq2`
     - `main::%eq3`
     - 三处都从 `i1` 改成 `bool`

3. `test/type-recovery/llvm-ir/expected/tr-level-2/14_Equality1.htypes:14`
   - 修改内容：
     - `main::%cond` 从 `i1` 改成 `bool`

4. `test/type-recovery/llvm-ir/expected/tr-level-2/16_Poly1.htypes:18-20`
   - 修改内容：
     - `main::%eq1`
     - `main::%eq2`
     - `main::%eq3`
     - 三处都从 `i1` 改成 `bool`

5. `test/type-recovery/llvm-ir/expected/tr-level-2/17_StackArray.htypes:45,68`
   - 修改内容：
     - `goodG2B::%brif_val.not` 从 `i1` 改成 `bool`
     - `goodG2B::%new_81` 从 `i1` 改成 `bool`

6. `test/type-recovery/llvm-ir/expected/tr-level-2/18_offset1.htypes:88,103`
   - 修改内容：
     - `test1::%bb.brif_next.i9` 从 `i1` 改成 `bool`
     - `test2::%bb.brif_next.i8` 从 `i1` 改成 `bool`

## 验证

1. 执行
   - 命令：
     - `ctest --test-dir build -R notdec.type_recovery.llvm_ir.tr_level_2 --output-on-failure`

2. 结果
   - `notdec.type_recovery.llvm_ir.tr_level_2` 通过
   - suite 汇总恢复为：
     - 10 passed
     - 3 xfailed
     - 0 failed

## 备注

这次是 baseline 跟随当前打印规则更新。

如果后面要保留 `i1` 而不是 `bool`，那就不该继续改 baseline，而应该回头改：

- `HType::getAsString()`
- `HTypeSnapshotFormatter::formatType()`

让 snapshot 和常规打印重新回到旧格式。
