# 2026-05-18 bin2llvm Bench2 重跑结果

## 用户原始 prompt

```text
尝试重新跑一下那几个目标的项目的转换，看是否发现更多问题
```

## 范围

这次不改代码，只用当前 `external/NotDec-bin2llvm` 重跑 Bench2 小样例。

产物目录：

```text
/tmp/notdec-bench2-rerun-20260518
```

构建命令：

```bash
cmake --build /tmp/notdec-bin2llvm-build \
  --target notdec-heritage-module-check notdec-heritage-module-llvm notdec-heritage-llvm -j4
```

## 模块级重跑

### `vsftpd --limit=5`

输入：

```text
/sn640/NotDec-Exp/Bench2/rootfs/usr/sbin/vsftpd
```

结果：

1. Ghidra 导出 5 个函数，失败 0 个，external 95 个，导出 4673 ms。
2. checker 通过：direct call 240 个，unknown call 0 个。
3. lowering：5 个 body 全部成功，`real 12.37s`。
4. `llvm-as` 通过，`real 1.43s`。
5. `!notdec.effect` 269 处，`!notdec.indirect` 10546 处。
6. 未发现 poison / freeze fallback。
7. JSON 中 `INDIRECT` 25554 个，全部有 `effectOp`。

### `vsftpd --limit=10`

结果：

1. Ghidra 导出 10 个函数，失败 0 个，external 98 个，导出 5266 ms。
2. checker 通过：direct call 245 个，resolved internal call 1 个，unknown call 0 个。
3. lowering：10 个 body 全部成功，`real 14.71s`。
4. `llvm-as` 通过，`real 1.31s`。
5. `!notdec.effect` 275 处，`!notdec.indirect` 10548 处。
6. 未发现 poison / freeze fallback。
7. JSON 中 `INDIRECT` 25558 个，全部有 `effectOp`。

判断：`vsftpd` 当前小模块没有新的 verifier 问题。`INDIRECT effectOp` 导出正常。

### `libuv --limit=5`

输入：

```text
/sn640/NotDec-Exp/Bench2/rootfs/usr/lib/x86_64-linux-gnu/libuv.so.1.0.0
```

结果：

1. Ghidra 导出 5 个函数，失败 0 个，external 2 个，导出 237 ms。
2. checker 通过：direct call 4 个，unknown call 0 个。
3. lowering：5 个 body 全部成功，`real 0.02s`。
4. `llvm-as` 通过，`real 0.02s`。
5. `!notdec.effect` 5 处。
6. 没有 `INDIRECT`。
7. 未发现 poison / freeze fallback。

### `libuv --limit=20`

结果：

1. Ghidra 导出 20 个函数，失败 0 个，external 3 个，导出 254 ms。
2. checker 通过：direct call 19 个，unknown call 0 个。
3. lowering：20 个 body 全部成功，`real 0.03s`。
4. `llvm-as` 通过，`real 0.02s`。
5. `!notdec.effect` 20 处。
6. 没有 `INDIRECT`。
7. 未发现 poison / freeze fallback。

判断：`libuv` 仍是干净回归样例。

### `memcached --limit=5`

输入：

```text
/sn640/NotDec-Exp/Bench2/rootfs/usr/bin/memcached
```

结果：

1. Ghidra 导出 5 个函数，失败 0 个，external 11 个，导出 292 ms。
2. checker 通过：direct call 19 个，resolved internal call 1 个，unknown call 0 个。
3. lowering：3 个 body 成功，2 个 body 失败，`real 0.11s`。
4. 失败函数：
   - `FUN_001066f0 ram:001066f0`
   - `FUN_00106711 ram:00106711`
5. 模块 `.ll` 仍可通过 `llvm-as`，因为失败 body 降级成 declaration。
6. JSON 中 `INDIRECT` 158 个，全部有 `effectOp`。

失败 1：

```text
FUN_001066f0 ram:001066f0:
Entry block to function must not have predecessors!
PHINode should have one entry for each predecessor of its parent basic block!
  %"vn:3650" = phi i64 [ %"vn:3656", %"bb:6" ], [ %"vn:3741.mem", %entry ], !notdec.indirect !12
```

判断：这是 CFG / PHI 结构问题。`entry` 被当成 PHI incoming predecessor，LLVM verifier 不接受。

