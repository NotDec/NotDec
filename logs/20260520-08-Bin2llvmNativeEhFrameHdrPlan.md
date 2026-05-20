# 2026-05-20 Bin2LLVM native eh_frame_hdr 校验计划

## 用户原始 prompt

```text
继续推进
```

## 背景

上一阶段已经在 native discovery 中接入 `.eh_frame`：

1. `EhFrameAnalyzer` 能线性解析 `.eh_frame`。
2. libuv 当前结果是 `FDE: 480`、`parsed FDE: 480`。
3. `eh-frame` source 已经进入 function seed，seed 总数从 311 增到 484。
4. range source 也已经能显示为 `eh-frame`。

但 `.eh_frame_hdr` 现在只记录存在，没有解析 binary search table。

在后续把 eh_frame range 用作 decode 边界前，需要先确认 `.eh_frame_hdr` 和 `.eh_frame` 是否一致。这样可以提前发现 reader 错位、table 编码不支持、FDE table 与 `.eh_frame` 实体不一致等问题。

## 目标

本阶段只做 `.eh_frame_hdr` table 解析和交叉校验：

1. 读取 `.eh_frame_hdr` header。
2. 解析 `eh_frame_ptr_enc`、`fde_count_enc`、`table_enc`。
3. 解析 FDE count。
4. 解析 binary search table 的 `initial_location` 和 `fde_address`。
5. 与 `.eh_frame` 扫描得到的 FDE starts / record addresses 对比。
6. report 输出 count、match、missing、extra、unsupported、invalid 和样例。

## 技术路线

复用当前 `EhFrameAnalyzer` 里的最小 DWARF EH encoding reader。

重点：

1. `.eh_frame_hdr` 的 table 地址字段也使用 DW_EH_PE 编码。
2. `initial_location` 是函数起始地址。
3. `fde_address` 应该指向 `.eh_frame` 中的 FDE record。
4. `.eh_frame` reader 需要把每个 FDE 的 `pc begin` 和 FDE record address 记录到 stats 里。
5. `.eh_frame_hdr` 解析完后，用 set 做交叉校验。

第一版只支持当前 libuv 常见 encoding：

1. `absptr`
2. `pcrel`
3. `udata4`
4. `sdata4`
5. `udata8`
6. `sdata8`
7. `omit`

## 判断标准

短期验收：

1. `notdec-native-discover` 能跑 libuv，不崩。
2. report 出现 `.eh_frame_hdr` table count。
3. libuv 的 hdr table count 与 `.eh_frame` FDE count 一致。
4. hdr table initial_location 能全部匹配 `.eh_frame` 的 FDE start。
5. invalid / unsupported 为 0，或者明确输出样例。
6. seed 总数不因为本阶段变化而减少。

## 不做什么

1. 不把 `.eh_frame_hdr` table 当作新的 seed 来源。
2. 不用 hdr 覆盖 `.eh_frame` range。
3. 不处理 LSDA。
4. 不处理 landing pad。
5. 不接递归 decode。
6. 不做 `.debug_frame`。

## 风险

1. DW_EH_PE pcrel base 很容易写错，必须按字段所在地址计算。
2. `.eh_frame_hdr` table 的 FDE 地址可能和 `.eh_frame` record address 有偏差，report 要能看见。
3. 当前 reader 如果继续塞在 `NativeAnalysis.cpp`，文件会变大；本阶段先不拆，避免引入文件组织变动。
4. 如果遇到 unsupported encoding，不能猜，必须跳过并 report。

## 性能影响

1. `.eh_frame_hdr` table 是线性读，数量和 FDE 数相同。
2. 当前只接 `notdec-native-discover`。
3. 不影响主 NotDec pass pipeline，不需要对 fortune 当前关注用例做同口径时间对比。

## 实现记录（2026-05-20，已完成）

### 改动文件和函数

1. `external/NotDec-bin2llvm/include/notdec-bin2llvm/NativeAnalysis.h:57`
   - 新增 `NativeEhFrameFdeInfo`，记录 `.eh_frame` 解析出的 `PcBegin` 和 FDE record address。
2. `external/NotDec-bin2llvm/include/notdec-bin2llvm/NativeAnalysis.h:62`
   - 新增 `NativeEhFrameHdrEntry`，记录 `.eh_frame_hdr` table 的 `InitialLocation` 和 `FdeAddress`。
3. `external/NotDec-bin2llvm/include/notdec-bin2llvm/NativeAnalysis.h:69`
   - 扩展 `NativeEhFrameStats`，增加 hdr parsed、hdr count、match/missing/mismatch、hdr invalid/unsupported 和样例字段。
