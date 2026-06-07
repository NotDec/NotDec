# 原始 prompt

把当前现状，和简化算法的思路单独写到一个logs/下的文档里面

本次调整 prompt：

```text
重新写一下这个plan。logs/20260607-01-type-simplifier-performance-plan.md 分三个思路，分别是原来的思路1，思路4，以及给CompactType比较增加Hash。
```

# 当前现状

当前性能问题集中在 `fortune.o3.wasm.ll` 的类型恢复阶段。触发点不是
`687cd6d1268b7d901ca09aabb25eb2f4338a580a` 这条规则本身是否该保留；这条规则是 bug
修复，需要保留。问题是它把更多 cast source 正确接入 MLsub 后，原来 `TypeSimplifier`
里的重复计算和按值比较成本被放大了。

同口径 fortune 结果：

| 版本 | 构建 | 时间 | RSS |
| --- | --- | ---: | ---: |
| `23b24a8` | Debug | `7.22s` | `167832 KB` |
| `23b24a8` | RelWithDebInfo + assert | `4.64s` | `163952 KB` |
| `687cd6d` | Debug | `59.87s` | `180780 KB` |
| `687cd6d` | RelWithDebInfo + assert | `8.21s` | `177064 KB` |

assertion 排查结果：

- Debug 下只给 `binarysub_lib` 关普通 `assert()`：`58.20s`。
- Debug 下给 `binarysub_lib` 同时关普通 `assert()` 和 `_GLIBCXX_ASSERTIONS`：`57.76s`。
- RelWithDebInfo 下普通 `assert()` 和 `_GLIBCXX_ASSERTIONS` 都开着仍是 `8.21s`。

所以主要问题不是 assertion，而是 Debug `-O0` 下 STL 模板、递归、按值比较和重复计算被放大。
Release 也有退化，只是优化器把大量模板调用内联掉了，所以没有 Debug 那么明显。

当前 Debug timing：

```text
bulkSimplifyDetailed roots=4437
canonicalize_ms=27169
struct_merge_ms=5606
local_simplify_ms=21002
analyze_ms=3155
origin_ms=625
simplify_ms=1200
coalesce_ms=15314
total_ms=53779
```

perf children 主要热点：

```text
63.21%  notdec::mlsub::MLsubRecovery::run
62.52%  notdec::mlsub::ConstraintsGenerator::genTypes
55.74%  binarysub::TypeSimplifier::bulkSimplifyDetailed
35.11%  binarysub::TypeSimplifier::canonicalizeType
29.80%  binarysub::operator<
29.58%  binarysub::CompactType::operator<
```

profile 文件：

- `/tmp/notdec-debug-profile-fortune/perf.data`
- `/tmp/notdec-debug-profile-fortune/reports/perf-flat.txt`
- `/tmp/notdec-debug-profile-fortune/reports/perf-children-summary.txt`
- `/tmp/notdec-debug-profile-fortune/reports/perf-children-callgraph.txt`

# 优化目标

短期目标是把 Debug fortune 从约 `60s` 明显降下来，同时不改变 `687cd6d` 的语义修复。
Release 已经在 `8s` 左右，但也需要确认优化不会让 HType 质量下降。

判断标准：

- `fortune.o3.wasm.ll --tr-level=2 --dump-htypes` 能跑完。
- Debug fortune 时间明显下降，优先看 `canonicalize_ms` 和 `total_ms`。
- RelWithDebInfo 不退化。
- `binarysub` 自测通过。
- 类型恢复相关 CTest 至少跑当前关注 suite。

# 简化算法思路

当前先把优化方向收敛成三类。它们都围绕 `bulkSimplifyDetailed()`，但缓存层级不同：

- `canonicalizeType` closure cache：缓存变量闭包，主要打 `canonicalize_ms`。
- bulk 后半段 cache：缓存 root 局部 simplify/coalesce 的结果，主要打 `local_simplify_ms` 和
  `coalesce_ms`。
- `CompactType` hash：降低按值比较成本，也可能让结构相同的递归片段更容易折叠。

这三类不要一开始混在一起做。先用统计确认重复和比较成本来自哪里，再逐步实现。

