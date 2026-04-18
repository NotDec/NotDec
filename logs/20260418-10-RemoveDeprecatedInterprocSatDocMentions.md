# 20260418-10 Remove Deprecated Interproc/SAT Doc Mentions

## 1. 背景

当前 `tr-level >= 2` 主链路已经切到 `mlsub::MLsubRecoveryMain`，
而 `NOTDEC_DISABLE_INTERPROC` / `NOTDEC_SAT_DISABLE` 的现存代码引用仍在旧的
retypd / `ConstraintGraph` 路径中。

因此，把这两个环境变量继续写成 README / AGENTS 中的当前推荐入口，容易让后续调试和性能分析沿用过时习惯。

## 2. 修改

1. 删除 README 中英文 Notice 里对这两个环境变量的推荐：
   - [README.md](/sn640/NotDec/README.md#L42)
   - [README.md](/sn640/NotDec/README.md#L79)
   - 变更位置：英文 Notice `42-45` 行附近、中文 Notice `79-82` 行附近
   - 涉及函数：无，文档变更
2. 删除 AGENTS 中把这两个环境变量当作当前推荐调试入口的条目：
   - [AGENTS.md](/sn640/NotDec/AGENTS.md#L193)
   - 变更位置：运行与调试约定小节、`--gen-work-dir` 说明附近，当前落在 `193-194` 行
   - 涉及函数：无，文档变更

## 3. 备注

1. 本次只收敛当前文档表述，不改历史 `logs/` 中保留的执行记录。
2. 代码层面仍可见旧环境变量引用：
   - [src/Passes/ConstraintGenerator.cpp](/sn640/NotDec/src/Passes/ConstraintGenerator.cpp#L122)
   - [src/TypeRecovery/ConstraintGraph.cpp](/sn640/NotDec/src/TypeRecovery/ConstraintGraph.cpp#L1586)
3. 如果后续决定彻底移除旧入口，再单独清理脚本、旧路径判断和相关实现。
