# LLVM IR LSP Server 设计分析

本文尝试回答两个问题：

1. 一版 Language Server Protocol (LSP) server 通常怎么实现
2. 如果场景是 LLVM IR `.ll` 文件，需要实现哪些接口，以及这些接口背后大概怎么做

这里不追求“把所有 LSP 特性一次做全”，而是以一个可逐步演进的 LLVM IR language server 为目标，给出一套适合实际落地的实现骨架。

## 1. 先把 LSP 本质说清楚

LSP 的本质并不复杂：

- 编辑器负责采集用户动作，例如打开文件、修改文本、把光标移到某个位置
- language server 负责维护文档状态，并回答“这个位置是什么”“定义在哪里”“有哪些引用”“现在有哪些诊断”这类问题
- 双方通过 JSON-RPC 传消息，常见承载方式是 `stdin/stdout`

一个 language server 的基本责任通常只有三类：

1. 跟踪打开文件和未保存改动
2. 维护与当前文本版本一致的诊断和语义信息
3. 对编辑器请求返回结构化结果

这和 `clangd`、`mlir-lsp-server` 的实现分层都很接近：外面是 JSON-RPC/LSP，里面是语言相关语义引擎。

参考：

- LSP overview: <https://microsoft.github.io/language-server-protocol/>
- clangd design: <https://clangd.llvm.org/design/>
- clangd code walkthrough: <https://clangd.llvm.org/design/code>
- MLIR LSP design: <https://mlir.llvm.org/docs/Tools/MLIRLSP/>

## 2. 一版 LSP server 的典型分层

最常见也最稳妥的实现可以拆成四层：

### 2.1 Transport 层

职责：

- 读写 JSON-RPC 消息
- 处理 `Content-Length` framing
- 区分 request / response / notification

这一层不应该知道 LLVM IR，也最好几乎不知道 LSP 的业务细节。它只负责“可靠收发消息”。

### 2.2 Protocol 层

职责：

- 解析 LSP 方法名，例如 `textDocument/hover`
- 将 JSON 映射成内部结构体
- 做 capability 协商
- 把请求分发给语言语义层

这一层主要解决“LSP 长什么样”的问题，而不是“LLVM IR 是什么”。

### 2.3 Document & Analysis 层

职责：

- 维护打开文档的文本、版本号、URI
- 在文本变化后触发解析、验证、索引
- 把语义分析结果缓存起来

这是第一版 server 最核心的状态层。没有它，后面的 hover/definition/references 都很难稳定做对。

### 2.4 Feature 层

职责：

- 实现 `hover`
- 实现 `definition`
- 实现 `references`
- 实现 `documentSymbol`
- 实现 `completion`
- 实现 `semanticTokens`
- 实现 `rename`

feature handler 最好尽量薄，主要做两件事：

1. 根据光标位置或请求参数定位语义对象
2. 把分析结果翻译成 LSP 返回值

## 3. LLVM IR 场景下的对象模型

LLVM IR 和 C/C++ 不一样，第一版 server 的核心优势在于：它的语义对象比 C++ 简单得多，很多功能不必依赖庞大的跨翻译单元编译系统。

### 3.1 LLVM IR 里最常见的可导航对象

- `@global` 全局变量
- `@func` 函数
- `%tmp`、`%arg` 这类 SSA value
- 基本块 label
- 类型名，例如 `%struct.foo = type { ... }`
- 元数据引用，例如 `!dbg`
- 属性、calling convention、linkage、instruction opcode

### 3.2 对 LSP 最关键的几类关系

- 定义到引用：`Value*` 的 def-use 链
- 引用到定义：某个 use 对应的 operand 来自哪个定义
- 局部符号层级：module -> function -> basic block -> instruction
- 文本位置到 IR 对象：cursor position -> token -> semantic node

只要这四类映射做扎实，LLVM IR 的大部分核心 IDE 能力都能做出来。

## 4. 最小可用版需要实现哪些 LSP 接口

如果目标是“先做出一版能用的 LLVM IR LSP server”，我建议分三批。

