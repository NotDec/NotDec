# MLsub 阶段 B 显式模式入口

日期：2026-04-17

## 1. 本次范围

沿着 [logs/20260417-09-MLsubStageBRemainingPlan.md](/sn640/NotDec/logs/20260417-09-MLsubStageBRemainingPlan.md)
里的第一步，先把“阶段 B 是显式模式”这层语义收紧起来，但这次还不拆
pass pipeline。

本次只完成：

1. 新增显式 CLI：
   - `--frozen-tr-input-ir`
2. 新增配置位，表示当前输入已经是冻结后的 TR input IR
3. 当启用 `NOTDEC_EXTRA_CONSTRAINTS` 时，强制要求传入
   `--frozen-tr-input-ir`
4. 禁止把 `NOTDEC_EXTRA_CONSTRAINTS` 和 `--emit-tr-input-ir` 混在同一条命令里
5. 文档里把“两阶段工作流”入口说明补齐

本次明确没有做：

1. 还没有把 `PassEnv::build_passes()` 拆成真正的阶段 A / 阶段 B
2. 也就是说，当前 `--frozen-tr-input-ir` 先只承担“显式语义与入口约束”
3. pipeline 真正拆分留到下一步

## 2. 代码修改

### 2.1 配置层新增 `frozenTRInputIR`

文件：

- [include/notdec/DecompilerContext.h:14](/sn640/NotDec/include/notdec/DecompilerContext.h#L14)
  到 [include/notdec/DecompilerContext.h:22](/sn640/NotDec/include/notdec/DecompilerContext.h#L22)

涉及结构：

- `notdec::Options`

修改内容：

1. 新增：
   - `bool frozenTRInputIR = false;`
2. 用来承载“当前输入已经是冻结 TR input IR”的显式运行语义

### 2.2 CLI 与组合校验

文件：

- [src/NotDec.cpp:112](/sn640/NotDec/src/NotDec.cpp#L112)
  到 [src/NotDec.cpp:123](/sn640/NotDec/src/NotDec.cpp#L112)
- [src/NotDec.cpp:151](/sn640/NotDec/src/NotDec.cpp#L151)
  到 [src/NotDec.cpp:219](/sn640/NotDec/src/NotDec.cpp#L151)

涉及函数：

- `main()`

修改内容：

1. 新增命令行参数：
   - `--frozen-tr-input-ir`
2. `main()` 中新增：
   - 读取 `NOTDEC_EXTRA_CONSTRAINTS`
3. 新增组合校验：
   - `--frozen-tr-input-ir` 不能和 `--emit-tr-input-ir` 同时使用
   - `NOTDEC_EXTRA_CONSTRAINTS` 要求显式传入 `--frozen-tr-input-ir`
   - `NOTDEC_EXTRA_CONSTRAINTS` 不能和 `--emit-tr-input-ir` 混用
4. 新增输入后缀校验：
   - `--frozen-tr-input-ir` 只接受 `.ll/.bc`
5. 把 `frozenTRInputIR` 写入 `notdec::Options`

### 2.3 调试文档更新

文件：

- [DEBUG.md:11](/sn640/NotDec/DEBUG.md#L11)
  到 [DEBUG.md:33](/sn640/NotDec/DEBUG.md#L11)
- [DEBUG.md:88](/sn640/NotDec/DEBUG.md#L88)
  到 [DEBUG.md:97](/sn640/NotDec/DEBUG.md#L88)

修改内容：

1. 在基本示例里加入：
   - `./build/bin/notdec /tmp/tr-input.ll -o /tmp/out.ll --tr-level=2 --frozen-tr-input-ir`
2. 明确说明：
   - `--frozen-tr-input-ir` 表示当前输入是阶段 A 导出的冻结 IR
   - 当前启用 `NOTDEC_EXTRA_CONSTRAINTS` 时必须带这个参数
   - `NOTDEC_EXTRA_CONSTRAINTS` 不能和 `--emit-tr-input-ir` 同时使用

## 3. 验证

### 3.1 编译

执行：

```bash
cmake --build ./build --target notdec-decompile
```

结果：通过。

### 3.2 缺少阶段 B 显式模式时拒绝运行

执行：

```bash
NOTDEC_EXTRA_CONSTRAINTS=/tmp/dummy.json \
./build/bin/notdec test/type-recovery/llvm-ir/cases/01_Simple1.ll \
  --tr-level=2 -o /tmp/out.ll
```

结果：

1. 直接报错：
   - `Error: NOTDEC_EXTRA_CONSTRAINTS requires --frozen-tr-input-ir.`
2. 同时给出两阶段工作流提示：
   - 先 `--emit-tr-input-ir`
   - 再在冻结 `.ll/.bc` 上重跑

### 3.3 阶段 A / 阶段 B 组合冲突时拒绝运行

执行：

```bash
NOTDEC_EXTRA_CONSTRAINTS=/tmp/dummy.json \
./build/bin/notdec test/type-recovery/llvm-ir/cases/01_Simple1.ll \
  --tr-level=2 \
  --emit-tr-input-ir=/tmp/tr-input.ll \
  --frozen-tr-input-ir
```

结果：

1. 直接报错：
   - `Error: --frozen-tr-input-ir cannot be combined with --emit-tr-input-ir.`

### 3.4 仅开启显式模式时不破坏现有路径

执行：

```bash
./build/bin/notdec test/type-recovery/llvm-ir/cases/01_Simple1.ll \
  --tr-level=2 \
  --frozen-tr-input-ir \
  -o /tmp/out-frozen.ll
```

结果：通过，成功写出 `/tmp/out-frozen.ll`。

### 3.5 当前关注 case 仍可正常运行

执行：

```bash
./build/bin/notdec test/type-recovery/howard-o3-split/cases/fortune.o3.wasm.1.ll \
  --tr-level=2 \
  --frozen-tr-input-ir \
  -o /tmp/fortune-frozen.ll
```

结果：通过，成功写出 `/tmp/fortune-frozen.ll`。

## 4. 当前判断

这一步做完后，至少“阶段 B 必须显式声明自己是阶段 B”这层语义已经落到正式
CLI 了。

但需要明确：

1. 当前只是入口收紧
2. `PassEnv::build_passes()` 还没有真正拆开
3. 所以下一步仍然应该回到：
   - 阶段 A 规范化 pass
   - 阶段 B `MLsubRecoveryMain`
   这两段的真正物理解耦
