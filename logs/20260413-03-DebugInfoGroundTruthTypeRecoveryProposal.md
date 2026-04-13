# 2026-04-13 基于 DebugInfo Ground Truth 的类型恢复测试 Proposal

## 背景

当前类型恢复回归主要依赖：

- 手写 LLVM IR 输入
- 或源码临时编译成 LLVM IR
- 再对 `--dump-htypes` 生成的 `.htypes` snapshot 做文本比较

这条链路已经能覆盖“是否稳定产出某份 HType 结果”，但还不够适合回答更关键的问题：

- 恢复出来的结构体布局到底对不对
- 某个参数 / 局部对象 / 全局对象应该对应什么结构体形状
- 当前 `.htypes` 中的 `record / ptr<load/store>` 是否真的和源码语义一致

这次希望新增一类更“语义化”的类型恢复测试：

1. 用 C 编写具体 case
2. 将 C 编译为**带调试信息的 LLVM IR**，把它作为 ground truth
3. 基于该 IR 去掉调试信息后的结果作为 `notdec` 输入
4. 跑出 `.htypes`
5. 用调试信息中的结构体定义和变量类型，自动对比 `.htypes`，评估恢复准确性

---

## 目标

### 目标

1. 新增一套面向“结构体恢复准确性”的类型恢复 suite
2. ground truth 以 LLVM IR 中的 debug metadata 为准，而不是手写 `.htypes`
3. 让 case 的作者继续用 C 写测试，不要求手写 LLVM IR
4. 保留 `.htypes` 作为中间产物和调试视图，但 pass/fail 由语义比较器决定
5. 增加辅助工具，降低新 case 的生成和调试成本

### 非目标

1. 第一阶段不追求覆盖所有 DWARF / DI 语义
2. 第一阶段不要求恢复字段名，只先比较布局、偏移、嵌套关系、指针关系
3. 第一阶段不把 `.c` 反编译输出作为 oracle
4. 第一阶段不强行统一现有 `llvm-ir` / `sysy` suite 的 oracle 机制

---

## 总体方案

建议新增独立 suite，例如：

- `test/type-recovery/debug-info-c/`

核心策略：

1. `cases/*.c`
   - 人工编写和维护的测试源
2. `truth/*.dbg.ll`
   - 由工具从 C 编译得到、并 check in 的带调试信息 LLVM IR
   - 这是该 suite 的权威 ground truth
3. 运行测试时
   - 从 `truth/*.dbg.ll` 用 `opt-14 -strip-debug -S` 生成 workdir 下的输入 `.ll`
   - 对该 stripped IR 跑 `notdec --dump-htypes`
   - 从 `truth/*.dbg.ll` 提取结构体真值
   - 将真值与 `.htypes` 做语义比较

这里建议**把 `truth/*.dbg.ll` check in，而不是每次测试都临时从 C 重编译**。

原因：

1. ground truth 需要稳定，不能受 clang 小版本和本机参数漂移影响
2. debug metadata 本身就是 oracle，如果每次现编，oracle 也会漂移
3. C 仍然保留为作者入口，但权威回归产物应是固定的 `.dbg.ll`

换句话说，这个 suite 的角色分层是：

- `cases/*.c`
  - 作者友好的输入源
- `truth/*.dbg.ll`
  - 被锁定的权威真值
- `workdir/*.input.ll`
  - 每次运行时从 truth 动态 strip 出来的真实 `notdec` 输入
- `workdir/*.out.htypes`
  - 类型恢复结果

---

## 建议目录布局

建议新增：

```text
test/type-recovery/debug-info-c/
  README.md
  manifest.json
  cases/
    01_simple_struct.c
    02_nested_ptr.c
  truth/
    01_simple_struct.dbg.ll
    02_nested_ptr.dbg.ll
  support/
    ...
  legacy/            # 可选，放早期实验产物
```

workdir 中的临时产物建议统一为：