### 4.1 第一批：生命周期和文本同步

这些几乎是必需的。

#### `initialize`

作用：

- 接收 client capabilities
- 返回 server capabilities
- 初始化 workspace 根目录、配置项、是否支持增量同步等信息

LLVM IR server 在这里至少应该声明：

- `textDocumentSync`
- `hoverProvider`
- `definitionProvider`
- `referencesProvider`
- `documentSymbolProvider`
- `documentFormattingProvider` 或 `documentRangeFormattingProvider`（如果准备做）
- `semanticTokensProvider`（如果准备做）
- `completionProvider`（如果准备做）

#### `initialized`

作用：

- 告诉 server：初始化阶段结束
- 可以在这里启动后台索引、加载外部配置、预热缓存

#### `shutdown`

作用：

- 请求 server 进入可退出状态

#### `exit`

作用：

- 进程退出

#### `textDocument/didOpen`

作用：

- 把文件内容交给 server
- 建立 `DocumentState`
- 触发第一次解析和诊断

#### `textDocument/didChange`

作用：

- 更新文档版本和文本
- 重新解析或增量更新

第一版建议直接用 `TextDocumentSyncKind::Full`，每次都收整份文本。LLVM IR 文件通常不大，先把正确性做稳比做复杂增量 patch 更重要。

#### `textDocument/didClose`

作用：

- 释放打开文档态缓存
- 视策略保留或清除索引

#### `textDocument/didSave`

作用：

- 可选
- 如果 server 需要把“磁盘状态”和“内存状态”区分开，这个接口有用
- 对 LLVM IR 来说，很多第一版实现甚至可以不依赖它

### 4.2 第二批：真正能让编辑器“有用起来”的功能

#### `textDocument/publishDiagnostics`

这是 notification，不是 request。

作用：

- 向编辑器推送解析错误、验证错误、语义错误

LLVM IR 场景下最自然的做法是：

1. 先用 LLVM assembly parser 解析 `.ll`
2. 解析失败时，把 `SMDiagnostic` 或等价错误结构翻译成 LSP `Diagnostic`
3. 解析成功后，做 module/function 级验证，再把 verifier 结果转成 `Diagnostic`

对用户体验来说，diagnostics 往往是第一重要的功能，因为这是“server 真在理解 LLVM IR”最直观的体现。

#### `textDocument/hover`

作用：

- 光标悬停时展示对象信息

LLVM IR 场景可以返回：

- symbol kind，例如 function/global/local SSA value/basic block
- 对应 LLVM type
- 定义摘要
- 对 instruction opcode 的简短说明
- 对 intrinsic 的签名和作用说明

一个很实用的 hover 形式是直接返回一小段规范化 IR 片段，例如：

```llvm
%x = load i32, ptr %p, align 4
; type: i32
; parent: @foo
```

#### `textDocument/definition`

作用：

- 从当前引用跳到定义

LLVM IR 场景实现难度相对低，因为：

- SSA use 通常能直接回到定义 instruction 或 function argument
- `call @foo(...)` 可以跳到 `define ... @foo(...)`
- `br label %bb1` 可以跳到对应 basic block
- `@g` 的 use 可以跳到 global definition

核心问题反而是“如何从文本 offset 精确定位到哪个 operand/token”。

#### `textDocument/references`

作用：

- 查找某个对象的所有引用

LLVM IR 场景的天然数据结构就是 def-use 链和符号表：

- 对本文件内 SSA value：遍历 uses
- 对 function/global/type alias：走符号索引
- 对 label：扫描分支和 phi incoming block

#### `textDocument/documentSymbol`

作用：

- 给编辑器 outline 视图

LLVM IR 的 outline 很适合按下面层级返回：

1. module
2. globals
3. functions
4. blocks
5. 可选：命名的 instructions 或 type definitions

这类接口实现简单，但对大文件导航非常有帮助。

### 4.3 第三批：增强体验的功能

#### `textDocument/completion`

LLVM IR 补全和 C++ 补全不一样，不太需要复杂语义恢复，但需要强上下文判断。

常见补全来源：

