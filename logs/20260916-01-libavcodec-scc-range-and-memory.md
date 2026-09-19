# libavcodec: SCC 下标越界崩溃 + 类型恢复内存增长

日期：2026-09-16

## 背景

microsub2 接入 Bench2 评估框架后，`ffmpeg-libavcodec`（16 MB 共享库）在
NotDec 类型恢复阶段以异常结束：

```
[microsub2] type recovery failed: NotDec type recovery stopped:
vector::_M_range_check: __n (which is 1) >= this->size() (which is 1)
```

异常文本说明是 `std::vector::at(1)` 打在 size==1 的 vector 上（`std::array::at`
与 `map::at` 的报错文本不同，可排除；`BottomUpTiming` 用的是 `std::array`）。
`ffmpeg-libavfilter` 在同一阶段被杀（rc=137）。

## 改动

`src/TypeRecovery/mlsub/MLsubGenerator.cpp`：

1. `getFuncCG()`（约 2668 行）：原来 `count(CGN)` 之后直接
   `AG.AllSCCs.at(AG.Func2SCCIndex.at(CGN))`，map 查询有保护但**下标没有范围
   检查**。改成一次 `find`，下标 `>= AG.AllSCCs.size()` 时和"节点不在
   SCC 分区里"一样返回 `nullptr`（调用方已按 nullptr 处理）。
2. 跨 SCC summary 实例化（约 6790 行）：原来只有 `assert(Ind2 > Ind)`，
   release 构建里 assert 被编译掉，越界直接抛异常。改成越界时打印
   `Warning: skip call with out-of-range MLsub SCC index` 并 `continue`，
   与上面已有的 `skip unhandled call without MLsub SCC` 分支保持同一约定。

两处都保留原有语义：能查到 SCC 时行为完全不变；只有原本会抛异常/断言失败
的路径改为"该函数没有 summary"。

## 验证

- 构建：`cmake --build build-notdec-nothreads2 --target notdec-typerecovery -j8` ；
  microsub2 重新链接（`xmake -P microsub2 -y -r microsub2.exe`，二进制时间戳更新）。
- 回归：microsub2 smoke（含类型恢复）9/9；差分语义测试 22/22 逐字节一致。
- libavcodec 重跑（`NOTDEC_CONSTRAINT_STATS=1` + 35GB 看门狗）进行中，
  用于确认崩溃是否消失以及约束图规模分布。

## 未解决

- **内存**：libavcodec 的类型恢复在崩溃/被杀前常驻内存涨到 22-25 GB
  （缓存 IDB 那轮 43 分钟、峰值 25.0 GB 后被 OOM killer 杀死）。需要按
  `NOTDEC_CONSTRAINT_STATS=1` 的按函数约束图统计定位是哪些函数把图撑大，
  再决定加限额还是修根因。
- **上述守卫没有消除崩溃**：改动后单独重跑 libavcodec（`NOTDEC_CONSTRAINT_STATS=1`、
  35 GB 看门狗）在 46 分 28 秒、峰值 24.2 GB 时仍然抛出同一条
  `vector::_M_range_check: __n (which is 1) >= this->size() (which is 1)`，并且
  新增的 `skip call with out-of-range` 警告一次都没有出现，说明抛出点不在
  这两处。两处改动仍然保留（越界/残留下标本就该降级而不是抛异常），但根因
  定位要继续：静态排查已排除 `opTypes`/`FirstTopoPosition`（map，报错文本
  不会是 vector），候选还剩 `Groups.at(Group.Root)`、`Fields.at(I)`、
  `candidates.at(selector)` 等；正在用 gdb 断在
  `std::__throw_out_of_range_fmt` 上跑完整复现取回溯。
- **内存**：libavcodec 类型恢复峰值 24-25 GB，`NOTDEC_CONSTRAINT_STATS=1`
  没有产生可见输出（需要再确认该开关的输出通道）；句柄级优化（emit 后释放
  MBA）实测只把 tmux 1.92→1.82 GB、libavcodec 25.0→24.2 GB，说明常驻内存
  主要在类型恢复的约束图/类型图上。

## 2026-09-17 新增：SPEC 2017 `628.pop2_s` 的可复现复现器（无需 IDA）

起因：给 htype-layout 数据集补跑 11 个没有 microsub2 运行记录的 SPEC case。
`628.pop2_s`（参考二进制 171 MB）用 `--allow-fresh` 从零分析，在 1581.60 s、
峰值 10,955,908 KiB（约 10.4 GiB）时失败，stderr 结尾：

```
Error: unable to handle meet: i128 with vector   （大量重复）
[microsub2] type recovery failed: NotDec type recovery stopped:
  vector::_M_range_check: __n (which is 1) >= this->size() (which is 1)
```

与 libavcodec 是同一条越界异常，`47e82935` 的防御性 guard 同样没有覆盖到
（warning 未出现），与当时的结论一致。

**可复现复现器**（不需要重跑 IDA；frozen IR 由失败运行留在 work 目录）：

```bash
cd /sn640/NotDec-Exp/Bench2/datasets/htype-layout/runs/binary_datasets_spec2017_cases_628.pop2_s
timeout 1800 /sn640/NotDec/build-notdec-nothreads2/bin/notdec \
  work/02-mlsub-input.ll --frozen-tr-input-ir --tr-level=2 \
  --merge-struct-ptr-load-store -o /tmp/pop2-frozen-out.ll
```

结果：exit 134（SIGABRT，core dumped），同一行
`Exception: vector::_M_range_check: __n (which is 1) >= this->size() (which is 1)`，
约 13 分钟、峰值约 9.7 GB RSS 到达抛出点（frozen IR 259 MB，已在磁盘上）。
这条路径比 libavcodec 更适合做回溯定位：不需要 IDA、输入固定、崩溃点稳定。