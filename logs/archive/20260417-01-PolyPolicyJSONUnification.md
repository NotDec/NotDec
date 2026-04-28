# 2026-04-17 Poly Policy JSON Unification

## 背景

把 MLsub 里的 `NOTDEC_POLY_FUNCS` 和 `NOTDEC_LEVEL_OVERRIDE` 合并为单一
policy 文件入口，只保留 `NOTDEC_POLY_FUNCS`。新格式要求
`NOTDEC_POLY_FUNCS` 指向一个 JSON object：

```json
{
  "version": 1,
  "poly_funcs": ["malloc", "free"],
  "level_override": {
    "malloc": 1
  }
}
```

本次修改明确不兼容旧的纯数组格式，也不再读取 `NOTDEC_LEVEL_OVERRIDE`。

## 代码修改

1. 在 [src/TypeRecovery/mlsub/MLsubGenerator.cpp:50](/sn640/NotDec/src/TypeRecovery/mlsub/MLsubGenerator.cpp#L50) 到 [src/TypeRecovery/mlsub/MLsubGenerator.cpp:53](/sn640/NotDec/src/TypeRecovery/mlsub/MLsubGenerator.cpp#L53) 新增 `PolyPolicyConfig`
   - 涉及函数：匿名 namespace 内的配置解析辅助结构
   - 职责：同时承载 `poly_funcs` 和 `level_override`

2. 在 [src/TypeRecovery/mlsub/MLsubGenerator.cpp:90](/sn640/NotDec/src/TypeRecovery/mlsub/MLsubGenerator.cpp#L90) 到 [src/TypeRecovery/mlsub/MLsubGenerator.cpp:98](/sn640/NotDec/src/TypeRecovery/mlsub/MLsubGenerator.cpp#L98) 新增 `failPolyPolicy()`
   - 涉及函数：`failPolyPolicy`
   - 职责：为 `NOTDEC_POLY_FUNCS` policy 提供专用报错信息

3. 在 [src/TypeRecovery/mlsub/MLsubGenerator.cpp:149](/sn640/NotDec/src/TypeRecovery/mlsub/MLsubGenerator.cpp#L149) 到 [src/TypeRecovery/mlsub/MLsubGenerator.cpp:193](/sn640/NotDec/src/TypeRecovery/mlsub/MLsubGenerator.cpp#L193) 新增 `loadPolyPolicyConfig()`
   - 涉及函数：`loadPolyPolicyConfig`
   - 职责：
     - 读取 `NOTDEC_POLY_FUNCS`
     - 要求顶层为 object
     - 从 `poly_funcs` 读取多态函数集合
     - 从可选的 `level_override` 读取每函数 level 下界

4. 在 [src/TypeRecovery/mlsub/MLsubGenerator.cpp:2116](/sn640/NotDec/src/TypeRecovery/mlsub/MLsubGenerator.cpp#L2116) 到 [src/TypeRecovery/mlsub/MLsubGenerator.cpp:2124](/sn640/NotDec/src/TypeRecovery/mlsub/MLsubGenerator.cpp#L2116) 修改 `MLsubRecovery::prepareSCC()`
   - 涉及函数：`MLsubRecovery::prepareSCC`
   - 职责：
     - 删除独立的 `NOTDEC_LEVEL_OVERRIDE` 读取逻辑
     - 改为统一从 `loadPolyPolicyConfig()` 取得 `PolyFuncs` 和 `LevelOverrides`

5. 在 [src/Passes/ConstraintGenerator.cpp:84](/sn640/NotDec/src/Passes/ConstraintGenerator.cpp#L84) 到 [src/Passes/ConstraintGenerator.cpp:108](/sn640/NotDec/src/Passes/ConstraintGenerator.cpp#L108) 新增 `loadPolyFuncs()`
   - 涉及函数：`loadPolyFuncs`
   - 职责：
     - 让旧 `ConstraintGenerator` 路径也读取新的 object 格式
     - 只消费 `poly_funcs`，忽略 `level_override`

6. 在 [src/Passes/ConstraintGenerator.cpp:975](/sn640/NotDec/src/Passes/ConstraintGenerator.cpp#L975) 到 [src/Passes/ConstraintGenerator.cpp:985](/sn640/NotDec/src/Passes/ConstraintGenerator.cpp#L975) 修改 `TypeRecovery::prepareSCC()`
   - 涉及函数：`TypeRecovery::prepareSCC`
   - 职责：改为通过 `loadPolyFuncs()` 读取新的 `NOTDEC_POLY_FUNCS` policy

7. 更新外部 policy 数据文件
   - 涉及文件：
     - `/sn640/NotDec-Exp/ICSE-HOWARD/polymorphic_funcs.json`
     - `/sn640/NotDec-Exp/submit-HOWARD/polymorphic_funcs.json`
   - 职责：
     - 把旧数组格式改成新的 object 格式
     - 保持原路径不变，避免现有 launch / Makefile 入口直接失效

## 验证

1. 构建验证
   - 命令：`cmake --build ./build --target notdec-decompile -j2`
   - 结果：通过

2. 运行验证
   - 临时 policy：`/tmp/notdec_poly_policy.json`
   - 命令：
     `NOTDEC_POLY_FUNCS=/tmp/notdec_poly_policy.json ./build/bin/notdec test/type-recovery/howard-o3-split/cases/fortune.o3.wasm.4.ll -o /tmp/fortune.poly-policy.out.ll --tr-level=2`
   - 结果：通过，成功产出 `/tmp/fortune.poly-policy.out.ll`

3. 现有 policy 路径回归验证
   - 命令：
     `NOTDEC_POLY_FUNCS=/sn640/NotDec-Exp/ICSE-HOWARD/polymorphic_funcs.json ./build/bin/notdec test/type-recovery/howard-o3-split/cases/fortune.o3.wasm.4.ll -o /tmp/fortune.poly-policy.realpath.out.ll --tr-level=2`
   - 结果：通过，成功产出 `/tmp/fortune.poly-policy.realpath.out.ll`
