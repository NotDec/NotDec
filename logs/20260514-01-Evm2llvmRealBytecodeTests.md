# evm2llvm 真实字节码测试计划和记录

## 背景

之前 `external/NotDec-evm2llvm/test/fixtures/` 里的测试主要是手写 CSV。
这类测试能快速覆盖 loader、SSA value、PHI 和 private call，但不能说明真实
Gigahorse 输出能被 evm2llvm 接住。

Gigahorse 仓库已有真实 `.hex` 测试集，位置是：

```text
/sn640/gigahorse-toolchain/tests/
/sn640/gigahorse-toolchain/examples/
```

当前先不一次性删除 CSV fixture。保留少量快测，新增默认关闭的真实 bytecode
测试组，用来做更可信的端到端验证。

## 目标

1. wrapper 从 `.hex` 跑 Gigahorse，再把 facts 交给 evm2llvm。
2. CTest 能注册真实 `.hex -> facts -> .ll -> .bc -> verify` 流程。
3. 默认测试仍然快；真实 bytecode 测试通过开关打开。
4. 慢样例单独放一组，避免普通验证被拖慢。

## 实现记录

修改文件和函数：

1. `external/NotDec-evm2llvm/scripts/notdec-evm2llvm.py:66`
   - 修改 `build_gigahorse_command`。
   - 给 Gigahorse 加 `--results_file <work_dir>/results.json`。
   - 原因是本地 Gigahorse 默认会写 checkout 根目录的 `results.json`，在当前环境会因为权限失败；facts 已生成但 wrapper 会中断。

2. `external/NotDec-evm2llvm/CMakeLists.txt:20`
   - 新增 `NOTDEC_EVM2LLVM_ENABLE_GIGAHORSE_SLOW_TESTS`。
   - 新增 `NOTDEC_EVM2LLVM_GIGAHORSE_ROOT` cache path，默认 `/sn640/gigahorse-toolchain`。

3. `external/NotDec-evm2llvm/test/CMakeLists.txt:42`
   - 新增 `add_evm2llvm_gigahorse_test`。
   - 每个样例注册三步：
     - wrapper emit `.ll`
     - `llvm-as`
     - `opt -passes=verify`

4. `external/NotDec-evm2llvm/test/CMakeLists.txt:91`
   - 在 `NOTDEC_EVM2LLVM_ENABLE_GIGAHORSE_TESTS=ON` 时注册小样例：
     - 手写 bytecode `arithmetic`
     - Gigahorse `tests/via-ir/simple-storage.hex`
     - Gigahorse `tests/guards/owner-guard.hex`
     - Gigahorse `tests/storage/map1.hex`

5. `external/NotDec-evm2llvm/test/CMakeLists.txt:108`
   - 在 `NOTDEC_EVM2LLVM_ENABLE_GIGAHORSE_SLOW_TESTS=ON` 时注册：
     - Gigahorse `examples/long_running.hex`

## 验证

配置和注册：

```bash
cmake -S external/NotDec-evm2llvm -B build-evm2llvm -G Ninja \
  -DNOTDEC_EVM2LLVM_ENABLE_GIGAHORSE_TESTS=ON
ctest --test-dir build-evm2llvm -N -R evm2llvm.gigahorse
```

结果：

```text
注册 12 个 Gigahorse 测试：
arithmetic / simple-storage / owner-guard / map1
每个样例包含 emit、llvm-as、opt-verify 三步。
```

最小真实 bytecode：

```bash
cmake --build build-evm2llvm --target evm2llvm -j4
ctest --test-dir build-evm2llvm \
  -R 'evm2llvm.gigahorse.simple-storage' --output-on-failure
```

结果：

```text
3/3 passed
emit 用时约 34.03s
llvm-as / opt-verify 通过
```

原 CSV fixture：

```bash
ctest --test-dir build-evm2llvm -R evm2llvm.fixture --output-on-failure
```

结果：

```text
15/15 passed
```

## 后续工作

1. 跑 `owner-guard` 和 `map1`，确认当前 opcode 覆盖情况。
2. 按失败点补真实合约常见 opcode 或 facts 校验。
3. 稳定后减少手写 CSV fixture，只保留能快速定位 SSA/PHI/private call 的少数样例。
4. 需要全链路信心时打开 slow 组跑 `long_running`。

