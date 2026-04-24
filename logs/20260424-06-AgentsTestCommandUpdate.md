# 2026-04-24 AGENTS 常用测试命令补充

## 背景

把这轮实际反复使用的两个局部验证入口补到 `AGENTS.md`：

- `./build/bin/TypeBuilderTest`
- `./build/binarysub`

这样后面做类型恢复和 `binarysub` 相关改动时，不用再临时翻历史记录找命令。

## 代码修改

1. `AGENTS.md:249-251`
   - 修改内容：
     - 在 `## 8. 测试` 的典型命令里追加：
       - `cmake --build ./build --target TypeBuilderTest binarysub -j4`
       - `./build/bin/TypeBuilderTest`
       - `./build/binarysub`

## 备注

这次只是补充命令入口，没有改测试策略，也没有新增 suite。
