# EVM native memory 当前状态

用户原始 prompt：

> 相关内容总结一下放到新的logs/文件里

## 当前状态

EVM memory IR 现在已经切到 native LLVM memory 形态：

- 测试 IR 中不再使用 `evm_mload` / `evm_mstore` / `evm_mstore8`。
- 主项目 `src/`、`include/` 和 `test/run_evm_solidity_patterns_suite.py` 中不再保留这些旧 helper 的兼容路径。
- Solidity pattern matcher 现在直接识别 `inttoptr i256 ... to ptr` 后的 `load` / `store`。
- EVM 函数 fixture 已补 `null_pointer_is_valid`，避免 LLVM 把 `store` 到 EVM 地址 0/4/64 当普通空指针 UB 删除。

相关提交：

- `df17212 Migrate EVM test IR to native memory ops`
  - 把 EVM 测试 IR 从旧 helper 调用迁到 native `load` / `store`。
  - 更新 EVM type recovery oracle。
- `44f261c Drop legacy EVM memory helper compatibility`
  - 删除主项目和 runner 里的旧 memory helper 兼容。
  - 更新 Solidity pattern / rewrite oracle。
- `e79e244 Ignore local build directory`
  - 在 `external/NotDec-evm2llvm` 子模块里把 `build/` 加入 `.gitignore`。
- `acfb1ba Update evm2llvm submodule gitignore`
  - 顶层更新 evm2llvm 子模块指针。

当前顶层工作树只剩无关状态：

- `external/NotDec-bin2llvm` 仍有本地 dirty，本轮没有碰。

## 已暂停的 marker 方向

按当前判断，下面这些 Solidity memory/data marker 先不继续推进：

- `notdec_solidity_memory_consumer`
- `notdec_solidity_memory_write`
- `notdec_solidity_memory_read`
- `notdec_solidity_memory_copy_write`
- `notdec_solidity_abi_return_data_word_write`
- `notdec_solidity_event_data_word_write`
- `notdec_solidity_external_call_input_word_write`
- 同类 ABI/event/external-call/revert data marker

原因：

- 这些 marker 和 native `load` / `store` / `calloc` 语义有较多重复。
- 当前生成逻辑容易把高层 role 过早写死。
- 更合理的方向是等类型推理有稳定承载方式后，再决定是否用 metadata、summary 或新的 fact 层表达这些 role。

保留方向：

- `calloc` / `calloc_unbounded` 作为 free memory allocation 的通用表达。
- Solidity rewrite marker 和 metadata 继续保留，用来表示已识别的 revert、checked-bounds、ABI return、event、external call 等高层事实。

## 验证结果

最近一次完整 EVM 验证：

```bash
ctest --test-dir build -R 'notdec.type_recovery.evm.tr_level_2|notdec.evm.solidity_patterns|notdec.evm.solidity_rewrite' --output-on-failure
```

结果：

- 3/3 通过。
- 总耗时 `189.92s`。

旧 helper 残留检查：

```bash
rg "evm_mload|evm_mstore|evm_mstore8|EVM_MLOAD|EVM_MSTORE" src include test/run_evm_solidity_patterns_suite.py -n
```

结果：无输出。

fortune 当前关注用例同口径：

```bash
/usr/bin/time -f 'elapsed=%e user=%U sys=%S maxrss=%M' \
  ./build/bin/notdec test/type-recovery/realworld/cases/fortune.o3.wasm.ll \
  -o /tmp/notdec-fortune-native-memory-no-helper-compat.ll \
  --tr-level=2 \
  --frozen-tr-input-ir \
  --dump-htypes=/tmp/notdec-fortune-native-memory-no-helper-compat.htypes
```

结果：

- 通过。
- `elapsed=12.58 user=12.18 sys=0.39 maxrss=852300`
- 和前面 12.5 秒级结果基本一致，没有明显性能退化。

## 当前风险

- 旧 helper 兼容已删，后续如果有外部旧 EVM IR 输入，会不再被这些 matcher 识别。
- Solidity pattern oracle 已按 native IR 重新计数，后续比较时要注意这些数字不是旧 helper 口径。
- 类型推理现在还没有完整消费 native EVM memory 的 offset / allocation object 关系。
- 动态 offset 不能急着恢复成 fixed field，否则容易把数组访问误当 struct 字段。

## 下一步建议

1. 先做 native load/store 的 MLsub 接入小闭环。
   - 目标是让 `inttoptr + load/store` 产生 EVM memory field 约束。
   - 先只处理常量 offset。

2. 更新 `test/type-recovery/evm/cases/04_evm_memory_helpers.ll`。
   - 这个 case 名字可以后续改掉，内容应从旧 helper 语义改成 native memory 语义。
   - oracle 应确认 htypes 能看到常量 offset 的关系。

3. 接 `calloc` / `ptrtoint` / `inttoptr` 的关系。
   - 先证明 allocation base 和后续 store/load 地址能连起来。
   - 不急着恢复 Solidity ABI role。

4. 动态 offset 先保守。
   - 可以记录为数组/unknown memory access。
   - 不要生成固定 record field。

5. 等类型恢复承载方式稳定后，再回头处理高层 role。
   - ABI return、event、external call、revert data 等 role 不建议现在恢复成旧 marker。
   - 后续再决定用 metadata、summary，还是专门的 fact 层。
