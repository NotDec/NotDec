# 2026-04-08 HType Top/Bottom 与 Snapshot Position 记录

## 背景

本次会话围绕 `tr-level=2` 的 HType 快照进一步缩小 `HType` 与 `UType` 之间的语义差距，并继续改进 `.htypes` 的可读性与可解释性。

目标主要分为三部分：

1. 让 `HType` 直接保留 `Top/Bottom`，而不是在导出阶段提前退化为 `i32/i64`
2. 在 `.htypes` 中为 value 标出 position，是 `+` 还是 `-`
3. 清理 snapshot 中无意义的空注释 `/* */`

---

## 一、HType 直接支持 Top / Bottom

### 动机

当前 `binarysub::UType` 已经有：

- `UTop { size }`
- `UBot { size }`

但原先 `TypeBuilder` 在把 `UType` 转为 `HType` 时，会把 `UTop/UBot` 统一回退成“指针宽度整数”，例如 wasm32 下变成 `i32`。

这会带来两个问题：

1. 丢失和 `UType` 的一一对应关系
2. 在 `.htypes` 里把“未知上界/下界”误显示成普通整数，干扰后续分析

### 实现

在 `external/NotDec-llvm2c` 的 `HType` 体系中新增：

- `TopType`
- `BottomType`

并在 `HTypeContext` 中新增对应 factory。

随后在 `src/TypeRecovery/mlsub/TypeBuilder.cpp` 中把：

- `UTop -> HType::TopType`
- `UBot -> HType::BottomType`

而不是再映射成 `getIntPtr()`。

同时补齐了：

- `HType::getAsString()`
- `HTypeSnapshotFormatter::formatType()`
- `llvm2c` 的 `TypeManager` lowering fallback

这样 backend 侧仍然可以继续把 `Top/Bottom` 保守降为整数 fallback，但 snapshot 本身不再提前丢语义。

### 结果

例如原先：

```text
main::%bb.allocator.i1 => i32
```

现在会更接近真实推理结果：

```text
main::%bb.allocator.i1 => bottom:32
```

这也解释了此前 `06_SimpleRecursive2.htypes` 中 `ptrtoint` 为什么显示成 `i32`：并不是它真被恢复成普通整数，而是此前 `UBot` 在 HType 层被过早回退成了整型显示。

### 相关提交

子模块 `external/NotDec-llvm2c`：

- `b982635` `Support HType top and bottom bounds`

主仓库：

- `d0f7408` `Preserve top and bottom in recovered HTypes`

---

## 二、在 `.htypes` 中标记 value position 为 `+/-`

### 动机

当前 mlsub / binarysub 内部在求解时天然区分 polarity：

- covariant 对应 `+`
- contravariant 对应 `-`

调试文件 `NOTDEC_TYPE_RECOVERY_DEBUG_DIR/ValueTypes.txt` 已经会输出：

```text
[+] Value: ...
[-] Value: ...
```

但最终 `.htypes` 快照之前没有保留这部分信息，导致：

- 快照和调试视图之间语义不一致
- 用户无法直接从 golden 里判断某个 value 是正位还是负位

### 实现

在 `HTypeResult` 中增加：

- `std::set<ExtValuePtr> ContraVariantValues`

快照打印时，`[types]` 段改为输出：

```text
[+] key => type
[-] key => type
```

同时在 mlsub 结果汇总时增加一层持久化：

- `ConstraintsGenerator::SnapshotContraVariantValues`

原因是 `genTypes()` 之后会释放一部分中间状态，不能依赖释放后的临时集合。

### 结果

例如 stack alloca 之类的值在 snapshot 中现在会显示为：

```text
[-] main::%stackPtr => ...
```

而普通 value 则是：

```text
[+] main::%stack => ...
```

这让 `.htypes` 更接近 mlsub 内部真实求解视图。

### 相关提交

子模块 `external/NotDec-llvm2c`：

- `b02c09d` `Annotate HType snapshots with value polarity`

主仓库：

- `016d56e` `Record value polarity in HType snapshots`

---

## 三、空注释不再打印 `/* */`

### 动机

在若干 `.htypes` case 中，结构体字段和声明尾部会出现：

```text
field_0; /*  */
}; /*  */
```

这些空注释不承载有效信息，只会增加 diff 噪音，也影响阅读。

### 实现

在 `external/NotDec-llvm2c/lib/notdec-llvm2c/Interface/HType.cpp` 中统一增加了“仅当 comment 非空时才打印注释”的逻辑，覆盖了两条路径：

1. `RecordDecl/UnionDecl/TypedefDecl::print()`
2. `HTypeSnapshotFormatter::formatDecl()`

### 结果

现在空注释会直接省略，例如：

```text
struct struct_0 {
  ... field_0;
};
```

受影响的 golden 主要是：

- `02_ConstantAddr1.htypes`
- `06_SimpleRecursive2.htypes`
- `08_Memory1.htypes`

### 相关提交

子模块 `external/NotDec-llvm2c`：

- `33bc8d6` `Skip empty comments in HType snapshots`

主仓库：

- `073af3a` `Omit empty HType snapshot comments`

---

## 四、测试与验证

本次会话中，相关修改的中间态与最终态都多次通过了：

```bash
ctest --test-dir build -R notdec.type_recovery.llvm_ir.tr_level_2 --output-on-failure
```

验证要点包括：

1. 仅引入 `Top/Bottom` 的中间态，suite 通过
2. 再引入 `+/-` position 标记后，更新 golden，suite 通过
3. 最后清理空注释并更新少量 golden 后，suite 继续通过

单例验证中也重点检查了：

- `test/decompile/llvm-ir/cases/06_SimpleRecursive2.ll`
- 对应的 `06_SimpleRecursive2.htypes`

确认：

- `bottom:32` 能正确保留
- `[-]` / `[+]` 能正确打印
- 空 `/* */` 已不再出现

---

## 五、影响文件概览

### 主仓库

- `include/notdec/TypeRecovery/mlsub/TypeBuilder.h`
- `src/TypeRecovery/mlsub/TypeBuilder.cpp`
- `include/notdec/TypeRecovery/mlsub/MLsubGenerator.h`
- `src/TypeRecovery/mlsub/MLsubGenerator.cpp`
- `test/decompile/llvm-ir/expected/type-recovery-tr-level-2/*.htypes`

### 子模块 `external/NotDec-llvm2c`

- `include/notdec-llvm2c/Interface/HType.h`
- `include/notdec-llvm2c/Interface.h`
- `lib/notdec-llvm2c/Interface/HType.cpp`
- `lib/notdec-llvm2c/TypeManager.cpp`

---

## 六、当前状态

当前这几批修改都已经提交完成，不是 WIP。

会话结束时主仓库工作树状态为：

- 代码改动已提交
- 仅剩未跟踪目录 `.codex/`

后续如果继续推进，可优先考虑：

1. 评估 `Top/Bottom` 在更多 backend 展示路径中的保留策略
2. 决定 `.htypes` 中是否还要继续加入 lower/upper 或更多解释性字段
3. 继续减少 snapshot 与 `ValueTypes.txt` 调试输出之间的语义差距
