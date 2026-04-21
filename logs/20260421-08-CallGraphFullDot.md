# 2026-04-21 CallGraphFull.dot

## 背景

当前 workdir 里已有 `CallGraph.txt` 和 `CallGraph.dot`，其中
`CallGraph.dot` 会隐藏没有函数体的 external declaration。

这对先看主链路是有帮助的，但在排查：

- 某条调用边是否真的连到了 libc / wasm import
- 某个 override / summary 对应的 external 函数有没有进入 CallGraph

时，需要一个“不裁剪 external declaration”的完整版 dot。

## 本次修改

### 1. `CallGraphDOTInfo` 改成实例级控制是否隐藏 external declaration

文件：
[include/notdec/Utils/CallGraphDotInfo.h](/sn640/NotDec/include/notdec/Utils/CallGraphDotInfo.h)

涉及函数 / 逻辑：

- `notdec::utils::CallGraphDOTInfo::CallGraphDOTInfo`  
  行 26-32：新增 `bool HideExternalFunctions = true` 参数，并保存到成员
  `HideExternalFunctions`
- `notdec::utils::CallGraphDOTInfo::shouldHideExternalFunctions`  
  行 59：新增 getter，供 dot trait 查询实例配置
- `llvm::DOTGraphTraits<CallGraphDOTInfo *>::isNodeHidden`  
  行 127-135：由原来的全局常量切换为读取 `CGInfo` 实例配置

这样同一套 DOT trait 就可以同时支持：

- 现有简版 `CallGraph.dot`
- 新增完整版 `CallGraphFull.dot`

而不需要复制第二套 CallGraph dot 实现。

### 2. 给 external declaration 节点加轻量区分样式

文件：
[include/notdec/Utils/CallGraphDotInfo.h](/sn640/NotDec/include/notdec/Utils/CallGraphDotInfo.h)

涉及函数：

- `llvm::DOTGraphTraits<CallGraphDOTInfo *>::getNodeAttributes`  
  行 178-195

调整：

- 对 `F->isDeclaration()` 的节点，返回
  `style="filled,dashed", fillcolor="gray95", color="gray50"`
- 有函数体的函数仍保持原来的 heat-color 逻辑

这样完整版里 external declaration 会保留，但视觉上不会和有函数体的本地函数混在一起。

### 3. workdir 新增 `CallGraphFull.dot`

文件：
[src/TypeRecovery/mlsub/MLsubGenerator.cpp](/sn640/NotDec/src/TypeRecovery/mlsub/MLsubGenerator.cpp)

涉及函数：

- `MLsubRecovery::runOnModule`  
  行 2134-2163

调整：

- 行 2145-2153：现有 `CallGraph.dot` 显式改为
  `CallGraphDOTInfo(&M, &*CallG, nullptr, true)`，继续隐藏 external declaration
- 行 2155-2163：新增 `CallGraphFull.dot` 导出，使用
  `CallGraphDOTInfo(&M, &*CallG, nullptr, false)`，保留 external declaration

## 文档更新

文件：
[DEBUG.md](/sn640/NotDec/DEBUG.md)

涉及位置：

- 行 107-119

更新内容：

- 补充 `CallGraph.dot` 当前“默认隐藏 external declaration”的说明
- 新增 `CallGraphFull.dot` 的用途和节点样式说明

## 验证

### 编译

```bash
cmake --build /sn640/NotDec/build -j4
```

### 目标用例验证

```bash
/sn640/NotDec/build/bin/notdec \
  /sn640/NotDec/test/type-recovery/realworld/cases/fortune.o3.wasm.ll \
  -o /tmp/fortune-callgraph-full.ll \
  --tr-level=2 \
  -g \
  --work-dir=/tmp/notdec-callgraph-full
```

验证结果：

- 生成了 `/tmp/notdec-callgraph-full/CallGraph.dot`
- 生成了 `/tmp/notdec-callgraph-full/CallGraphFull.dot`
- `CallGraph.dot` 中未匹配到 `printf` / `malloc`
- `CallGraphFull.dot` 中能看到 `printf` / `malloc` 等 external declaration
- `CallGraphFull.dot` 里这些节点带
  `style="filled,dashed", fillcolor="gray95", color="gray50"`

本次修改是非侵入式的：只新增一个完整版 workdir 产物，并把 external 过滤从全局常量收敛成实例级配置，没有改动 CallGraph 构造或类型恢复主流程。
