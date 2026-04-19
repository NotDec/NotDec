# llvm2c workdir 新增 `ValueCTypes.txt`

日期：2026-04-19

## 1. 目的

当前 workdir 已有：

1. `ValueTypes.txt`
   - 反映 `ExtValuePtr -> HType/UType` 恢复结果
2. `SelectableValues.txt`
   - 反映当前可用于 selector 的稳定 value id

但在调 `llvm2c` 时，还缺一层“真正进入 backend 的 C 类型视图”。

这次补了一份新的 workdir 调试文件：

- `ValueCTypes.txt`

用于显示：

1. 每个 `ExtValuePtr` 的 stable id
2. 对应 LLVM IR type
3. 对应 recovered HType
4. `HType -> clang::QualType` lowering 之后的 C 类型字符串

这样可以把问题拆成三层看：

1. `ValueTypes.txt` 就已经错了
2. `ValueTypes.txt` 对，但 `HType -> Clang type` lowering 错了
3. Clang type 对，但后续 `StructuralAnalysis` / printer 出问题

## 2. 修改

### 2.1 `ClangTypeResult` 新增导出接口

修改位置：

- [external/NotDec-llvm2c/include/notdec-llvm2c/TypeManager.h:65](/sn640/NotDec/external/NotDec-llvm2c/include/notdec-llvm2c/TypeManager.h#L65)
- [external/NotDec-llvm2c/lib/notdec-llvm2c/TypeManager.cpp:73](/sn640/NotDec/external/NotDec-llvm2c/lib/notdec-llvm2c/TypeManager.cpp#L73)

涉及函数：

- `notdec::llvm2c::ClangTypeResult::writeValueCTypes`

输出格式当前为：

```text
# Final Value -> lowered Clang C type mapping

[+] main::arg0 (...) => int ; ir=i32 ; htype='c:32
...

[memory] <memory> => int ; htype=top:32
```

当前每条 value 记录包含：

1. 极性 `[+] / [-]`
2. `toStableString()` 形式的 stable id
3. 必要时补充 verbose value 标签
4. lowered Clang type
5. LLVM IR type
6. 原始 HType 字符串

另外：

1. 显式跳过了内部 `nullptr` value 键
2. 单独保留 `[memory] <memory>` 行

### 2.2 为导出路径增加 safe lowering

修改位置：

- [external/NotDec-llvm2c/lib/notdec-llvm2c/TypeManager.cpp:188](/sn640/NotDec/external/NotDec-llvm2c/lib/notdec-llvm2c/TypeManager.cpp#L188)

涉及函数：

- `canLowerToClangTypeImpl`
- `canLowerToClangType`

背景：

直接对所有 `Result->ValueTypes` 调 `convertType()` 时，会遇到少量 HType 仍然
引用尚未挂上 `ASTDecl` 的 record/typedef，导致导出本身崩溃。

这类情况对调试文件不应是 fatal，因此现在会先做一层可 lowering 判定：

1. 能安全 lowering：输出实际 C type
2. 不能安全 lowering：输出 `<unlowered>`

这样不会因为少量未就绪声明把整次反编译打断。

### 2.3 接入 llvm2c workdir 导出

修改位置：

- [external/NotDec-llvm2c/lib/notdec-llvm2c/StructuralAnalysis.cpp:83](/sn640/NotDec/external/NotDec-llvm2c/lib/notdec-llvm2c/StructuralAnalysis.cpp#L83)
- [external/NotDec-llvm2c/lib/notdec-llvm2c/StructuralAnalysis.cpp:1570](/sn640/NotDec/external/NotDec-llvm2c/lib/notdec-llvm2c/StructuralAnalysis.cpp#L1570)

涉及函数：

- `notdec::llvm2c::decompileModule`

行为：

1. 新增文件名常量 `ValueCTypes.txt`
2. 在 `CT->defineDecls()` 后、结构化分析前，如果配置了 `opts.workDir`
   就写出该文件

## 3. 验证

### 3.1 冒烟 case

```bash
./build/bin/notdec \
  test/type-recovery/llvm-ir/cases/01_Simple1.ll \
  -o /tmp/notdec-valuectypes-smoke/out.c \
  --tr-level=2 \
  -g \
  --work-dir=/tmp/notdec-valuectypes-smoke
```

结果：

1. 成功生成 `/tmp/notdec-valuectypes-smoke/out.c`
2. 成功生成 `/tmp/notdec-valuectypes-smoke/ValueCTypes.txt`

样例内容：

```text
[+] main::arg0 (Value: i32 %_arg_0 (In Func: main)) => int ; ir=i32 ; htype='c:32
[+] main::arg1 (Value: i32 %_arg_1 (In Func: main)) => int ; ir=i32 ; htype='d:32
```

### 3.2 当前 `fortune` 限制

针对：

- `test/type-recovery/realworld/cases/fortune.o3.wasm.ll`

当前 `ValueCTypes.txt` 还不能稳定作为 smoke oracle，原因不是这次导出代码本身，
而是当前 `llvm2c` 在该 case 上仍可能先撞到既有的 `demoteSSA` / PHI 相关
use-after-free。

因此当前状态是：

1. `ValueCTypes.txt` 导出链路已经在小 case 上验证通过
2. `fortune` 上是否能稳定看到该文件，仍取决于更前面的 `llvm2c` 既有稳定性

## 4. 后续建议

如果后续要把它真正接进测试或更强的 workdir oracle，我建议顺序是：

1. 先把 `fortune` 上 `demoteSSA` 的 UAF 修掉
2. 再考虑给 `ValueCTypes.txt` 增加 machine-readable 格式
3. 最后再决定是否把 realworld suite 扩成：
   - debug truth 对 `.htypes`
   - workdir 对 `ValueCTypes.txt`

当前更适合作为调试辅助文件，而不是 `pass/fail` 主 oracle。
