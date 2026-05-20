# Evm2llvm Apehex Batch Loop Relocation

## 这次做了什么

把 apehex 的 batch loop 从 `external/NotDec-evm2llvm` 挪到了 `NotDecChainExp`，保持行为不变，只改放置位置和入口说明。

## 改动点

- `/sn640/NotDecChainExp/evm2llvm_apehex_pilot/scripts/notdec-evm2llvm-apehex-batch.py`
  - 保留原来的批量选样、失败重跑、manifest/summary 写出、逐样本执行链路。
  - 主要入口还是 `parse_args()`、`load_state()`、`load_previous_failures()`、`build_new_samples()`、`classify_failure()`、`main()`。
  - 关键逻辑分布在 87-188、191-199、234-291、342-396、399-531 行。
- `/sn640/NotDecChainExp/evm2llvm_apehex_pilot/docs/apehex_batch_loop.md`
  - 保留使用方式、输出目录结构、测试流程说明。
- `/sn640/NotDec/external/NotDec-evm2llvm/docs/README.md`
  - 第 47-53 行改成指向新的文档位置，作为这里的入口提示。
- `/sn640/NotDec/external/NotDec-evm2llvm/.gitignore`
  - 新增 `__pycache__/` 和 `*.pyc`，避免 Python 字节码进状态。

## 删除的旧位置

- `/sn640/NotDec/external/NotDec-evm2llvm/scripts/notdec-evm2llvm-apehex-batch.py`
- `/sn640/NotDec/external/NotDec-evm2llvm/docs/apehex_batch_loop.md`

## 验证

- `python3 -m py_compile /sn640/NotDecChainExp/evm2llvm_apehex_pilot/scripts/notdec-evm2llvm-apehex-batch.py`

## 结论

这次只是搬迁和挂入口，不改 batch 流程本身。后续新的用例和重跑流程都从 `NotDecChainExp/evm2llvm_apehex_pilot/` 这边维护。
