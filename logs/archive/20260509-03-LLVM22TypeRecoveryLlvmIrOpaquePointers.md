# 2026-05-09 LLVM 22 `test/type-recovery/llvm-ir` 迁移

## 背景

`test/type-recovery/llvm-ir/` 这批 case 还保留了不少旧式 `T*` IR 写法。
当前目标是把输入改成 LLVM 22 能直接吃的 opaque-pointer 语法，同时把已经跑通的 case
从 `xfail` 收回成正常 `pass`。

这次只动测试输入和 goldens，不碰主项目源码，也不碰当前 `fortune` 的未提交改动。

## 目标

1. 把 `cases/*.ll` 里的 typed pointer 语法改成 `ptr`。
2. 修掉 `19_PtrPtr.ll` 里 LLVM 22 不接受的 `bitcast i32 -> ptr`。
3. 把已经恢复正常的 case 从 `xfail` 改成 `pass`，补齐对应 `.htypes`。
4. 把目录里原本没进 manifest 的 `21_PointerAnalysisBranchingFieldCycle.ll` 一并纳入。

## 已完成

### 1. 批量改成 LLVM 22 opaque-pointer 语法

这次改的是输入 IR 本身，主要是把 `i32*`、`i64*`、`i8*`、`float*`、`[N x i8]*`
这些旧写法换成 `ptr`。

改动范围：

1. `test/type-recovery/llvm-ir/cases/02_ConstantAddr1.ll:5-8`
2. `test/type-recovery/llvm-ir/cases/03_LoadUpdateGood.ll:5-10`
3. `test/type-recovery/llvm-ir/cases/04_LoadUpdate.ll:5-10`
4. `test/type-recovery/llvm-ir/cases/05_MultiOffset.ll:6-14`
5. `test/type-recovery/llvm-ir/cases/06_SimpleRecursive2.ll:4-15`
6. `test/type-recovery/llvm-ir/cases/07_PassStack1.ll:4-19`
7. `test/type-recovery/llvm-ir/cases/08_Memory1.ll:7-18`
8. `test/type-recovery/llvm-ir/cases/09_OffsetLoop.ll:6-16`
9. `test/type-recovery/llvm-ir/cases/10_BottomUp1.ll:6-18`
10. `test/type-recovery/llvm-ir/cases/11_SimpleRecursive1.ll:7-21`
11. `test/type-recovery/llvm-ir/cases/12_Stack1.ll:4-23`
12. `test/type-recovery/llvm-ir/cases/13_stack_variable_alloc.ll:9-17`
13. `test/type-recovery/llvm-ir/cases/14_Equality1.ll:6-31`
14. `test/type-recovery/llvm-ir/cases/15_signed1.ll:9-38`
15. `test/type-recovery/llvm-ir/cases/16_Poly1.ll:7-48`
16. `test/type-recovery/llvm-ir/cases/17_StackArray.ll:43-102`
17. `test/type-recovery/llvm-ir/cases/18_offset1.ll:6-118`
18. `test/type-recovery/llvm-ir/cases/19_PtrPtr.ll:5-12`
19. `test/type-recovery/llvm-ir/cases/20_PointerAnalysisFieldCycle.ll:8-28`
20. `test/type-recovery/llvm-ir/cases/21_PointerAnalysisBranchingFieldCycle.ll:12-55`

涉及到的关键函数主要是：

- `@fun_1`、`@main` in `07_PassStack1.ll`
- `@goodG2B` in `17_StackArray.ll`
- `@do_printf`、`@get_s`、`@test1`、`@test2` in `18_offset1.ll`
- `@conv_signed`、`@conv_unsigned`、`@main` in `15_signed1.ll`
- `@main` in `19_PtrPtr.ll`、`20_PointerAnalysisFieldCycle.ll`、`21_PointerAnalysisBranchingFieldCycle.ll`

### 2. 修掉 LLVM 22 下不合法的 cast

- `test/type-recovery/llvm-ir/cases/19_PtrPtr.ll:5-12`
  - 把 `bitcast i32 %b to i32*` 改成 `inttoptr i32 %b to ptr`
  - 这样才能继续后面的 `load i32, ptr %c`

### 3. 更新 manifest 和 golden

- `test/type-recovery/llvm-ir/manifest.json:80-133`
  - `13_stack_variable_alloc`、`15_signed1`、`19_PtrPtr` 从 `xfail` 改成 `pass`
  - 新增 `21_PointerAnalysisBranchingFieldCycle`
  - 现有 case 全部补上 `expected/tr-level-2/*.htypes`

- `test/type-recovery/llvm-ir/expected/tr-level-2/07_PassStack1.htypes`
  - 重新收了 `07_PassStack1` 的新结果

- 新增 goldens：
  - `test/type-recovery/llvm-ir/expected/tr-level-2/13_stack_variable_alloc.htypes`
  - `test/type-recovery/llvm-ir/expected/tr-level-2/15_signed1.htypes`
  - `test/type-recovery/llvm-ir/expected/tr-level-2/19_PtrPtr.htypes`
  - `test/type-recovery/llvm-ir/expected/tr-level-2/21_PointerAnalysisBranchingFieldCycle.htypes`

## 验证

单测命令：

```bash
ctest --test-dir build -R notdec.type_recovery.llvm_ir.tr_level_2 --output-on-failure
```

结果：

- `20 passed, 0 xfailed, 0 xpassed, 0 failed`
- `real 2.28s`

我还单独跑过这几个 case 的 `notdec`：

- `13_stack_variable_alloc.ll`
- `15_signed1.ll`
- `19_PtrPtr.ll`
- `21_PointerAnalysisBranchingFieldCycle.ll`

都能正常退出，说明这次改动不是把旧失败点掩过去，而是确实把输入和 goldens 对齐了。

## 判断

方案评分：8/10。

优点：

- 改动很直接，输入已经统一到 LLVM 22 风格。
- `xfail` 收回后，suite 现在能真实反映当前状态。
- 这次只改测试输入和 golden，没有碰主路径代码，风险小。

代价：

- `17_StackArray` 和 `18_offset1` 这类大 case 还是保留了不少旧注释和分析标记，继续手工维护会比较累。
- 这批 case 里有些输出结构会随着 opaque-pointer 语法变化，后面如果再改类型恢复，golden 还是会跟着动。

后续如果还要继续收紧这套 suite，建议只做两件事：

1. 继续把新加的 LLVM 22 case 固定进 manifest。
2. 以后如果再改 IR 语法，先跑这套 suite，再决定要不要动类型恢复本体。
