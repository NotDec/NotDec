# 原始 prompt

把当前现状，和简化算法的思路单独写到一个logs/下的文档里面

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

## 2. 把递归检测里的按值比较换成 identity key

perf 里 `binarysub::operator<` 和 `CompactType::operator<` 占比很高。很多场景只是判断某个对象
是否在当前路径里，不需要按值比较：

- `canonicalizeType::go1()` 的 `inProcess`
- 遍历和递归检测里的 seen set
- 只关心对象身份的 visited 集合

这类结构可以换成：

```text
CompactType* + polarity
```

配 `unordered_set` / `unordered_map`。

目标不是删除 `CompactType::operator<`。按值比较在需要稳定排序、合并或输出时仍可能有用。优化点是：
只要语义上是“当前对象是否访问过”，就不要走递归结构比较。

风险：

- 必须区分“对象身份”与“结构相等”。如果某处依赖结构相等去合并等价类型，不能改成 identity。
- 第一阶段只改递归检测和 visited，不动真正的结构排序。

## 3. 减少递归时复制 `inProcess`

当前有类似代码：

```cpp
auto newInProcess = inProcess;
newInProcess.insert(pty);
```

这会在递归每层复制 set/map。Debug 下复制成本和随后的结构比较都会被放大。

`canonicalizeType::go1()` 可以先考虑 mutable set + RAII push/pop：

```text
进入递归前 insert
退出时 erase
```

这里 `inProcess` 只是当前递归路径，语义比较直接。

`coalesceCompactType::go()` 更复杂，因为 `inProcess` 里存的是 `recVarGetter`，getter 捕获当前递归点
的局部状态。这里不能简单改成一个全局 set。更接近原语义的做法是 persistent frame：

```text
Frame { key, recVarGetter, parent }
```

查找时沿 parent 链往上找。这样不复制整张 map，又保留“当前路径上每个递归点有自己的 getter”的语义。

风险：

- persistent frame 的 lookup 是 O(depth)。如果递归深度很大，可能不一定比复制 map 快。
- `coalesceCompactType()` 的递归变量生成逻辑比较敏感，应该排在 `canonicalizeType` 优化之后。

## 4. per-root local simplify cache 暂时靠后

`bulkSimplifyDetailed()` 后半段对每个 root 单独做：

```cpp
analyzeOccurrences(root)
simplifyType(root)
coalesceCompactType(localCompact)
```

timing 里 `local_simplify_ms=21002`、`coalesce_ms=15314`，说明这里也重。

所谓 per-root cache，是考虑如果多个 root 的 canonical compact body 相同或共享大量子图，能不能复用
`analyzeOccurrences`、`simplifyType` 或 `coalesceCompactType` 的结果。

这一步暂时靠后，因为结果受这些因素影响：

- polarity
- reachable recursive vars
- `variableOrigins`
- synthetic id 分配
- root 自己的显示变量名和 origin

cache key 不好定，容易复用错结果。前面 identity key 和 closure cache 做完后，再看是否仍然需要这一步。

# 建议顺序

1. 先加统计，不改行为：
   - `closeOver` 调用次数。
   - `(SimpleType*, polarity)` 去重后数量。
   - closure 平均大小和最大大小。
   - `canonicalizeType::go1()` 的 `inProcess` 最大深度和复制次数。
   - `CompactType::operator<` 是否主要来自递归检测。

2. 实现 `canonicalizeType` closure cache。

3. 把 `canonicalizeType::go1()` 的 `inProcess` 改成 identity hash + push/pop。

4. 跑 Debug fortune 和 RelWithDebInfo fortune，对比：
   - `canonicalize_ms`
   - `bulkSimplifyDetailed total_ms`
   - 总 elapsed
   - HType 输出是否有明显变化

5. 如果仍慢，再评估 `coalesceCompactType()` 的 persistent frame。

6. 最后才考虑 per-root local simplify cache。

# 当前不做

- 不回退 `687cd6d`。
- 不通过修改 Debug 优化等级解决问题。
- 不先做全局长期 cache。
- 不先重写 `TypeSimplifier` 整体算法。
