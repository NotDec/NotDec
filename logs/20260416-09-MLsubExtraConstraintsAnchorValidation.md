# MLsub 额外约束文件锚点校验实现

日期：2026-04-16

## 1. 本次范围

本次继续只落一小块和约束注入直接相关、但不进入 selector/action 本体的功能：

1. 新增 `NOTDEC_EXTRA_CONSTRAINTS`
2. 允许 `MLsub` 在开始分析前读取一份 JSON
3. 当前只支持校验顶层 `ir_anchor`
4. 如果文件里出现非空 `functions`，直接报“尚未实现”，避免误以为函数级约束已经生效

本次仍未实现：

- `bindings`
- `actions`
- `arg/ret/inst/operand/binding` selector 解析
- `subtype/equals/pndiff` 注入

## 2. 具体修改

### 2.1 `include/notdec/TypeRecovery/mlsub/MLsubGenerator.h`

1. 在 [include/notdec/TypeRecovery/mlsub/MLsubGenerator.h:340](/sn640/NotDec/include/notdec/TypeRecovery/mlsub/MLsubGenerator.h#L340)
   到 [include/notdec/TypeRecovery/mlsub/MLsubGenerator.h:346](/sn640/NotDec/include/notdec/TypeRecovery/mlsub/MLsubGenerator.h#L346)
   为 `MLsubRecovery` 新增成员：
   - `ExtraConstraintsFile`
2. 在 [include/notdec/TypeRecovery/mlsub/MLsubGenerator.h:380](/sn640/NotDec/include/notdec/TypeRecovery/mlsub/MLsubGenerator.h#L380)
   到 [include/notdec/TypeRecovery/mlsub/MLsubGenerator.h:382](/sn640/NotDec/include/notdec/TypeRecovery/mlsub/MLsubGenerator.h#L382)
   声明：
   - `validateExtraConstraintsFile()`

### 2.2 `src/TypeRecovery/mlsub/MLsubGenerator.cpp`

1. 在 [src/TypeRecovery/mlsub/MLsubGenerator.cpp:72](/sn640/NotDec/src/TypeRecovery/mlsub/MLsubGenerator.cpp#L72)
   到 [src/TypeRecovery/mlsub/MLsubGenerator.cpp:79](/sn640/NotDec/src/TypeRecovery/mlsub/MLsubGenerator.cpp#L79)
   新增 `failExtraConstraints()`，用于单独报告额外约束文件错误。
2. 在 [src/TypeRecovery/mlsub/MLsubGenerator.cpp:156](/sn640/NotDec/src/TypeRecovery/mlsub/MLsubGenerator.cpp#L156)
   到 [src/TypeRecovery/mlsub/MLsubGenerator.cpp:170](/sn640/NotDec/src/TypeRecovery/mlsub/MLsubGenerator.cpp#L170)
   将 `MLsub` 输入 IR 摘要从“读 workdir 文件后再 hash”改成：
   - `renderModuleToString()`
   - `computeMD5Hex()`
   直接对当前模块文本求摘要
3. 在 [src/TypeRecovery/mlsub/MLsubGenerator.cpp:425](/sn640/NotDec/src/TypeRecovery/mlsub/MLsubGenerator.cpp#L425)
   到 [src/TypeRecovery/mlsub/MLsubGenerator.cpp:486](/sn640/NotDec/src/TypeRecovery/mlsub/MLsubGenerator.cpp#L486)
   新增：
   - `validateExtraConstraintsAnchor()`
   - `validateUnsupportedExtraConstraintBodies()`
4. 在 [src/TypeRecovery/mlsub/MLsubGenerator.cpp:884](/sn640/NotDec/src/TypeRecovery/mlsub/MLsubGenerator.cpp#L884)
   到 [src/TypeRecovery/mlsub/MLsubGenerator.cpp:903](/sn640/NotDec/src/TypeRecovery/mlsub/MLsubGenerator.cpp#L884)
   实现 `MLsubRecovery::validateExtraConstraintsFile()`：
   - 读取 JSON
   - 校验 `version == 1`
   - 校验 `ir_anchor`
   - 拒绝非空 `functions`
5. 在 [src/TypeRecovery/mlsub/MLsubGenerator.cpp:807](/sn640/NotDec/src/TypeRecovery/mlsub/MLsubGenerator.cpp#L807)
   到 [src/TypeRecovery/mlsub/MLsubGenerator.cpp:820](/sn640/NotDec/src/TypeRecovery/mlsub/MLsubGenerator.cpp#L820)
   修改 `MLsubRecovery::run()`：
   - 先计算当前模块的 `md5`
   - 继续导出 `02-mlsub-input.anchor.json`
   - 如果设置了 `NOTDEC_EXTRA_CONSTRAINTS`，则在进入 CallGraph / bottom-up 前先校验

当前支持的 `ir_anchor` 字段：

- `stage`
- `content_hash.algorithm`
- `content_hash.value`
- `data_layout`
- `target_triple`

当前规则：

1. `stage` 必须是 `mlsub-input`
2. `content_hash.algorithm` 目前只接受 `md5`
3. `content_hash.value` 不匹配直接报错
4. `data_layout` / `target_triple` 如果填写，则必须与当前模块一致
5. `functions` 只能缺省或为空对象

### 2.3 `DEBUG.md`

1. 在 [DEBUG.md:71](/sn640/NotDec/DEBUG.md#L71)
   到 [DEBUG.md:76](/sn640/NotDec/DEBUG.md#L76)
   补充 `NOTDEC_EXTRA_CONSTRAINTS` 当前能力说明：
   - 已支持 `ir_anchor` 校验
   - 函数级 `actions` 尚未实现

## 3. 验证

### 3.1 编译

执行：

```bash
cmake --build ./build --target notdec-decompile
```

结果：通过。

### 3.2 正例

先用：

```bash
./build/bin/notdec test/type-recovery/llvm-ir/cases/01_Simple1.ll \
  -o /tmp/notdec-extra-constraints-pass.out.ll \
  --tr-level=2 \
  --gen-work-dir \
  --work-dir=/tmp/notdec-extra-constraints-pass
```

拿到 `/tmp/notdec-extra-constraints-pass/02-mlsub-input.anchor.json` 的锚点值。

再执行：

```bash
NOTDEC_EXTRA_CONSTRAINTS=/tmp/notdec-extra-constraints-pass.json \
./build/bin/notdec test/type-recovery/llvm-ir/cases/01_Simple1.ll \
  -o /tmp/notdec-extra-constraints-pass-validated.out.ll \
  --tr-level=2
```

结果：

- 成功打印 `Loading MLsub extra constraints from: ...`
- 运行继续完成，没有因锚点校验失败而中断

### 3.3 反例

执行：

```bash
NOTDEC_EXTRA_CONSTRAINTS=/tmp/notdec-extra-constraints-fail.json \
./build/bin/notdec test/type-recovery/llvm-ir/cases/01_Simple1.ll \
  -o /tmp/notdec-extra-constraints-fail.out.ll \
  --tr-level=2
```

其中 `content_hash.value` 故意改错。

结果：

- 进程在 `MLsub` 开始前报错退出
- 报错位置为 `ir_anchor.content_hash.value`

## 4. 后续建议

下一步最自然的是继续接：

1. 允许 `functions` 非空
2. 先实现 `arg/ret/inst/operand` selector 解析
3. 再实现最小的 `actions.kind = pndiff` 或 `subtype`