## 1. 给 `canonicalizeType` 的 closure 加局部 cache

`canonicalizeType()` 里变量节点会调用 `closeOver({ty}, pol)`，沿 `lowerBounds` 或
`upperBounds` 找闭包。当前 `bulkSimplifyDetailed()` 有 `4437` 个 roots，很可能重复计算同一个
变量和同一个 polarity 的 closure。

可行性判断：

- 在一次 `bulkSimplifyDetailed()` 内，约束已经生成完，`VariableState::lowerBounds` /
  `upperBounds` 理论上不再变化。
- `canonicalizeType()` 自己会写 `recursive`、`recVars`、`variableOrigins`，但不会改 subtype
  bounds。
- 所以 cache 可以放在单个 `TypeSimplifier` 生命周期内。
- 不能做全局长期 cache，因为后续如果还有 `constrain()` 改 bounds，旧 closure 会失效。

cache key 应该使用 identity：

```text
SimpleType* + polarity
```

不要用结构比较作为 key。

风险：

- 如果实际存在边 canonicalize 边改 bounds 的路径，cache 会错。实现前要确认
  `bulkSimplifyDetailed()` 调用期间没有新的 `constrain()`。
- cache 的 value 是 `SimpleTypeSet`，如果仍用按值排序的 set，收益会被打折；但这一步仍比重复
  closure 稳。

## 2. 给 bulk 后半段加 per-root / per-compact cache

`bulkSimplifyDetailed()` 后半段对每个 root 单独做：

```cpp
analyzeOccurrences(root)
simplifyType(root)
coalesceCompactType(localCompact)
```

timing 里 `local_simplify_ms=21002`、`coalesce_ms=15314`，说明这里也重。这里的 cache 和
思路 1 不同：思路 1 缓存的是变量闭包；这里缓存的是更大的 per-root 简化结果。

可能缓存的层级：

- `canonical CompactType root -> OccurrenceAnalysisState`
- `canonical CompactType root -> simplified CompactType`
- `canonical CompactType root + polarity -> UType`

```text
key = CompactType 语义 + polarity + recursive bindings + origin/id 策略
```

收益前提：

- 多个 root 的 canonical compact body 完全相同，或者共享大量子结构。
- `analyzeOccurrences`、`simplifyType`、`coalesceCompactType` 的结果不依赖 root 独有状态，或者这些状态
  可以合并。

难点：

- `variableOrigins` 要能合并，不能丢 origin。
- synthetic id 分配要稳定，否则输出 HType 会抖。
- recursive vars 的名字、size、origin 可能依赖 root。
- 如果 key 直接用 `CompactType` 按值比较，又会回到当前 `CompactType::operator<` 的成本问题。

所以这一步不要直接做大 cache。先加统计：

- canonical root 的结构去重数量。
- root 之间共享子图的比例。
- `analyzeOccurrences` / `simplifyType` / `coalesceCompactType` 各自是否重复处理相同 compact 结构。
- origin 合并后是否仍能保持 HType 可读性和 debug 信息。

如果重复率不高，这一步收益有限。如果重复率高，再从最小层级开始做，比如先缓存
`analyzeOccurrences`，不要一开始缓存最终 `UType`。

## 3. 给 `CompactType` 按值比较增加 hash

perf 里 `binarysub::operator<` 和 `CompactType::operator<` 占比高。当前 `CompactType::operator<`
是结构比较：

```cpp
std::tie(vars, prims, size, record, function, ptrLoad, ptrStore, psize)
```

这个比较会递归比较子 `CompactTypePtr`。Debug 下递归、STL 模板和按值比较都会被放大。可以考虑给
`CompactType` 增加结构 hash，让需要按值判断的地方先比 hash，减少深层 `<` 调用。

这不是简单把 `std::set` 换成 `std::unordered_set`：

- `unordered_set` 需要 hash 和 equality，不用 `operator<`。
- 如果仍然按值比较，hash 必须覆盖 `CompactType` 的所有语义字段。
- `CompactType` 构造期是可变的，参与 hash 的内容插入 hash 容器后不能再变。
- 递归结构不能直接递归算 hash，否则可能无限递归。