- instruction opcode，例如 `load`、`store`、`icmp`
- linkage / calling convention / attributes
- 已存在的 `%ssa`、`@global`、`@func`
- 类型名
- metadata name
- intrinsic 名称

第一版可以用“词法上下文 + 轻量语法上下文”实现：

- 行首更偏向 instruction/opcode
- `call` 后更偏向函数名和 intrinsic
- 类型位置更偏向类型名
- operand 位置更偏向 `%` / `@` 符号

#### `textDocument/semanticTokens/full`

作用：

- 语义高亮

LLVM IR 很适合做 semantic tokens，因为 token 类型边界比较清晰：

- keyword/opcode
- type
- function
- parameter
- variable
- namespace-like symbol（可映射到 global/type name）
- comment
- number
- string
- metadata

#### `textDocument/rename`

LLVM IR 很适合做 rename，但建议分级支持：

1. 先支持本地 `%ssa`、basic block label
2. 再支持函数名、全局变量名
3. 最后再考虑类型名、metadata 名

rename 最大的坑不是语义，而是文本改写：

- `%0`、`%1` 这类匿名值不能随便改成具名后破坏打印风格
- 某些名字是 printed form，不一定稳定映射到内部对象
- 必须保证工作在“当前版本的文本文档”上

#### `textDocument/codeAction`

LLVM IR 的 code action 不一定要早做，但可以考虑：

- 插入缺失的 symbol name
- 规范化某些书写
- 自动修复简单 verifier 错误
- 调用 `llvm-as`/`opt` 风格的格式化或 canonicalize

## 5. 这些接口背后大概怎么做

下面以一个典型请求流来说明。

### 5.1 打开文件后的主链路

`didOpen`
-> 建立 `DocumentState`
-> 保存文本与版本号
-> 调用解析器构造 `Module`
-> 建立位置映射与符号索引
-> 运行 verifier
-> 推送 diagnostics

这里最关键的产物通常有四个：

1. `DocumentState`
2. `ParsedIR`
3. `SymbolIndex`
4. `LineIndex` 或 offset/position 映射表

### 5.2 Hover 的主链路

`hover(position)`
-> 文本位置转 byte offset
-> 找到当前 token
-> 从 token 定位到语义对象
-> 读取缓存的解析结果与符号信息
-> 组装 markdown/plaintext 返回

实现时需要特别注意：

- LSP 的位置是 `line/character`
- character 常按 UTF-16 code unit 计数
- LLVM 的很多 parser 和源码位置结构按 byte offset 工作

因此必须有一个可靠的 position <-> offset 转换层。

### 5.3 Definition / References 的主链路

`definition(position)` 或 `references(position)`
-> 位置定位到 token
-> token 解析成 symbol reference
-> 查 `SymbolIndex`
-> 把源位置转成 LSP `Location` / `Location[]`

这里建议把“可导航对象”单独抽象成一个内部类型，例如：

```text
SymbolId
  - kind: Function | Global | LocalValue | Block | TypeName | Metadata
  - document: URI
  - stable key: printed name / ordinal / custom internal id
```

这样 hover、definition、references、rename 可以共用一套语义定位逻辑。

## 6. LLVM IR server 的内部数据结构建议

### 6.1 `DocumentStore`

建议维护：

- `uri -> latest text`
- `uri -> latest version`
- `uri -> parse result`
- `uri -> diagnostics`
- `uri -> symbol index`

### 6.2 `ParsedIR`

建议至少包含：

- `std::unique_ptr<llvm::Module>`
- 解析/验证阶段的错误集合
- token 到语义对象的映射
- 语义对象到源码 range 的映射

一个现实问题是：LLVM 自身更多是“解析后拿 IR 对象”，不天然提供一套完整的“编辑器级 token 精确反查”能力。所以第一版往往需要自己补一层源码映射。

### 6.3 `SymbolIndex`

第一版完全可以先做“单文件索引”：

- 全局符号表：`@func`、`@global`、named type
- 局部符号表：`%arg`、`%tmp`、basic block
- use list：symbol -> all source ranges