```text
<case>.input.ll
<case>.out.ll
<case>.out.htypes
<case>.truth.json
<case>.compare.json
<case>.compare.txt
<case>.log
```

其中：

- `.truth.json`
  - 从 `truth/*.dbg.ll` 提取出的规范化真值
- `.compare.json`
  - 结构化对比结果，便于后续脚本或 CI 消费
- `.compare.txt`
  - 面向人看的 diff / mismatch 摘要

---

## 需要新增或改造的工具

## 1. case 生成工具

建议新增一个面向作者的脚本，例如：

- `test/tools/gen_debug_info_case.py`

输入：

- `cases/foo.c`

输出：

- `truth/foo.dbg.ll`

推荐编译参数：

```bash
clang-14 \
  -opaque-pointers=0 \
  -S -emit-llvm \
  -O0 \
  -g \
  -Xclang -disable-O0-optnone \
  -fno-builtin \
  -fno-discard-value-names \
  -I <support-dir> \
  cases/foo.c \
  -o truth/foo.dbg.ll
```

这个工具的职责：

1. 从 C 生成带 debug info 的 LLVM IR
2. 统一编译参数，避免每个 case 各自手写
3. 在必要时校验 `truth/*.dbg.ll` 与 `cases/*.c` 是否同步
4. 可选支持 `--refresh` / `--check` 两种模式

---

## 2. debug truth 提取工具

建议新增一个工具，把 `truth/*.dbg.ll` 转成规范化 JSON，例如：

- `test/tools/extract_debug_type_truth.py`

输入：

- `truth/foo.dbg.ll`

输出：

- `workdir/foo.truth.json`

建议提取的信息至少包括：

1. 命名结构体 / 匿名结构体图
2. 结构体大小
3. 字段列表
   - 字段名
   - `offset_bits`
   - `size_bits`
   - `kind`
     - primitive
     - pointer
     - record
     - array
4. 符号到类型的绑定
   - 函数参数
   - 返回值（如果 DI 中能稳定拿到）
   - 全局变量
   - 由 `dbg.declare` / `dbg.value` 绑定到 IR value 的局部变量

实现层面建议优先走**LLVM IR / DebugInfo 的正式 API**，不要依赖脆弱的正则硬解析。

如果要尽量减少主程序侵入，优先级建议是：

1. 独立工具
2. runner 调用该工具
3. 暂不改 `notdec` 主链路

---

## 3. `.htypes` 解析与比较工具

建议新增：

- `test/tools/compare_htypes_with_debug_truth.py`

输入：

- `workdir/foo.out.htypes`
- `workdir/foo.truth.json`

输出：

- `workdir/foo.compare.json`
- `workdir/foo.compare.txt`
- 进程退出码决定 case pass/fail

这个工具建议分两层：

### 3.1 `.htypes` parser

把当前稳定的 `.htypes` 文本转成内部 AST / JSON。

需要解析的重点：

1. `[decls]`
   - `struct struct_N { ... }`
   - 字段类型
   - 字段 offset 注释
2. `[types]`
   - stable key
   - `+/-` polarity
   - 对应 HType
3. 指针 / record / array / function / union/intersection / top/bottom / type variable

### 3.2 semantic comparator

把 debug truth 和 recovered HType 统一到同一套“比较视图”上。

第一阶段建议只关心：

1. 是否恢复出对应 record
2. record 的字段 offset 是否匹配
3. 字段是否为 pointer / nested record / primitive
4. 嵌套结构是否可递归对齐

默认忽略：

1. 字段名
2. typedef 名
3. 调试位置、行号
4. HType 中纯符号变量名（如 `'a:32`）

---

## Runner 与 Manifest 改造

当前 `test/run_type_recovery_suite.py` 已经支持：

1. manifest 驱动
2. `source_prep`
3. 跑 `notdec --dump-htypes`
4. 对 `.htypes` 文本做 snapshot 比较

建议把它扩成“多 oracle 模式”，而不是重写一个新 runner。