可行路线：

1. 先给 `value_ptr` 的比较加同指针快路径。这是低风险小优化：

```cpp
if (lhs.ptr_.get() == rhs.ptr_.get())
  return false;
```

2. 再加 `CompactType` 结构 hash 统计，不改变行为：

- 每次 `CompactType::operator<` 发生时，记录是否同指针、hash 是否不同、是否需要深比较。
- 统计 `record/function/ptrLoad/ptrStore` 递归比较深度。

3. 如果统计显示大量结构相等/相似比较，再实现 hash 缓存：

- hash 在 `CompactType` 冻结后计算。
- hash key 包含 `vars`、`prims`、`size`、record fields、function args/result、ptrLoad、ptrStore、
  `psize`。
- 递归情况需要稳定 token，不能无限展开。
- 合并结构相同的节点时，origin/id 取 union，不能丢失。

预期收益：

- 降低 `CompactType::operator<` 的递归比较成本。
- 支持后续 per-root cache 使用 structural hash key。
- 如果做 hash-consing，可能让递归类型输出更短。

风险：

- 递归 hash 正确性复杂。
- hash-consing 可能改变 HType 输出形态，需要 golden diff。
- 不能让可变 `CompactType` 带着旧 hash 继续变。
- 这一步可能比 closure cache 和局部 cache 更复杂，应该靠后。

## 备选：identity visited / inProcess 优化

原计划里提到过把递归检测的 `inProcess` 从按值比较改成 identity key：

```text
CompactType* + polarity
```

这个能减少递归路径检测的结构比较和 map/set 复制，但它会把“结构相等”改成“对象相同”。如果后续希望
利用结构相等让递归类型折叠得更短，就不能把它作为主要方向直接替换全部逻辑。

当前先把它作为对照方案：

- 如果目标只是提速且不关心结构相等折叠，可以在 `canonicalizeType::go1()` 先试 identity。
- 如果目标是兼顾折叠质量，则优先研究 structural hash，而不是直接改 identity。

# 建议顺序

1. 先加统计，不改行为：
   - `closeOver` 调用次数。
   - `(SimpleType*, polarity)` 去重后数量。
   - closure 平均大小和最大大小。
   - canonical root 的结构重复率。
   - root 之间共享 compact 子结构的比例。
   - `CompactType::operator<` 调用次数、递归深度、同指针比例。

2. 实现 `canonicalizeType` closure cache。

3. 评估 bulk 后半段是否值得 cache。优先考虑 `analyzeOccurrences`，不要直接缓存最终 `UType`。

4. 跑 Debug fortune 和 RelWithDebInfo fortune，对比：
   - `canonicalize_ms`
   - `local_simplify_ms`
   - `coalesce_ms`
   - `bulkSimplifyDetailed total_ms`
   - 总 elapsed
   - HType 输出是否有明显变化

5. 如果 `CompactType::operator<` 仍然是主要热点，再做结构 hash / hash-consing 方案。

6. identity visited / inProcess 优化作为对照或补充，不作为第一阶段主线。

# 实现记录：思路 1 closure cache

已实现 `canonicalizeType` 单变量 closure cache。

改动：

- `external/binarysub/include/binarysub/binarysub.h:373`，在 `TypeSimplifier` 增加
  `closureCache`，key 是 `TypeNode* + polarity`，value 是 `SimpleTypeSet`。
- `external/binarysub/include/binarysub/binarysub.h:410`，`isClear()` / `clear()` 纳入
  `closureCache`。
- `external/binarysub/src/binarysub.cpp:1009`，在 `TypeSimplifier::canonicalizeType()` 的
  `closeOver()` 中缓存单变量入口。多变量入口仍走原逻辑。

实现判断：

- cache 只放在 `TypeSimplifier` 生命周期内，不做全局 cache。
- `bulkSimplifyDetailed()` 期间约束图应已固定，`VariableState::lowerBounds` /
  `upperBounds` 不应再变。
- key 用 identity，避免 `SimpleType` 按值比较。

验证：

