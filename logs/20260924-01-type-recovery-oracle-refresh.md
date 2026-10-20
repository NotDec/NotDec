# 20260924-01 type_recovery oracle 刷新：3 套重锚 + fortune 锚点与 DWARF 报告

## 0. 一句话

`20260923-03` §5 第 3 项（4 套 type_recovery 自 2026-08-03 起漂移）处理完：
漂移全部来自已提交的类型恢复行为，与工作区里 binarysub/MLsub 的本地改动无关
（用“把 committed 版 `MLsubGenerator.cpp` 单独编成 object、换进 `libnotdec-typerecovery.a`
再链接”的二进制复跑，47 个 `.htypes` 输出逐字节相同）。据此把 llvm-ir 14 例、evm 11 例、
sysy 9 例共 **34 个 golden 重锚**，realworld 的 fortune fixture 锚点哈希刷新后不再 abort、
真正跑起了 wasm-DWARF 对比，对比报告显示全局槽和 struct 字段仍大量缺失，按 README 约定把该例
标成 `xfail` 并写明理由。4 套 ctest 现在全绿（llvm-ir 22、evm 16、sysy 9 通过，realworld 1 xfail）。

## 1. 判定“漂移 vs 本地改动”

工作区里 `include/notdec/TypeRecovery/mlsub/MLsubGenerator.h` 与 `src/TypeRecovery/mlsub/MLsubGenerator.cpp`
是别的会话的本地改动（`addConstantMemoryField` 从“存加载值类型”改成“存地址节点”）。
为了不把 WIP 当成基线，也没有动它的工作区文件，做法是：

1. `git show HEAD:<path>` 取 committed 的 header/source 到 `/tmp/head-mlsub/`；
2. 从 `ninja -t commands notdec` 里取 `MLsubGenerator.cpp.o` 的编译命令，把
   `-I/sn640/NotDec/include/notdec` 前插 `-I/tmp/head-mlsub/include`、源码路径换掉、输出换到 `/tmp/MLsubGenerator.head.o`；
3. 拷贝 `src/libnotdec-typerecovery.a`，`ar d` 掉原 `MLsubGenerator.cpp.o`、`ar r` 进 committed object，得到 `/tmp/libnotdec-typerecovery-head.a`；
4. 用同一条链接命令（把 `src/libnotdec-typerecovery.a` 换成临时库）链接出 `/tmp/notdec.headsrc`。

结果：committed 二进制与当前（含 WIP）二进制在 3 套 suite 的 47 个 `.htypes` 上
`cmp` 全部一致，失败清单也一致 —— **漂移与这次 WIP 无关，全部是已提交行为**。

## 2. 漂移构成（34 例，按 diff 分类）

| 类别 | 例子 | 来源 |
| --- | --- | --- |
| `bytes: N` 字段字节范围注解，+81 / -0 | 全部有 struct 的用例 | binarysub `dd622f3`（2026-09-18）HType report: always print the recovered field byte extent |
| `const(i32 N)@loc` → `addr(0x…)` | llvm-ir 02/08/19 | `0c9bf33c` treat constant inttoptr addresses as memory fields in every module、`30abfc4b` |
| 递归 struct 归并（rec_16/rec_18 收进 rec_12/rec_14） | llvm-ir 06 | 递归 SCC 归一化线（`12ea5c00` 之后的延续） |
| 新增 struct/字段（+9 个 struct） | llvm-ir 02/09/17/19/20/21/22 | 同上 + 常量地址建 record |
| 丢失 struct（-5）：memory marker 不再物化 record；storage root 结构重编号 | evm 03/11/14、llvm-ir 06 | EVM memory/storage 分离线；storage 链本身仍在（struct_0..struct_5），只是少了 memory 侧的 `struct_0*`，`[memory] type` 变成 `void**` |
| 变量 dump 覆盖变全 + 名字/下标位移 | sysy 9 例（如 21_if_test2 从 3 行变 38 行，`bb0.i0` → `bb0.i1`）、llvm-ir 其余 | `51a87312`/`e121c340` 之后的 dump 口径 |

