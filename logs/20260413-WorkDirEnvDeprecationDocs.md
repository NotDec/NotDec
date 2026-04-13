# 2026-04-13 WorkDir Env Deprecation Docs

## 背景

仓库已经切到 `--gen-work-dir` / `--work-dir` 这套 workdir CLI，
但 `AGENTS.md` 的“运行与调试约定”里仍把：

- `NOTDEC_DEBUG_DIR`
- `NOTDEC_TYPE_RECOVERY_DEBUG_DIR`

写成当前推荐调试入口，和实际实现不一致。

## 本次修改

更新 `AGENTS.md`：

1. 将工作目录机制改写为当前事实
   - `--gen-work-dir` / `-g`
   - `--work-dir=<path>`

2. 将 `NOTDEC_DEBUG_DIR` / `NOTDEC_TYPE_RECOVERY_DEBUG_DIR` 明确标注为
   已被 workdir 机制替代、文档层面弃用

3. 保留仍有效的分析环境变量说明
   - `NOTDEC_SUMMARY_OVERRIDE`
   - `NOTDEC_SIGNATURE_OVERRIDE`
   - `NOTDEC_SAT_DISABLE`
   - `NOTDEC_DISABLE_INTERPROC`
   - `NOTDEC_SAT_NOPTRRULE`

## 备注

`README.md` 与 `DEBUG.md` 当前已经使用 workdir 语义，因此本次无需同步改动。
