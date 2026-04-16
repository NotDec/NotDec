# MLsub 输入 IR 锚点导出实现

日期：2026-04-16

## 1. 本次范围

这次先实现规划中的一小块基础设施：在 `MLsub` 开始分析前，除了继续导出
`02-mlsub-input.ll`，再额外导出一份机器可读的 IR 锚点文件
`02-mlsub-input.anchor.json`。

本次没有实现：

- JSON 额外约束注入
- selector 解析
- 约束文件与 IR 锚点的双向校验
- `sha256` 字段

## 2. 具体修改

### 2.1 `src/TypeRecovery/mlsub/MLsubGenerator.cpp`

1. 在 [src/TypeRecovery/mlsub/MLsubGenerator.cpp:48](/sn640/NotDec/src/TypeRecovery/mlsub/MLsubGenerator.cpp#L48)
   到 [src/TypeRecovery/mlsub/MLsubGenerator.cpp:55](/sn640/NotDec/src/TypeRecovery/mlsub/MLsubGenerator.cpp#L55)
   新增 `kMLsubInputIRFile` 和 `kMLsubInputAnchorFile` 常量，
   明确 `MLsub` 输入 IR 与其锚点 JSON 的 workdir 文件名。
2. 在 [src/TypeRecovery/mlsub/MLsubGenerator.cpp:146](/sn640/NotDec/src/TypeRecovery/mlsub/MLsubGenerator.cpp#L146)
   到 [src/TypeRecovery/mlsub/MLsubGenerator.cpp:185](/sn640/NotDec/src/TypeRecovery/mlsub/MLsubGenerator.cpp#L185)
   新增三个 helper：
   - `computeFileMD5Hex()`
   - `writeJSONFile()`
   - `writeMLsubInputAnchor()`
3. 在 [src/TypeRecovery/mlsub/MLsubGenerator.cpp:728](/sn640/NotDec/src/TypeRecovery/mlsub/MLsubGenerator.cpp#L728)
   到 [src/TypeRecovery/mlsub/MLsubGenerator.cpp:733](/sn640/NotDec/src/TypeRecovery/mlsub/MLsubGenerator.cpp#L733)
   修改 `MLsubRecovery::run()`：
   - 继续导出 `02-mlsub-input.ll`
   - 紧接着导出 `02-mlsub-input.anchor.json`

当前锚点 JSON 包含：

- `version`
- `stage`
- `ir_file`
- `content_hash.algorithm`
- `content_hash.value`
- `data_layout`
- `target_triple`

说明：

- 当前内容摘要先落成 `md5`，并显式把算法写入 JSON，避免后续升级为
  `sha256` 时产生字段语义歧义。

### 2.2 `DEBUG.md`

1. 在 [DEBUG.md:64](/sn640/NotDec/DEBUG.md#L64)
   到 [DEBUG.md:76](/sn640/NotDec/DEBUG.md#L76)
   补充 `02-mlsub-input.ll` 与 `02-mlsub-input.anchor.json` 的用途说明。
2. 在 [DEBUG.md:136](/sn640/NotDec/DEBUG.md#L136)
   到 [DEBUG.md:146](/sn640/NotDec/DEBUG.md#L146)
   更新推荐阅读顺序，把锚点 JSON 放到 `02-mlsub-input.ll` 之后。

## 3. 验证

### 3.1 编译

执行：

```bash
cmake --build ./build --target notdec-decompile
```

结果：通过。

### 3.2 冒烟运行

执行：

```bash
./build/bin/notdec test/type-recovery/llvm-ir/cases/01_Simple1.ll \
  -o /tmp/notdec-mlsub-anchor-smoke.out.ll \
  --tr-level=2 \
  --gen-work-dir \
  --work-dir=/tmp/notdec-mlsub-anchor-smoke
```

结果：

- workdir 中出现 `02-mlsub-input.ll`
- workdir 中出现 `02-mlsub-input.anchor.json`
- `02-mlsub-input.anchor.json` 成功写出 `stage/data_layout/target_triple`
  和 `content_hash`

## 4. 后续建议

下一步可以继续接：

1. 让约束 JSON 预留 `ir_anchor` 结构
2. 在读取额外约束文件时校验 `stage`
3. 再补 `sha256` 或统一升级到更强 hash 算法