4. `external/NotDec-bin2llvm/lib/NativeAnalysis.cpp:373`
   - `EhFrameAnalyzer::run()` 在 `.eh_frame` 后解析 `.eh_frame_hdr`，并调用 `compareEhFrameHdr(...)`。
5. `external/NotDec-bin2llvm/lib/NativeAnalysis.cpp:415`
   - 增加 `DW_EH_PE_datarel` 支持。实际 libuv `.eh_frame_hdr` table encoding 是 `0x3b`，也就是 `datarel | sdata4`，这是计划阶段漏掉的。
6. `external/NotDec-bin2llvm/lib/NativeAnalysis.cpp:429`
   - `EhFrameReader::parse()` 继续负责 `.eh_frame` 线性扫描，同时把 FDE record address 传给 `parseFde(...)`。
7. `external/NotDec-bin2llvm/lib/NativeAnalysis.cpp:839`
   - `EhFrameReader::parseFde()` 记录 `{pcBegin, FDE record address}` 到 `FrameFdes`。
8. `external/NotDec-bin2llvm/lib/NativeAnalysis.cpp:850`
   - 新增 `EhFrameHdrReader`，读取 version、eh_frame pointer、FDE count、table entries。
   - `datarel` base 按 `.eh_frame_hdr` section start 计算；`pcrel` base 按字段地址计算。
9. `external/NotDec-bin2llvm/lib/NativeAnalysis.cpp:1075`
   - 新增 `compareEhFrameHdr(...)`，按 start 和 FDE record address 对比 hdr table 与 `.eh_frame` 扫描结果。
10. `external/NotDec-bin2llvm/lib/NativeAnalysis.cpp:1191`
    - `ReportAnalyzer::run()` 输出 hdr parsed、hdr table count、matched starts、missing、FDE address match/mismatch、hdr invalid/unsupported。

### 验证

构建：

```bash
cmake -S external/NotDec-bin2llvm -B external/NotDec-bin2llvm/build-native -G Ninja \
  -DNOTDEC_BIN2LLVM_ENABLE_LIEF=ON \
  -DNOTDEC_BIN2LLVM_ENABLE_SLEIGH=ON \
  -DNOTDEC_BIN2LLVM_SLEIGH_SOURCE_DIR=/sn640/sleigh \
  -DLLVM_DIR=/sn640/NotDec/llvm-22.1.0.obj/lib/cmake/llvm \
  -DCMAKE_EXPORT_COMPILE_COMMANDS=TRUE
cmake --build external/NotDec-bin2llvm/build-native --target notdec-native-discover -j4
```

libuv：

```bash
external/NotDec-bin2llvm/build-native/bin/notdec-native-discover \
  /sn640/NotDec-Exp/Bench2/rootfs/usr/lib/x86_64-linux-gnu/libuv.so.1.0.0 \
  > /tmp/notdec-native-libuv-ehframe-hdr-report.txt
```

关键结果：

1. `function seeds: 484`，和上一阶段一致，没有减少。
2. `parsed hdr: yes`。
3. `FDE: 480`，`parsed FDE: 480`。
4. `hdr FDE count: 480`，`hdr table entries: 480`。
5. `hdr matched starts: 480`。
6. `hdr missing in frame: 0`。
7. `frame FDEs missing in hdr: 0`。
8. `hdr FDE address matches: 480`。
9. `hdr FDE address mismatches: 0`。
10. `invalid: 0`，`unsupported: 0`，`hdr invalid: 0`，`hdr unsupported: 0`。

CTest：

```bash
ctest --test-dir external/NotDec-bin2llvm/build-native \
  -R notdec.native_discover.x86_64_smoke --output-on-failure
```

结果：通过，`1/1`。

### 性能和维护评估

1. 实现效果：9/10。libuv 的 `.eh_frame_hdr` table 与 `.eh_frame` 扫描结果完全一致，可以作为后续 decode 边界接入前的 sanity check。
2. 复杂度：6/10。新增了 hdr reader，但没有引入新的分析阶段，也没有改变 seed 来源。
3. 维护成本：6/10。`NativeAnalysis.cpp` 继续变大，下一步如果接 decode 边界或支持更多 encoding，应该考虑把 eh_frame reader 拆到独立文件。

更好的方案是拆出独立 `NativeEhFrame.cpp`，但本阶段先保持小范围改动，避免同时做文件组织调整和语义变更。