## apehex 真实链上样本鲁棒性检查

数据来自 `/sn640/NotDecChainExp/apehex_evm_contracts/hex/ethereum/cleaned`。
先从 `0000.parquet` 按 runtime bytecode 大小抽 8 个样本：

- tiny: 2 个，171 bytes
- small: 2 个，298 / 323 bytes
- medium: 2 个，3313 / 2141 bytes
- large: 2 个，11293 / 11293 bytes

生成文件：

- manifest: `/sn640/NotDecChainExp/evm2llvm_apehex_pilot/manifest.csv`
- 运行结果: `/sn640/NotDecChainExp/evm2llvm_apehex_pilot/summary_cached.csv`
- 单样本日志: `/sn640/NotDecChainExp/evm2llvm_apehex_pilot/logs/*.cached.log`

第一次跑时误用了 Python `subprocess.run(..., timeout=100)`。超时后 wrapper 退出了，
但 Gigahorse / Souffle 子进程还在后台继续编译，影响后续测试。已手动杀掉该进程组。
后续批量测试必须用 `timeout -k` 包住整条 wrapper 命令，或者让 runner 自己创建进程组并杀
整个进程组。

Gigahorse 首次运行会编译 Souffle 程序，不应计入样本分析时间。本机这次耗时：

- `last_resort.dl`: 249.38s
- `fallback_scalable.dl`: 254.44s
- `function_inliner.dl`: 327.69s
- `main.dl`: 361.14s

缓存完成后，8 个样本的 Gigahorse 都能在 1.6s 到 4.7s 左右产出 facts，`client_timeouts=0`。
失败集中在 evm2llvm：

| 样本 | 大小 | 结果 | 失败点 |
| --- | ---: | --- | --- |
| `01_tiny_c68dbd0999` | 171 | fail | `unsupported opcode DELEGATECALL at 0x5e` |
| `02_tiny_6a9bc8606b` | 171 | fail | `unsupported opcode DELEGATECALL at 0x5e` |
| `03_small_9752b87bf2` | 298 | fail | `unsupported opcode DELEGATECALL at 0xdf0x0` |
| `04_small_9bda9774f4` | 323 | fail | evm2llvm 产出 `.ll`，但 `llvm-as` 报 `expected type` |
| `05_medium_233cfe3212` | 3313 | fail | evm2llvm 非 0 退出，需继续细分 stderr |
| `06_medium_fe9f436f05` | 2141 | fail | evm2llvm 非 0 退出，需继续细分 stderr |
| `07_large_0994def38c` | 11293 | fail | evm2llvm 非 0 退出，需继续细分 stderr |
| `08_large_9fc75dd266` | 11293 | fail | evm2llvm 非 0 退出，需继续细分 stderr |

当前判断：

1. Gigahorse 接 apehex runtime bytecode 没有明显问题，首次慢主要是 Souffle 编译缓存。
2. evm2llvm 当前 opcode 覆盖不足，`DELEGATECALL` 是最先暴露的真实链上高频缺口。
3. `04_small` 用 pilot 脚本里的 `/usr/bin/llvm-as` 失败，是因为系统工具还是 LLVM 14；
   当前项目应使用 `/sn640/NotDec/llvm-22.1.0.obj/bin/llvm-as`。用 LLVM 22 复查后该样本
   `llvm-as` 和 `opt -passes=verify` 均通过。
4. 后续应该先补 `DELEGATECALL`，再把 wrapper 失败日志改成保留 evm2llvm stderr 的第一条真实错误，
   否则大批量统计会被 `CalledProcessError` 淹没。

## 性能影响

本次只改 `external/NotDec-evm2llvm` 的 wrapper 和测试注册。
没有改 NotDec 主 pass pipeline、类型恢复、结构体合并或 pointer analysis。
因此没有跑 fortune 当前关注用例。

当前方案评分：

- 实现效果：8/10。真实 bytecode 测试入口已经可用，最小样例通过。
- 复杂度：4/10。只是 CTest 封装和 wrapper 参数修正。
- 维护成本：5/10。真实 Gigahorse 测试依赖本地工具链和运行时间，但比手写 CSV 更可信。
