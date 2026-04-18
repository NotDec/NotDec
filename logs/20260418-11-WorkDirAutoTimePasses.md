# 20260418-11 WorkDir Auto TimePasses

## 1. 背景

在 `fortune.o3.wasm.4.ll` 上做 profile 时，LLVM pass timing report 依赖：

```bash
--time-passes --info-output-file=/tmp/fortune.timepasses.txt
```

其中 `fortune.timepasses.txt` 不是 NotDec 自己写的，而是 LLVM 全局
CommandLine 选项 `--time-passes` / `--info-output-file` 的输出。

当前仓库已经有 `--gen-work-dir` / `--work-dir` 机制，但之前不会自动把这份
timing report 放进 work dir，每次都得手工补 `--info-output-file=...`。

## 2. 修改

1. 在 [src/NotDec.cpp](/sn640/NotDec/src/NotDec.cpp#L167) 新增
   `configureWorkDirLLVMReports(const notdec::Options &Opts)`。
   - 读取 LLVM 注册选项 `time-passes`、`time-passes-per-run`、
     `info-output-file`
   - 当满足以下条件时自动补默认输出路径：
     - 已启用 `workDir`
     - 用户显式打开了 `--time-passes` 或 `--time-passes-per-run`
     - 用户没有自己传 `--info-output-file`
   - 默认落盘到 `work_dir/02-time-passes.txt`
2. 在 [src/NotDec.cpp](/sn640/NotDec/src/NotDec.cpp#L283) 的 `main`
   中，在 `notdec::setWorkDir(opts.workDir)` 之后调用
   `configureWorkDirLLVMReports(opts)`，保证后续 pass 运行时 LLVM timer
   已经拿到输出文件。

## 3. 验证

执行：

```bash
env NOTDEC_POLY_FUNCS=/sn640/NotDec-Exp/ICSE-HOWARD/polymorphic_funcs.json \
  ./build/bin/notdec \
  test/type-recovery/howard-o3-split/cases/fortune.o3.wasm.4.ll \
  -o /tmp/fortune.autotime.out.ll \
  --tr-level=2 \
  --dump-htypes /tmp/fortune.autotime.out.htypes \
  -g --work-dir=/tmp/notdec-work-timepasses \
  --time-passes
```

结果：

1. 自动生成：
   - [/tmp/notdec-work-timepasses/02-time-passes.txt](/tmp/notdec-work-timepasses/02-time-passes.txt)
2. 文件内容是标准 LLVM pass timing report，可直接看到
   `notdec::mlsub::MLsubRecoveryMain` 仍然是主要热点。

## 4. 涉及函数

1. `configureWorkDirLLVMReports(const notdec::Options &Opts)`
   - 文件：[src/NotDec.cpp](/sn640/NotDec/src/NotDec.cpp#L167)
2. `main(int argc, char *argv[])`
   - 文件：[src/NotDec.cpp](/sn640/NotDec/src/NotDec.cpp#L205)
