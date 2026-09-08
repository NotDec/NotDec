# Apehex EVM 跑批说明

本文档集中记录 apehex/evm2llvm 跑批的路径和历史运行约定。需要运行、排查或扩展
EVM 批处理时先读本文档；其中的 screen 会话和批次示例是历史记录，不能据此断言当前
仍有后台循环运行。

## 数据和脚本

- 数据集：`/sn640/NotDecChainExp/apehex_evm_contracts`
- 运行根目录：`/sn640/NotDecChainExp/evm2llvm_apehex_pilot`
- 历史结果：`/sn640/NotDecChainExp/apehex_evm_contracts/notdec-runs`
- 当前 IR-only 批处理脚本：`/sn640/NotDecChainExp/evm2llvm_apehex_pilot/scripts/notdec-evm2llvm-ir-apehex-batch.py`
- 完整脚本参数、失败重跑和输出布局：外部运行目录的
  `docs/apehex_batch_loop.md`

历史上曾在 screen 的 `apehex-longrun` 会话中循环运行，批次名形如
`YYYYMMDD-evm2llvm-train-batchNNN`。一次批次通常使用
`--gigahorse-dir /sn640/gigahorse-toolchain`、`--batch-size 40` 和 `--jobs 8`；
每个批次包含 `manifest.csv`、`summary.csv`、`inputs/`、`logs/`、`outputs/` 和
`work/`。开始新批次或重跑失败样本前，先按外部文档的 `--dry-run` 流程确认游标和范围。

## NotDec 评估

CheckedBoundsPass 迭代默认从最近批次抽约 100 个 IR 做 smoke/audit。只有出现新的
checked-bounds skip、runner 失败或明确需要扩大覆盖时才扫描更多批次。涉及 Solidity
语义时，对照 `/sn640/solidity` 的 codegen，不要只从样例推断。
