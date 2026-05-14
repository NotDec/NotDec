# 2026-04-20 Binarysub UType Variable Origin Tracking

## 目标

在 `external/binarysub` 的类型简化链路里，持续保留原始 `VariableState::id`
到最终 `UType` 类型变量的对应关系，支持：

- 给定原始 `variableState` 编号，查询最终 `UType` 里的类型变量编号
- 给定最终 `UType` 类型变量编号，反查其对应的原始 `variableState` 编号集合

## 修改明细

### 1. 在 `UTypeVariable` 上显式保存最终编号与原始来源集合

- 文件：`external/binarysub/include/binarysub/binarysub.h`
- 行：29-34, 116-123, 173-185
- 涉及：
  - `struct binarysub::UTypeVariable`
  - `make_utypevariable(...)`
  - `collect_utype_variable_origins(...)`
  - `collect_original_to_utype_variable_ids(...)`
  - `find_utype_variable_ids_for_original(...)`
  - `find_original_variable_ids_for_utype(...)`

本次把 `UTypeVariable` 从原先的 `{name, size}` 扩展为：

- `name`
- `size`
- `id`
- `originIds`

并补了双向查询 helper，避免下游再靠 `printType()` 文本做反推。

### 2. 在 simplifier 内部维护 `SimpleType variable -> origin ids`

- 文件：`external/binarysub/src/binarysub.cpp`
- 行：877-1032, 1217-1363, 1555-1669
- 涉及：
  - `binarysub::TypeSimplifier::canonicalizeType`
  - `binarysub::TypeSimplifier::simplifyType`
  - `binarysub::TypeSimplifier::coalesceCompactType`

具体处理：

- `canonicalizeType()` 里首次见到原始变量时，初始化 `variableOrigins[var] = {var.id}`
- 对 canonicalize 阶段新建的递归 fresh variable，继承当前 compact 变量组的原始 id 并集
- `simplifyType()` 里发生 co-occurrence unify 时，把被并入变量的 `originIds`
  合并到保留变量
- `coalesceCompactType()` 生成最终 `UTypeVariable` 时，把：
  - 最终 `id`
  - 合并后的 `originIds`
  一起写入 `UType`
- 对 coalesce 阶段为递归结构临时生成的 `μN` 变量，补了 synthetic final id，
  避免最终 `UType` 里存在“有名字但没编号”的类型变量

### 3. 保证 rename/normalize 后元数据不丢

- 文件：`external/binarysub/src/binarysub.cpp`
- 行：196-360
- 涉及：
  - `binarysub::normalizeVariableNames`
  - `collect_utype_variable_origins`
  - `collect_original_to_utype_variable_ids`

`normalizeVariableNames()` 现在只改显示名，不会清掉 `id` 和 `originIds`。

### 4. 增加定向回归测试

- 文件：`external/binarysub/src/binarysub-test.cpp`
- 行：406-468
- 涉及：
  - `test_utype_variable_origin_tracking`

- 文件：`external/binarysub/include/binarysub/binarysub-test.h`
- 行：4-16
- 涉及：
  - `test_utype_variable_origin_tracking` 声明

- 文件：`external/binarysub/src/binarysub-test-main.cpp`
- 行：35-39
- 涉及：
  - `main()`

测试直接手工创建两个 `VariableState`，通过 `constrain(lhs, rhs)` 与
`TypeSimplifier::simplify()` 触发变量合并，然后检查：

- 最终只剩一个 `UType` 类型变量
- 该变量的 `originIds == {lhsId, rhsId}`
- 正反两个 helper 的查询结果都一致

## 验证

执行：

```bash
cmake --build build --target binarysub -j4
./build/binarysub
cmake --build build --target notdec-decompile -j4
```

结果：

- `binarysub` 自测全部通过
- `notdec-decompile` 编译通过

## 备注

当前这次改动只把“编号关系”维护到了最终 `UType` 和 `binarysub` helper API。
如果后续需要把这层映射继续透传到 `MLsubGenerator` 的 JSON / debug dump，
可以直接从最终 `UTypeVariable::{id, originIds}` 取数，不需要再回看 simplifier 内部状态。