失败 2：

```text
FUN_00106711 ram:00106711:
Function return type does not match operand type of return inst!
  ret i32 0
 i64
```

判断：函数返回类型推断或 `RETURN` lowering 类型选择不一致。

### `memcached --limit=20`

结果：

1. Ghidra 导出 20 个函数，失败 0 个，external 28 个，导出 677 ms。
2. checker 通过：direct call 243 个，resolved internal call 25 个，unknown call 0 个。
3. lowering：9 个 body 成功，11 个 body 失败，`real 1.62s`。
4. `llvm-as` 通过，`real 0.02s`，原因同上，失败 body 降级成 declaration。
5. JSON 中 `INDIRECT` 2263 个，全部有 `effectOp`。

失败分类：

1. CFG / PHI verifier：
   - `FUN_001066f0`
2. return 类型不一致：
   - `FUN_00106711`
   - `FUN_001069e6`
3. non-void function 缺少 return value：
   - `FUN_00106740`
   - `FUN_00106751`
   - `FUN_001067c5`
   - `FUN_001067d6`
   - `FUN_00106828`
   - `FUN_0010684a`
   - `FUN_001068a7`
   - `FUN_001068b8`

判断：`memcached` 是下一步更好的 verifier 问题集合，比 `vsftpd` 更容易复现，样例小，运行快。

## 旧单函数 JSON 扫描

扫描目录：

```text
/sn640/NotDec-Exp/Bench2/bin2llvm-ir/*/*.json
```

跳过 `module-*`，其余单函数 JSON 用当前 lowering 重新跑。

通过：

1. `ffmpeg/one-fg_create.cold`
2. `libicu/1-_ZN6icu_7412LocalPointerINS_9UVector32EED1Ev`
3. `libuv/1-uv__cancelled`
4. `lighttpd/1-main_init_once`
5. `memcached/1-drive_machine.lto_priv.0.cold`
6. `openssh/1-window_change_handler.lto_priv.0`
7. `php/one-zm_info_date`
8. `redis/1-dictSdsKeyCompare.cold`
9. `tmux/1-cmd_display_menu_args_parse.lto_priv.0.cold`
10. `vsftpd/1-vsf_sysutil_sockaddr_set_ipv4addr.cold`
11. `vsftpd/main`
12. `wolfssl/1-wc_PKCS7_DecodeEncryptedData.cold`

失败：

1. `python/one-_PyPegen_fill_token.cold`

```text
Function return type does not match operand type of return inst!
  ret i32 -1
 i64module verification failed
```

2. `wrk/one-parse_url_char.cold`

```text
Function return type does not match operand type of return inst!
  ret i32 1
 i64module verification failed
```

补充：`php/one-zm_info_date` 当前没有复现旧的 PHI verifier 失败，lower 和 `llvm-as` 都通过。

## 新发现的问题

1. `memcached` 模块级样例暴露 CFG / PHI verifier 问题：entry block 有 predecessor，且 PHI incoming 数不匹配。
2. 多个函数暴露 return 类型问题：函数签名是 `i64`，实际 `RETURN` lowering 生成 `ret i32`。
3. 多个 `memcached` 函数暴露 non-void return 缺少 value input。当前直接失败，后续需要判断 Ghidra 的函数返回类型是否可信，还是 `RETURN` pcode 对这类 thunk/cold fragment 没有值。
4. 当前模块工具会把失败 body 降级成 declaration，所以 `llvm-as` 通过不能代表所有 body 都成功。

## 下一步建议

优先处理 `memcached --limit=20`，因为它一次覆盖三类问题，且重跑很快：

1. 先看 `FUN_001066f0` 的 Ghidra CFG，确认为什么 entry 有 incoming。
2. 再看 `FUN_00106711` / `FUN_001069e6` 的 function prototype 和 `RETURN` input size。
3. 最后看缺 return value 的 7 个小函数，是不是 cold/thunk 片段或 Ghidra 返回类型误判。

## 2026-05-19 实施记录：先修容易的 RETURN 问题

本次只修 return lowering，不碰 CFG / PHI。

修改文件：