如果后面要支持工作区级跳转，再扩展成：

- per-file index
- merged workspace index

### 6.4 `SourceMap`

LLVM IR LSP 非常依赖精确位置映射，建议单独抽象：

- 文本 offset -> `Position`
- `Position` -> 文本 offset
- token range -> LSP range
- IR object -> source range
- source range -> IR object

如果这层不独立，后面 feature 会越来越难维护。

## 7. 解析和语义分析怎么接 LLVM

### 7.1 解析

`.ll` 文本最自然的方案就是直接复用 LLVM 的 assembly parser。

一个常见做法是：

- 内存里拿到整份文本
- 构造 memory buffer
- 调 LLVM parser 得到 `Module`
- 收集 parse diagnostics

对于 `.bc`：

- 从 LSP 角度看，不太适合作为“直接编辑对象”
- 更合理的策略是只把 `.ll` 作为主支持目标
- 如果要支持 `.bc`，更像“只读反汇编视图”而不是标准文本编辑

### 7.2 语义分析

解析成功后做几件便宜但高收益的事情：

1. 遍历 module，收集函数、全局、类型、块、具名 SSA value
2. 建立 operand -> def 映射
3. 建立 def -> uses 映射
4. 记录每个对象的源码 range
5. 可选地做 verifier

LLVM IR 本身已经是 typed IR，因此 hover/definition/references 的语义信息基础通常比源码语言更直接。

## 8. LLVM IR 和 clangd 的关键差异

虽然实现骨架和 `clangd` 很像，但 LLVM IR server 有几个重要不同。

### 8.1 不依赖 compile commands

`clangd` 很依赖 compile commands，因为 C/C++ 解析受头文件、宏、语言标准、目标平台影响很大。

LLVM IR `.ll` 解析通常不需要这套复杂编译配置，因此：

- 初始化更简单
- 单文件语义稳定性更高
- 很多功能更容易在第一版做出来

### 8.2 语义对象更“显式”

LLVM IR 的 def-use、类型、基本块结构本身就显式存在，这让：

- definition/references
- documentSymbol
- hover

这几类功能非常适合先做，而且收益很高。

### 8.3 真正难的是源码位置映射

LLVM IR server 相比 clangd 更简单的地方是语义对象；更麻烦的地方往往是：

- parser 不是为编辑器交互式导航设计的
- token/source range 的稳定映射要自己补
- 文本改动后旧对象失效，需要按版本管理缓存

## 9. 并发、取消和版本一致性

第一版即使不用复杂线程模型，也要提前约束好版本语义。

### 9.1 推荐的最小策略

- I/O 主线程负责收发 JSON-RPC
- 分析任务丢给后台 worker
- 每个文档只保留最新版本分析任务
- 分析完成后，只有当版本仍然匹配最新文档时才发布结果

### 9.2 为什么这很重要

否则用户快速输入时会出现：

- diagnostics 闪回旧版本
- hover 跳到旧位置
- references 结果和当前文本不一致

这一点 `clangd` 的 `TUScheduler` 做得非常重；LLVM IR server 第一版可以先做简单版“按文档串行 + 过期结果丢弃”。

## 10. 一个适合 LLVM IR 的 MVP 范围

如果目标是两到三周内做出“可用的一版”，建议范围收敛为：

### 10.1 必做

- `initialize`
- `shutdown`
- `exit`
- `didOpen`
- `didChange`（full sync）
- `didClose`
- `publishDiagnostics`
- `hover`
- `definition`
- `references`
- `documentSymbol`

### 10.2 建议第二阶段做

- `completion`
- `semanticTokens/full`
- `rename`

### 10.3 可以更后做

- `codeAction`
- `formatting`
- `workspaceSymbol`
- 跨文件索引

## 11. 一个可落地的 C++ 代码架构草图

如果用 C++ 实现，一个比较自然的目录/模块切分可以是：

