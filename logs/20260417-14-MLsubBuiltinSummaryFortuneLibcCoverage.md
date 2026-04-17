# 20260417-14 MLsub builtin summary fortune libc coverage

## 背景

检查 `test/type-recovery/howard-o3-split/cases/fortune.o3.wasm.4.ll` 的外部 libc 调用时，发现内置 summary 资源还缺少 9 个当前 case 实际会调用到的函数条目。

## 本次修改

1. 在 `/sn640/NotDec/resources/mlsub_builtin_summaries.json` 中补齐以下函数的 builtin summary 与 `pndiff`：
   - `exit`
   - `fseek`
   - `getpid`
   - `lseek`
   - `ntohl`
   - `random`
   - `sleep`
   - `srandom`
   - `time`
2. 具体位置：
   - `/sn640/NotDec/resources/mlsub_builtin_summaries.json:71` 新增 `exit`
   - `/sn640/NotDec/resources/mlsub_builtin_summaries.json:186` 新增 `fseek`
   - `/sn640/NotDec/resources/mlsub_builtin_summaries.json:236` 新增 `getpid`
   - `/sn640/NotDec/resources/mlsub_builtin_summaries.json:243` 新增 `lseek`
   - `/sn640/NotDec/resources/mlsub_builtin_summaries.json:267` 新增 `ntohl`
   - `/sn640/NotDec/resources/mlsub_builtin_summaries.json:348` 新增 `random`
   - `/sn640/NotDec/resources/mlsub_builtin_summaries.json:451` 新增 `sleep`
   - `/sn640/NotDec/resources/mlsub_builtin_summaries.json:476` 新增 `srandom`
   - `/sn640/NotDec/resources/mlsub_builtin_summaries.json:607` 新增 `time`

## 类型选择说明

1. 以当前 wasm32 IR 声明为准：
   - `lseek` / `time` 返回 `i64`
   - `fseek` / `getpid` / `random` 返回 `i32`
   - `exit` / `srandom` 返回 `void`
2. `time` 的参数按 `time_t *` 建成 `ptr<i64>`，便于和 `time(NULL)` 这类调用共享同一 summary 入口。

## 验证

1. 已执行 `python3 -m json.tool resources/mlsub_builtin_summaries.json`，确认资源文件格式合法。
2. 已对 `test/type-recovery/howard-o3-split/cases/fortune.o3.wasm.4.ll` 重新统计覆盖，结果为 51/51 命中。
