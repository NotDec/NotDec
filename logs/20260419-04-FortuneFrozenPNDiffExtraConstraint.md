# fortune frozen realworld case 补 `PNDiff` extra constraint

日期：2026-04-19

## 1. 背景

`work_dir/PNDiff.warn.txt:9-21` 还剩两条 `Add` 约束没有被 `PNDiff` 自动区分成
数字类型：

1. `add_file::%bb.brif_next127.i1`
   - IR: `%96 = add i32 %95, 1`
   - 对应源码：`fortune.c:829-831`
   - 实际语义是 `fp->num_children++`
2. `get_tbl::%bb.loop_entry.i7`
   - IR: `%76 = add i32 %72, %75`
   - 对应源码：`fortune.c:1450`
   - 实际语义是 `t1->str_numstr += t2->str_numstr`

这两条都落在同一个大 SCC 里，但 `binarysub-trace.log` 显示求解时都走了：

- `reason=left-equals-result`

也就是把 `result` 直接 alias 到 `op0`，没有额外证据把这一组强行推成 `number`。

## 2. use trace 结论

### 2.1 `add_file::%bb.brif_next127.i1`

查看 `work_dir/03-pndiff-final.ll:2628,2657-2658,2720-2722`：

1. 先 `store 0`
2. 再 `load`
3. 做 `icmp eq 0`
4. 做 `add 1`
5. 再 `store` 回去

这组 use 本身已经很像普通计数器；结合源码可确认它就是 `fp->num_children`。

### 2.2 `get_tbl::%bb.loop_entry.i7`

查看 `work_dir/03-pndiff-final.ll:3529,3533-3534`：

1. 从字段位置 `load`
2. 跟另一个 `num` 节点做 `add`
3. `store` 回字段

这里 use-site 只有“读字段 + 相加 + 写回字段”，单靠 IR 没有像 `icmp` / 索引那样更强的
数字证据；但源码上它明确是 `STRFILE::str_numstr` 的累加。

## 3. 修改

### 3.1 suite manifest 切到 frozen stage-B 输入

修改位置：

- `test/type-recovery/realworld/manifest.json:3-9`
- `test/type-recovery/realworld/manifest.json:15-17`

涉及：

1. 默认参数固定带 `--frozen-tr-input-ir`
2. 通过 `env.NOTDEC_EXTRA_CONSTRAINTS` 注入额外约束 JSON
3. case 输入切到 `cases/fortune.o3.wasm.ll`，并直接用这个文件名承载 frozen
   stage-B 输入，避免并存两份 `fortune` case

### 3.2 为两个 residual `Add` 增加 `inst` 级别 override

修改位置：

- `test/type-recovery/realworld/support/fortune.o3.wasm.extra.json:3-30`

涉及函数：

1. `add_file`
   - selector: `add_file::%bb.brif_next127.i1`
2. `get_tbl`
   - selector: `get_tbl::%bb.loop_entry.i7`

两个 action 都是：

- `kind = "pndiff"`
- `target.kind = "inst"`
- `state = "number"`

另外这次把 `ir_anchor.sha256` 更新成 suite runner 实际使用的绝对路径加载版本：

- `ebb18246a7d2c849aea678b4379399c10143d04dfce136ed3610045981c2c62d`

原因是 runner 会先把 case 路径 `resolve()` 成绝对路径，再交给 `notdec`；LLVM 打印
出来的 `ModuleID` 会因此变成绝对路径，从而影响 `renderModuleToString(M)` 的 SHA。

### 3.3 README 补充 anchor 约定

修改位置：

- `test/type-recovery/realworld/README.md:12-16`

说明 `support/fortune.o3.wasm.extra.json` 的 anchor hash 需要匹配
`ctest`/suite runner 的绝对路径加载形态，避免手工相对路径运行时生成的 SHA 误填进去。

## 4. 验证

### 4.1 直接跑 suite runner

```bash
python3 test/run_type_recovery_suite.py \
  --binary ./build/bin/notdec \
  --manifest ./test/type-recovery/realworld/manifest.json \
  --project-root . \
  --workdir /tmp/notdec-runner-realworld
```

结果：

- `fortune.o3.wasm` 通过

### 4.2 跑 CTest

```bash
ctest --test-dir build -R notdec.type_recovery.realworld.tr_level_2 --output-on-failure
```

结果：

- `1/1` 通过

## 5. 额外说明

这次没有去手改 frozen `.ll` 里单条 SSA 的匿名编号名字。原因是 LLVM 文本 IR 的匿名
编号是顺序敏感的，只改其中一个 `%96` / `%76` 会破坏后续编号约束，解析会直接失败。
当前更稳妥的做法就是继续使用 `SelectableValues.txt` 导出的稳定 `inst` selector。
