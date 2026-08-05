# XMM 浮点参数证据收紧实现记录

> 用户原始 prompt：
>
> 按这个改一下试试

## 背景

给未知外部函数推断 arity 时，浮点参数证据来自 XMM 槽（ZMM global 低 64 位）。实测 wrk
时发现严重过估：`lua_createtable` 被推断成 7 个 double 参数（真实 `(i64, i64, i64)`），
warning 从 1042 膨胀到 1356。

根因：二进制里编译器用 `movq xmm,rax` + `punpcklqdq` 做整数打包暂存，会写 XMM0-6；
base summary 把这些槽都判成 `LocalDefinition`，被当成浮点参数。`lua_createtable` 调用点
之前根本没有浮点写，整数参数 `L` 是透传 entry 值（不算 Local），TypedParams 又优先选
浮点多的调用点，最后变成 7 个 double 且丢第一个真参。

## 实现

- `lib/PcodeToLLVM.cpp`：新增 `isSseFloatWriteMnemonic()`（MOVSD/MOVSS/MOVAP*、CVT*、
  标量/打包算术、比较、浮点域逻辑等 SSE 浮点语义指令）；`prepareSseFloatWriteMarkers()`
  按指令 mnemonic 找出写 XMM 的浮点指令的 pcode op；主循环里 `write()` 给这些寄存器写
  打 `notdec.register.float_write` metadata。movq/punpcklqdq/pxor 等整数打包不打标。
- `lib/RegisterStorage.cpp` / `include/notdec-bin2llvm/RegisterStorage.h`：
  `write()`/`addAccessMetadata()` 增加 `isFloatWrite` 参数，写入 metadata。
- `lib/passes/summary/NativeRegisterSummary.cpp`：`RegisterOriginBits` 增加
  `FloatLocal`（Local 中"最后一次写是浮点指令"的子集）；store/partial-write 分支按
  metadata 维护它；`callsiteOrigin()` 对浮点槽要求 `FloatLocal` 全置，否则返回 `Mixed`，
  不参与参数推断。
- `lib/passes/summary/NativeRegisterSummarySSA.cpp`：`inferExternalCallShapes()` 只在
  `floatCount > 0` 时才报 `float_unknown_external_arity_stopped_at_call_clobber`，去掉
  ZMM0 在函数入口就是 call-clobber 的 179 条噪音。整数侧保持只认 Local（基线
  `lua_createtable` arity=3 本来就正确，不需要放宽 ForwardedEntry）。

## 验证

wrk 全量（`--all-confirmed --skip-runtime`）：

- `lua_createtable` 恢复 `declare i64 @lua_createtable(i64, i64, i64)`，调用点
  `call i64 @lua_createtable(i64 %RDI.arg, i64 0, i64 0)`。
- `round` 保持 `declare i64 @round(double)`（movsd 写的 XMM0 是真浮点参数）。
- `lua_pushnumber` arity=1..2/final=2（真实 `(i64, double)`），比之前过估到 4 好。
- warning 1356 → 1042（基线 1043，内容差异主要是 round 从 unresolved 变为
  inferred、lua_pushnumber 相关 RSI clobber warning 减少；新增少量 printf 的
  ZMM0 clobber 残留噪点，属于浮点槽进入 vararg 绑定的副产品）。
- `llvm-as` + `opt -passes=verify` 通过。
- `ctest --test-dir build -R native` 15/15 通过（含 fortune x86_64 / i386）。