1. `external/NotDec-bin2llvm/lib/HeritageToLLVM.cpp`
   - 第 389 行新增 `resizeToIntegerType(...)`，按 LLVM 函数返回类型做 zext / trunc。
   - 第 1499 行修改 `returnValueFor(...)`，不再固定把返回值缩成 4 字节。
   - 第 1504 行处理 non-void `RETURN` 缺 value input：生成对应返回类型的 `undef`，并打印带 op id / 函数 / block / return type 的 warning。

效果：

1. `python/one-_PyPegen_fill_token.cold` 从 verifier 失败变为通过。
2. `wrk/one-parse_url_char.cold` 从 verifier 失败变为通过。
3. `memcached --limit=20` 从 20 个函数里 11 个 body 失败，降到 2 个 body 失败。
4. 剩下 2 个失败都是 CFG / PHI：
   - `FUN_001066f0`
   - `FUN_00106740`

验证命令：

```bash
cmake --build /tmp/notdec-bin2llvm-build \
  --target notdec-heritage-module-llvm notdec-heritage-llvm -j4

/tmp/notdec-bin2llvm-build/bin/notdec-heritage-llvm \
  /sn640/NotDec-Exp/Bench2/bin2llvm-ir/python/one-_PyPegen_fill_token.cold.json \
  -o /tmp/notdec-return-fix-regress/python/out.ll

/sn640/NotDec/llvm-22.1.0.obj/bin/llvm-as \
  /tmp/notdec-return-fix-regress/python/out.ll \
  -o /tmp/notdec-return-fix-regress/python/out.bc

/tmp/notdec-bin2llvm-build/bin/notdec-heritage-llvm \
  /sn640/NotDec-Exp/Bench2/bin2llvm-ir/wrk/one-parse_url_char.cold.json \
  -o /tmp/notdec-return-fix-regress/wrk/out.ll

/sn640/NotDec/llvm-22.1.0.obj/bin/llvm-as \
  /tmp/notdec-return-fix-regress/wrk/out.ll \
  -o /tmp/notdec-return-fix-regress/wrk/out.bc

/tmp/notdec-bin2llvm-build/bin/notdec-heritage-module-llvm \
  /tmp/notdec-bench2-rerun-20260518/memcached20/module-limit20.json \
  -o /tmp/notdec-return-fix-regress/memcached20/out.ll

/sn640/NotDec/llvm-22.1.0.obj/bin/llvm-as \
  /tmp/notdec-return-fix-regress/memcached20/out.ll \
  -o /tmp/notdec-return-fix-regress/memcached20/out.bc

/tmp/notdec-bin2llvm-build/bin/notdec-heritage-module-llvm \
  /tmp/notdec-bench2-rerun-20260518/vsftpd10/module-limit10.json \
  -o /tmp/notdec-return-fix-regress/vsftpd10/out.ll

/tmp/notdec-bin2llvm-build/bin/notdec-heritage-module-llvm \
  /tmp/notdec-bench2-rerun-20260518/libuv20/module-limit20.json \
  -o /tmp/notdec-return-fix-regress/libuv20/out.ll
```

验证结果：

1. `python` lower `real 0.02s`，`llvm-as` 通过，IR 中为 `ret i64 4294967295`。
2. `wrk` lower `real 0.02s`，`llvm-as` 通过，IR 中为 `ret i64 1`。
3. `memcached --limit=20` lower `real 1.77s`，18 个 body 成功，2 个 body 失败，`llvm-as` 通过。
4. `vsftpd --limit=10` 仍是 10 个 body 全部成功，lower `real 14.70s`。
5. `libuv --limit=20` 仍是 20 个 body 全部成功，lower `real 0.03s`。

风险：

1. non-void `RETURN` 缺 value input 时返回 `undef` 只是结构兜底，不代表语义完整。warning 已带 op id，后续要回到 Ghidra prototype / cold fragment 语义上确认。
2. `ret i32` 到 `ret i64` 当前按 zero extend 处理，符合现有 `resize(...)` 的风格；如果后续能从函数签名拿到 signedness，再考虑 signed extend。

评分：

1. 实现效果：7/10。解决了容易的 return verifier 问题，明显减少 `memcached` 失败数。
2. 复杂度：2/10。只改 return lowering，新增一个小 helper。
3. 维护成本：3/10。`undef` 兜底需要后续追语义，但 warning 已经具体到 op。