这 5 处 struct 丢失都看过：evm 03 原来那个 struct 是 memory marker 物化出来的（现在 memory
类型不再是 record），evm 11/14 丢的是 memory 侧同型 struct、storage 链完整，llvm-ir 06 是递归
SCC 归并。都符合当前类型恢复线的方向，没有发现需要单独修的行为回归，因此按仓库惯例
（`985a6aea test: align EVM type-recovery oracles with current MLsub output`）重锚；
如果类型恢复线认为其中某一处是回归，重锚前的内容在 git 里可直接对比。

重锚用的二进制是 committed 版（`/tmp/notdec.headsrc`），写回时用 suite 自己的
`normalize_text`（逐行 rstrip + strip + 末尾一个换行），避免行尾差异噪声。

## 3. realworld：锚点哈希刷新 + xfail

原来的失败是 fixture 的 `ir_anchor.sha256` 与当前 MLsub 输入对不上，`failExtraConstraints()`
直接 abort。刷新时发现一个隐藏坑：

- 锚点哈希是 `computeSHA256Hex(renderModuleToString(M))`，而打印出来的 IR 第一行是
  `; ModuleID = '<输入路径原样>'`。**同一个文件，相对路径和绝对路径得到不同哈希**：
  `test/type-recovery/...` → `198236eb…`，`/sn640/NotDec/test/type-recovery/...` → `06a1f6c5…`。
  suite 的 runner 用 `resolve_path` 传绝对路径，所以 fixture 必须写 `06a1f6c5…`；
  按归档文档 `logs/archive/20260509-04-FortuneFrozenExtraConstraintAnchorRefresh.md` 时的手工命令
  （相对路径）会得到另一个值，这正是历史上反复“刷新锚点”后仍然对不上的原因之一。

改动：
- `test/type-recovery/realworld/support/fortune.o3.wasm.extra.json` 的
  `ir_anchor.sha256`：`3b42e60d…` → `06a1f6c5…`（2026-09-24 当前 suite 口径）。
- 锚点通过后，两个 `pndiff` selector 仍然解析成功，流程走完并产出对比报告。

对比报告（`build-notdec-nothreads/test/artifacts/notdec.type_recovery.realworld.tr_level_2/fortune.o3.wasm.compare.txt`）
显示这是真实的质量缺口，不是 fixture 问题：

- `@File_list` / `@File_tail` 的全局槽（offset 2268 / 3328）没有恢复；
- `@Noprob_tbl` 的 `str_version`/`str_numstr`/`str_longlen`（3340/3344/3348）缺失；
- `free_desc::arg0`、`get_tbl::arg0`、`matches_in_list::arg0` 等参数 struct 里大量 DWARF 字段缺失
  （`struct_1`/`struct_6` 只恢复了一部分偏移）。

按 `test/type-recovery/llvm-ir/README.md` 的 manifest 约定（`xfail` = known type recovery
failure），把该例标成 `xfail` 并把理由写进 manifest；对比报告仍在 artifacts 里，字段补齐后
跑一次会变成 `XPASS`，提示可以改回 `pass`。

## 4. 验证

命令与结果：

```
cd /sn640/NotDec
python3 test/run_type_recovery_suite.py --binary build-notdec-nothreads/bin/notdec \
  --manifest test/type-recovery/llvm-ir/manifest.json --project-root . --workdir /tmp/tr-check-llvm-ir
  → 22 passed, 0 failed
… evm → 16 passed, 0 failed
… sysy → 9 passed, 0 failed
… realworld → 0 passed, 1 xfailed, 0 failed

ctest --test-dir build-notdec-nothreads -R notdec.type_recovery
  → 4/4 Passed
```

重锚只改 `test/type-recovery/*/expected` 与 realworld fixture/manifest，不动源码；
其它套件（EVM solidity 系列、lifting、structuring）不受影响。

## 5. 遗留

- fortune 的字段/全局恢复缺口是类型恢复线的实际任务，报告里逐字段列了缺失偏移，可直接当工作清单；
  另外 `ir_anchor` 依赖输入路径拼写这件事值得单独修（anchoring 应基于 IR 内容而不是打印出的 ModuleID），
  否则换一个调用路径就又要刷新一次。
- 本轮重锚只覆盖当前 committed 行为；工作区里 MLsub/binarysub 的本地改动落地后，如果改变
  `.htypes` 输出，需要再跑一次同样的重锚流程（脚本思路见 §2）。
