# Bench2 source-ir 全项目 scalability 验证与 wrk 输入形状修复

## 用户原始 prompt

> 继续跑Bench2里面其他项目吧，看看是否scalability足够，大的binary也能跑出来

## 背景

memcached 类型恢复爆炸修复（20260808-02）后，需要确认其余 Bench2 source-ir
项目也能在合理时间/内存内跑出。项目规模梯度：fortune(0.5MB) → wrk(1.6MB)
→ ffplay(3.0MB) → vsftpd(4.2MB) → ngircd(5.1MB) → libuv(6.1MB) →
memcached(8.0MB)。

## 结果（8 线程，tr-level=2 + merge-struct-ptr-load-store，RelWithDebInfo）

| 项目 | IR | wall | RSS | 结果 |
|---|---|---|---|---|
| fortune | 0.5MB | 0.7s | 165MB | 通过，verifier OK |
| wrk | 1.6MB | 2.6s | 200MB | 通过（修复后），verifier OK |
| ffplay | 3.0MB | 5.7s | 226MB | 通过，verifier OK |
| vsftpd | 4.2MB | 7.4s | 268MB | 通过，verifier OK |
| ngircd | 5.1MB | 9.9s | 302MB | 通过，verifier OK |
| libuv | 6.1MB | 29.0s | 1.4GB | 通过，verifier OK |
| memcached | 8.0MB | 1:43 | 677MB | 通过，输出与基线 SAME |

结论：scalability 足够，最大项目在 2 分钟内、1.4GB 以内完成，输出全部通过
LLVM 22 verifier。类型恢复爆炸修复没有在其他项目上退化。

## wrk 卡点与修复

wrk 首跑崩溃（exit 134），两个输入形状问题，均与类型恢复性能无关：

1. `src/TypeRecovery/LowTy.cpp:117` `llvmType2Elem`：wrk.ll 含 214 处
   `x86_fp80`（long double），未处理分支直接 assert。补
   x86_fp80/fp128/ppc_fp128/half/bfloat 映射（Elem 仅是 NotPN 标签，
   join/meet 为字符串拼接，无语义影响）。
2. `external/binarysub/src/TypeBuilder.cpp` `convertFieldType`：wrk 推理输出
   含空 record（binarysub 归一化的内部占位 marker）作为对象字段，经
   `convertPointer` 变 void* 后 `stripObjectValuePointer` 断言 pointee 非空。
   修复：URecordType 分支空 record 直接返回 void*（与 convertPointer 行为
   一致，不再剥 pointer）；UInter 分支过滤循环跳过 marker term，过滤后
   为空时返回 void*（地址形状未知的透明值）。

## 验证

- 全部项目输出 `llvm-as`（LLVM 22）验证通过。
- TypeBuilder 修改回归：fortune/vsftpd/ngircd/libuv/ffplay 重跑输出与修改
  前 SAME；memcached 166 切片与完整 memcached 输出与基线 IDENTICAL。
- wrk 无历史基线，仅 verifier + 运行成功。

## 评分

- 实现效果：8/10。七个项目全部跑通，scalability 结论清晰。
- 复杂度：3/10。两处修复都是补分支/透明处理，各 5 行左右。
- 维护成本：2/10。无新数据结构，仅增加类型分支；marker 透明语义与
  isZeroSizedRecordMarker 注释一致。
- 更优方案：无。UInter 空过滤返回 void* 是保守选择，若后续发现输出污染
  可改为跳过字段，但当前无证据。
