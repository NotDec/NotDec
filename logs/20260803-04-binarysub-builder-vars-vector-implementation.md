# binarysub builder 变量 vector 累积实现记录

> 用户原始 prompt：
>
> 第一点可以直接改了，也先不做测试，改代码就行，然后思考一下第二点这个要改的话可能可以怎么改，好不好改

## 背景

ngIRCd 的累计分配中，`CompactTypeBuilder` 的 `SimpleVarSet` 红黑树节点是主要
热点之一。builder 只会追加变量，真正发布 `CompactType` 时才需要稳定排序、去重和
不可变 `CompactVarSet`。

## 实现

- `external/binarysub/src/binarysub.cpp:693-765`：三个 builder 合并路径改为向
  `std::vector<SimpleType>` 追加变量，不再对临时 `SimpleVarSet` 逐项分配。
- `external/binarysub/src/binarysub.cpp:886-914`、`944-956`：`freeze()` 前按
  `SimpleTypeStableIdentityLess` 排序去重，恢复旧的稳定 set 语义后才创建
  `CompactVarSet`。
- `external/binarysub/src/binarysub.cpp:1287-1320`、
  `external/binarysub/include/binarysub/binarysub.h:527-536`：
  `makeVarSetWithBase()` 直接接收已规范化 vector；仍从本 arena 的最大输入 root
  增量建立 persistent AVL set，或在集合相同的时候直接复用 root。

最终 `CompactVarSet` 的内容、顺序、所有权和 `CompactType` 的 hash 行为没有改变；
仅移除了 builder 生命周期内的临时红黑树。

## 实测

实现后在相同的 ngIRCd 输入上实测：
`/sn640/NotDec-Exp/Bench2/source-ir/ir/ngircd/ngircd.bc`，参数为
`--tr-level=2 --merge-struct-ptr-load-store -g --fast-work-dir`，
`NOTDEC_BINARYSUB_THREADS=8`。两次 native allocator 的输出 SHA-256 都是
`cc9adf69bca00bca6e168911ce690f135f5d6f9b408452400d7199fe21b15733`。

| 指标 | SimpleVarSet 基线 | vector 实现 | 变化 |
| --- | ---: | ---: | ---: |
| peak RSS（两次均值） | 2,402,350 KiB | 2,398,294 KiB | -4,056 KiB（-0.17%） |
| peak RSS（范围） | 2,399,248-2,405,452 KiB | 2,398,076-2,398,512 KiB | 落在基线波动附近 |
| jemalloc `alloc_space` | 8,319,138,182 B | 10,834,284,033 B | +30.2% |

RSS 的差异没有实际意义。jemalloc 的 `alloc_space` 是 512 KiB Poisson 采样下的累计
分配估计，单次结果不能替代 RSS，但这里的热点变化能解释方向：

- `std::_Rb_tree::_M_create_node` / `_M_get_node` 从约 3.30 GB 降到约 1.26 GB，说明
  builder 的临时 `SimpleVarSet` 节点确实减少了。
- 但 `std::vector::_M_range_insert` 从约 1.8 MiB 升到约 4.77 GB，其中
  `CompactTypeBuilder::appendRecordField()` 约 4.53 GB。它调用的
  `CompactTypeBuilder::append()`（`external/binarysub/src/binarysub.cpp:970-974` 到
  `693-733`）会把每个子字段已有的 `rhs->vars` 复制进新 vector；大量 child builder 的
  扩容和搬迁超过了省下的红黑树节点。

结论：`external/binarysub/src/binarysub.cpp:693-765` 和 `944-956` 的 vector 改动没有
降低实际峰值 RSS，且明显增加累计分配，不能作为当前内存优化保留。下一步应先决定回退，
或改为避免每个 child builder 都分配独立连续 `vars` buffer 的方案，再重新测量。

## 验证

- `cmake --build build-relwithdebinfo-20260731 --target binarysub notdec -j4`
- ngIRCd 两次完整 native allocator 运行、SHA-256 比对
- jemalloc `prof_accum:true,lg_prof_sample:19` 的 final heap，以 `/opt/addr2line`
  生成 `jeprof --alloc_space --cum` 报告
- `git -C external/binarysub diff --check`
