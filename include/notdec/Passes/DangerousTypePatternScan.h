#ifndef _NOTDEC_PASSES_DANGEROUS_TYPE_PATTERN_SCAN_H_
#define _NOTDEC_PASSES_DANGEROUS_TYPE_PATTERN_SCAN_H_

#include <llvm/IR/Module.h>
#include <llvm/IR/PassManager.h>

namespace notdec {

// 类型推理前的危险模式扫描。
//
// 背景：memcached 的 resp_allocate/resp_start 组合（对 13 个 mc_resp 柔性
// 数组的展开访问 + resp->next 链表自引用）让类型推理内存/时间爆炸。本 pass
// 在 MLsubRecoveryMain 之前扫描 IR，对"破坏力分数"超阈值的函数输出 warning，
// 避免每次都要先跑一遍类型推理才知道哪个函数危险。
//
// 破坏力分数（不使用 DebugInfo，全部来自 IR 自身）：
//   score(F) = sum over struct S:
//        dynamic-array-GEP-count(F, S) * size(S) * weight(S)
//   - dynamic-array-GEP：GEP 的 source element type 是 struct（或 struct 数组），
//     且最后一个索引是非常量/非 0（如 resp_allocate 里的
//     getelementptr %struct._mc_resp, ptr %r, i64 %idx）。
//   - size(S)：DataLayout 算出的结构体字节数。
//   - weight(S)：1.0；若命中"池分配挂链表"递归模式（某函数返回 GEP(S) 元素，
//     调用点把该返回值 store 到 load 出来的对象的字段里）则 +2.0。
//     这是无 DebugInfo 下对"struct 自引用/递归"的近似：对象从池中取出后
//     被挂进结构（链表/树），通常意味着该结构是递归共享类型。
//
// 阈值默认 10000，可用环境变量 NOTDEC_DANGEROUS_SCAN_THRESHOLD 覆盖。
struct DangerousTypePatternScan
    : llvm::PassInfoMixin<DangerousTypePatternScan> {
  llvm::PreservedAnalyses run(llvm::Module &M,
                              llvm::ModuleAnalysisManager &AM);
};

} // namespace notdec

#endif
