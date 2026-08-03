# binarysub occurrence allocation 优化实现记录

> 用户原始 prompt：
>
> 首先，之前的性能改进的效果怎么样？包括时间内内存上，既然这个问题是之前就有的，那就先收尾之前的性能改进。然后再看这个问题，这个问题不要通过保证确定性去解决，而是要思考算法上的问题。为什么@4的字段会丢？

## 范围

本记录只收尾 `CompactType` 简化中的分配优化，不修改 PNDiff 的求解语义。
PNDiff 的 `@4` 退化另行分析，不能用固定 worklist 顺序掩盖。

## 实现

- `external/binarysub/include/binarysub/binarysub.h:527-537` 和
  `src/binarysub.cpp:665-836,1148-1186`：`CompactTypeBuilder` 记住同一
  `CompactTypeArena` 中最大的输入 `CompactVarSet` root。freeze 时 root 已经
  覆盖全部变量就直接复用；否则只向该 root 插入差集。跨 arena root 仍完整导入，
  因为两个 persistent factory 的 child 引用计数不能混用。
- `external/binarysub/include/binarysub/binarysub.h:687-699` 和
  `src/binarysub.cpp:2415-2558,2697-2802`：occurrence 变量集合改成共享的、不可变
  `vector<SimpleType>` 快照。一个 `CompactType` 的每个变量共用同一份快照；交集使用
  有序双指针扫描，子集时直接复用较小快照；查找改为 `binary_search`。primitive 集合
  没有改变。
- `external/binarysub/src/binarysub-test.cpp:813-838` 验证同 arena root 不增加
  persistent AVL 槽位，跨 arena 会安全导入；`:1020-1065` 验证 snapshot 共享和交集
  结果。测试入口在 `include/binarysub/binarysub-test.h:18` 与
  `src/binarysub-test-main.cpp:47`。

## 测量

输入为 ngIRCd 的
`/sn640/NotDec-Exp/Bench2/source-ir/ir/ngircd/ngircd.bc`，`--tr-level=2`，
`--merge-struct-ptr-load-store`，8 个 binarysub 线程。所有 run 的 `out.ll` SHA-256
均为 `cc9adf69bca00bca6e168911ce690f135f5d6f9b408452400d7199fe21b15733`，LLVM 22
verifier 通过。

| 指标 | 旧 occurrence set | 共享 vector snapshot | 结论 |
| --- | ---: | ---: | --- |
| jemalloc 累计分配 | 53.96 GB | 12.28 GB | -77.2% |
| `TypeSimplifier::analyzeOccurrences` 累计分配 | 42.09 GB | 287.42 MB | -99.3% |
| 原生 allocator 最大 RSS | 3,611,556 KiB | 3,612,944 KiB | 基本不变 |
| 原生 allocator wall | 31.10 s | 31.27 s | 基本不变 |

最后两项不是回归结论：同一当前版本的重复 run 为 29.91 s，单次 0.17 s 的差别低于
机器波动。此次消掉的是频繁创建后释放的红黑树节点，因此累计分配大幅下降，而峰值仍由
canonicalize/`mergeAll` 保留的 CompactType 图决定。root 复用有单元测试保证，但本轮
没有单独剥离它的 profile，不把总收益错误归因给它。

累计 profile 的旧热点是 `std::set::insert` / `std::set_intersection`，其中
`analyzeOccurrences()` 占 42.09 GB；新 profile 中该函数只占 287.42 MB。新的主要
累计分配转为 `CompactTypeArena::mergeAll`、`canonicalizeType` 和 builder freeze，
这也是后续内存工作应关注的 live-memory 路径。

## 验证

- `cmake --build build-relwithdebinfo-20260731 --target binarysub -j4`
- `cmake --build build-relwithdebinfo-20260731 --target notdec -j4`
- `./build-relwithdebinfo-20260731/binarysub`
- ngIRCd 完整运行、输出哈希比对和 LLVM 22 `opt -passes=verify`

## 评估

实现效果 8/10：大量 allocator 流量消失，但不减少该 workload 的峰值常驻内存。
理解成本 3/10：数据仍是有序唯一集合，只是明确区分持久 AVL root 和只读快照。
维护成本 2/10：交集和查找集中在小型 helper，覆盖共享、交集、跨 arena 三种边界。