### 建议新增的阶段

1. `prepare_input`
   - 当前已有
2. `prepare_truth`
   - 从 `.dbg.ll` 提取 `.truth.json`
3. `run_notdec`
   - 当前已有
4. `compare_oracle`
   - 根据 suite / case 的 oracle kind 决定如何判定 pass/fail

### 建议新增的 oracle kind

1. `htype-snapshot`
   - 兼容当前 `llvm-ir` / `sysy` suite
2. `debug-struct-compare`
   - 新 suite 使用

### manifest 可新增的字段

```json
{
  "suite": "notdec type recovery debug info c tr-level=2",
  "default_args": ["--tr-level=2"],
  "oracle": {
    "kind": "debug-struct-compare"
  },
  "cases": [
    {
      "name": "01_simple_struct",
      "source": "cases/01_simple_struct.c",
      "ground_truth": "truth/01_simple_struct.dbg.ll",
      "status": "pass",
      "roots": [
        "foo::arg0",
        "@g_state"
      ]
    }
  ]
}
```

这里的 `roots` 很重要。

建议第一阶段只比较**根对象明确、stable key 明确**的值，例如：

1. `function::argN`
2. `@global`
3. 少量命名明确的局部对象

这样能显著降低 `.htypes` key 映射和 unnamed SSA 漂移带来的噪音。

---

## Ground Truth 与 HType 的比较语义

这里建议不要做“调试类型文本 == HType 文本”的字符串比较，而要比较规范化后的结构图。

## 1. truth 侧规范化

把 DI 里的结构统一成：

```json
{
  "records": {
    "struct.list_node": {
      "size_bits": 64,
      "fields": [
        { "offset_bits": 0, "kind": "primitive", "size_bits": 32 },
        { "offset_bits": 32, "kind": "pointer", "to": "struct.list_node" }
      ]
    }
  }
}
```

## 2. HType 侧规范化

把 `.htypes` 中的信息统一成近似结构：

```json
{
  "values": {
    "foo::arg0": {
      "kind": "pointer",
      "to": "struct_0"
    }
  },
  "records": {
    "struct_0": {
      "fields": [
        { "offset_bits": 0, "kind": "primitive" },
        { "offset_bits": 32, "kind": "pointer", "to": "struct_0" }
      ]
    }
  }
}
```

## 3. 比较规则

第一阶段建议使用“保守兼容”判定：

1. truth 中要求存在的字段 offset，recovered record 必须有
2. truth 中字段是 pointer 时，recovered 也必须能解释成 pointer
3. truth 中字段是 nested record / pointer-to-record 时，允许 recovered 使用匿名 `struct_N`
4. truth 中字段是 primitive 时，不要求完全匹配 signedness，但要求位宽或类别兼容
5. recovered 可以比 truth 更“宽”，但不能缺字段、错 offset、错层级

这样可以避免一开始就因为：

- `top/bottom`
- union/intersection
- load/store pointer richer 语义

把所有 case 都打成 fail。

后续如果这套比较器稳定，再考虑增加：

1. strict 模式
2. signedness / float 精细比较
3. array length 校验
4. union / recursive type 更严格的图同构比较

---

## 对 case 作者的约束建议

为了让这类 case 更稳定，建议新增一份 suite README 约束：

1. 优先围绕“一个根结构体”设计 case
2. 优先通过函数参数或全局变量暴露要检查的对象
3. 尽量避免需要依赖 unnamed SSA value 才能定位的 oracle
4. 尽量使用显式 `struct` / `typedef struct`
5. 第一阶段避免 bitfield、packed struct、复杂 union、C++ 继承等高噪音特性
6. 尽量保持 case 短小，单 case 只验证 1 到 2 个恢复点

这类约束不是能力上限，而是为了先把 suite 打磨成稳定的工程资产。

---

## 需要改动的文件范围

如果按这个 proposal 落地，预计会涉及：