```text
llvm-ir-lsp/
  main.cpp
  transport/
    JSONTransport.h
    JSONTransport.cpp
  lsp/
    Protocol.h
    Protocol.cpp
    LSPServer.h
    LSPServer.cpp
  ir/
    DocumentStore.h
    DocumentStore.cpp
    IRParser.h
    IRParser.cpp
    IRDocument.h
    IRDocument.cpp
    SymbolIndex.h
    SymbolIndex.cpp
    SourceMap.h
    SourceMap.cpp
  features/
    Hover.cpp
    Definition.cpp
    References.cpp
    DocumentSymbol.cpp
    Completion.cpp
    SemanticTokens.cpp
    Rename.cpp
```

职责大致如下：

- `main.cpp`
  - 启动 server，连接 transport 和 protocol 层
- `JSONTransport`
  - 负责 JSON-RPC 收发
- `LSPServer`
  - 注册和分发 LSP 方法
- `DocumentStore`
  - 管理文档生命周期和版本
- `IRParser`
  - 把文本解析成 LLVM `Module`
- `IRDocument`
  - 聚合单文档分析结果
- `SymbolIndex`
  - 支撑 definition/references/rename
- `SourceMap`
  - 解决位置映射问题
- `features/*`
  - 把语义查询翻译成具体 LSP 返回值

## 12. 对 LLVM IR LSP 来说，最容易踩坑的地方

### 12.1 只靠字符串匹配做语义

第一版可以用词法扫描辅助定位，但核心语义最好还是挂到 LLVM IR 对象或自建 symbol graph 上。否则 rename、references 很快会不可靠。

### 12.2 一开始就追求增量解析

对 LLVM IR 来说，先做 full sync + 全量重建通常更划算。等基本功能稳定后，再考虑更细粒度缓存。

### 12.3 低估位置映射复杂度

这通常比“把 `.ll` parse 成 `Module`”更难，也更影响最终可用性。

### 12.4 过早做跨文件全局索引

LLVM IR 的本地导航功能价值已经很高。跨文件索引可以等本地版本打稳后再上。

## 13. 建议的开发顺序

### 阶段 1：打通骨架

- `initialize`
- `didOpen`
- `didChange`
- `publishDiagnostics`

目标是：编辑器里能实时看到 LLVM parser/verifier 的错误。

### 阶段 2：打通位置映射

- tokenization
- source range / IR object 映射
- hover

目标是：光标落到 `%x`、`@foo`、`label %bb1` 上能得到稳定结果。

### 阶段 3：导航能力

- definition
- references
- documentSymbol

目标是：具备基本“看 IR”工作流。

### 阶段 4：增强交互

- completion
- semantic tokens
- rename

## 14. 结合 NotDec 场景的价值

如果把 LLVM IR LSP server 放在 NotDec 语境里，它不只是“给 `.ll` 做编辑器支持”，还可以自然延伸到一些反编译工作流：

- 对中间产物 `.ll` 提供更强的导航和诊断
- 对类型恢复、内存恢复生成的具名值做 hover 展示
- 把 pass 注解、恢复结果、调试元信息以 hover 或 code lens 形式暴露出来
- 对 workdir 中的 `00-lifted.ll`、`01-Optimized.ll` 等中间文件提供一致的查看体验

也就是说，先做通用 LLVM IR LSP，再叠加 NotDec 特有语义，是一条很自然的演进路径。

## 15. 结论

一版 LSP server 的核心不是“接口多不多”，而是三件事：

1. 文档版本和文本同步是否正确
2. 文本位置能否稳定映射到语义对象
3. 语义结果能否在最新版本上可靠返回

对 LLVM IR 来说，第一版最值得优先实现的接口是：

- 生命周期：`initialize` / `shutdown` / `exit`
- 文本同步：`didOpen` / `didChange` / `didClose`
- 反馈：`publishDiagnostics`
- 导航：`hover` / `definition` / `references` / `documentSymbol`

背后的实现重心则不是复杂编译系统，而是：

- 复用 LLVM parser/verifier
- 建立单文档语义索引
- 做扎实的位置映射与版本管理

如果这些基础打稳，后面的 `completion`、`semanticTokens`、`rename` 都会顺很多。
