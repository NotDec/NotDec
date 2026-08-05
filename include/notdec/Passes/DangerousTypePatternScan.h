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
// 输入 IR 可能是源码 IR、wasm 转出的 IR（i32 指针 + inttoptr）或二进制转出
// 的 IR（i8 GEP），都没有可靠的高层 struct 类型，所以检测完全不用类型/
// DebugInfo，只用指令形状：
//
//   1. 把 load/store/memset/memcpy 的指针操作数拆成 (base, 常量偏移)：
//      - GEP i8/ptr, ptr %p, i64 C -> (%p, C)
//      - add i32/i64/ptr %p, C     -> (%p, C)
//      - inttoptr i32 %addr to ptr -> 递归拆 %addr（wasm32）
//   2. 同一 (函数, base) 上的偏移集合找等差子序列（unroll 的大对象数组
//      展开会留下等差偏移，如 resp_allocate 的 13 项间距 1184）。
//   3. score = max(项数 × 间距)；若该函数是被"池分配挂链表"模式（把 call
//      返回值 store 到 load 出来的对象的偏移处）调用的目标，score ×3。
//   4. 递归命中且 score ≥ 阈值（默认 10000，环境变量
//      NOTDEC_DANGEROUS_SCAN_THRESHOLD 覆盖）报 HIGH warning；无递归证据的
//      大数组访问到 10 倍阈值才报 Note。
struct DangerousTypePatternScan
    : llvm::PassInfoMixin<DangerousTypePatternScan> {
  llvm::PreservedAnalyses run(llvm::Module &M,
                              llvm::ModuleAnalysisManager &AM);
};

} // namespace notdec

#endif