1. `test/run_type_recovery_suite.py`
   - 增加 truth 提取与 compare 阶段
2. `test/CMakeLists.txt`
   - 注册新 suite
3. `test/README.md`
   - 记录新 oracle 模式
4. `test/type-recovery/debug-info-c/README.md`
5. `test/type-recovery/debug-info-c/manifest.json`
6. `test/tools/gen_debug_info_case.py`
7. `test/tools/extract_debug_type_truth.py`
8. `test/tools/compare_htypes_with_debug_truth.py`
9. 首批 `cases/*.c` 与 `truth/*.dbg.ll`

第一阶段不强制要求改动 `src/NotDec.cpp` 或类型恢复主流程。

如果后续发现 `.htypes` 文本解析过于脆弱，再追加第二阶段设计：

1. 为 `HTypeResult` 增加 machine-readable dump
2. 例如额外导出 JSON / graph 格式

但这不应阻塞第一版 suite 的建立。

---

## 分阶段落地建议

## Phase 1: 最小闭环

目标：

1. 跑通一条 case
2. 以 `.dbg.ll` 为真值
3. 能完成 stripped input -> `.htypes` -> compare

建议内容：

1. 新建 suite 目录
2. 新增 `gen_debug_info_case.py`
3. 在 runner 中加入 `debug-struct-compare`
4. 先支持：
   - named struct
   - pointer to struct
   - globals / args
5. 先放 3 到 5 个最小 case

## Phase 2: 扩展结构覆盖面

加入：

1. nested struct
2. self-recursive struct
3. array field
4. pointer chain
5. 局部变量映射

## Phase 3: 提高比较精度

加入：

1. signedness / float 更细粒度比较
2. array 长度比较
3. richer pointer 语义比较
4. xfail -> pass 的稳定回归提升

---

## 风险与开放问题

## 1. debug info 到 stable key 的映射

这是实现里最需要小心的一点。

虽然 `.htypes` 已经有 stable key，但：

1. debug info 绑定的是源码变量
2. `.htypes` 绑定的是 `ExtValuePtr`
3. 两者不一定天然一一对应

所以第一阶段应优先比较：

1. `argN`
2. `@global`
3. 经 `dbg.declare` 能稳定映射的对象

不要一上来把所有局部 SSA 都纳入 oracle。

## 2. `.htypes` parser 的稳健性

当前 `.htypes` 已经较稳定，但本质仍是面向人阅读的文本格式。

因此建议：

1. Phase 1 先解析现有格式
2. 若 parser 复杂度明显失控，再补 machine-readable dump

## 3. clang 生成的 debug IR 漂移

即便仓库当前默认是 LLVM 14，debug metadata 仍可能受编译参数影响。

所以建议：

1. check in `truth/*.dbg.ll`
2. 用统一脚本 refresh
3. 不把“每次现编出来的 IR”直接当 oracle

---

## 建议的首批 case 方向

建议首批只做最能体现“结构体恢复准确性”的小 case：

1. 单层 struct 指针参数
2. struct 内含指针字段
3. 自引用链表节点
4. nested struct
5. global struct state

这些 case 的好处是：

1. debug truth 清晰
2. `.htypes` 里的 record / pointer 也容易人工复核
3. 一旦失败，定位成本较低

---

## 结论

建议把这项工作当成“在现有 HType snapshot suite 旁边，新增一套 debug-info ground truth suite”，而不是直接推翻现有测试体系。

更具体地说：

1. 保留当前 `llvm-ir` / `sysy` snapshot suite 做稳定性回归
2. 新增 `debug-info-c` suite 做结构体恢复准确性回归
3. 让 C 继续作为作者入口
4. 让 `.dbg.ll` 成为权威真值
5. 让 `.htypes` 成为被比较对象，而不是手写 oracle

这样既不会打断当前测试资产，也能逐步把类型恢复测试从“文本快照一致”推进到“结构语义正确”。