```bash
cmake --build ./build --target binarysub -j4
./build/binarysub
cmake --build ./build --target notdec -j4
/usr/bin/time -f 'elapsed=%e rss_kb=%M' \
  ./build/bin/notdec test/type-recovery/realworld/cases/fortune.o3.wasm.ll \
  -o /tmp/notdec-fortune-closure-cache.ll \
  --tr-level=2 \
  --dump-htypes=/tmp/notdec-fortune-closure-cache.htypes
ctest --test-dir build -R notdec.type_recovery.llvm_ir.tr_level_2 --output-on-failure
```

结果：

- `binarysub` 自测通过。
- fortune smoke 通过，生成 `/tmp/notdec-fortune-closure-cache.ll` 和
  `/tmp/notdec-fortune-closure-cache.htypes`。
- 本次 Debug fortune：`elapsed=57.20s`，`rss_kb=182684`。
- 对比计划里的旧 Debug fortune `59.87s`，closure cache 单独只有小幅下降，说明后续仍需要看
  `CompactType` 比较/hash 或 bulk 后半段 cache。
- `notdec.type_recovery.llvm_ir.tr_level_2` 初次运行是 10 passed / 11 failed。失败样例里
  `02_ConstantAddr1` 的 diff 是新增 `const(i32 1024)@main::%a:0` HType 条目，属于当前 worktree 的
  HType oracle 差异，不像 closure cache 的结构变化。
- 已同步这 11 个 `test/type-recovery/llvm-ir/expected/tr-level-2/*.htypes` oracle 后重跑：
  `notdec.type_recovery.llvm_ir.tr_level_2` 通过。

# 试验记录：思路 2 bulk 后半段 cache

尝试过一个保守版本，没有保留代码。

试验做法：

- 在 `bulkSimplifyDetailed()` 后半段，仍然对每个 root 运行 `analyzeOccurrences()`。
- 根据 `canonical CompactType root + local recursive bindings + polarity` 构造字符串 key。
- cache 命中时复用最终 `UTypePtr`，跳过 `simplifyType()` 和 `coalesceCompactType()`。
- `printDebug=true` 时禁用 cache，避免少打印 debug 信息。

验证：

```bash
cmake --build ./build --target notdec -j4
/usr/bin/time -f 'elapsed=%e rss_kb=%M' \
  ./build/bin/notdec test/type-recovery/realworld/cases/fortune.o3.wasm.ll \
  -o /tmp/notdec-fortune-local-cache.ll \
  --tr-level=2 \
  --dump-htypes=/tmp/notdec-fortune-local-cache.htypes

rm -rf /tmp/notdec-fortune-local-cache-work
NOTDEC_BINARYSUB_TRACE=1 /usr/bin/time -f 'elapsed=%e rss_kb=%M' \
  ./build/bin/notdec test/type-recovery/realworld/cases/fortune.o3.wasm.ll \
  -o /tmp/notdec-fortune-local-cache-trace.ll \
  --tr-level=2 \
  --dump-htypes=/tmp/notdec-fortune-local-cache-trace.htypes \
  --gen-work-dir \
  --work-dir=/tmp/notdec-fortune-local-cache-work
```

结果：

- 普通 fortune：`elapsed=57.29s`，`rss_kb=185160`，相比 closure cache 后的 `57.20s` 没有收益。
- trace fortune：主 bulk `roots=4437`，cache `entries=4411`，`hits=26`，`misses=4411`。
- 命中率太低，字符串 key 构造还会增加成本，所以撤回这版代码。

判断：

- 直接按完整 root 结果做 per-root final cache 不值得。
- 如果继续做思路 2，应该先做更细粒度统计，或者结合 `CompactType` structural hash / hash-consing。
- 当前更值得继续的是思路 3：减少 `CompactType::operator<` 的深层按值比较成本。

# 当前不做

- 不回退 `687cd6d`。
- 不通过修改 Debug 优化等级解决问题。
- 不先做全局长期 cache。
- 不先重写 `TypeSimplifier` 整体算法。
- 不先把所有 `CompactType` 比较改成 identity；结构相等对递归类型折叠可能有价值。
