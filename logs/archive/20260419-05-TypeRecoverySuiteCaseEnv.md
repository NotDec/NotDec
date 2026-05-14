# type recovery suite 支持 `case.env`

日期：2026-04-19

## 1. 背景

`test/type-recovery/realworld/manifest.json` 里的：

- `NOTDEC_EXTRA_CONSTRAINTS`

只服务于：

- `cases/fortune.o3.wasm.ll`

继续挂在 suite 顶层 `env` 上会把 case-local 配置伪装成 suite-wide 默认值，语义不对。

## 2. 修改

### 2.1 `run_type_recovery_suite.py` 增加 case 级 env 合并

修改位置：

- `test/run_type_recovery_suite.py:45-49`
- `test/run_type_recovery_suite.py:284-291`

涉及函数：

- `build_case_env`
- `main`

合并顺序现在是：

1. `os.environ`
2. `manifest.env`
3. `case.env`

这样 case 条目可以覆盖 suite 默认 env，而不是反过来。

### 2.2 `fortune` case 把 extra constraints 下沉到 case 条目

修改位置：

- `test/type-recovery/realworld/manifest.json:13-20`

涉及 case：

- `fortune.o3.wasm`

行为：

1. 删除 suite 顶层 `env`
2. 在 `fortune.o3.wasm` 条目下新增 `env.NOTDEC_EXTRA_CONSTRAINTS`

## 3. 验证

```bash
python3 test/run_type_recovery_suite.py \
  --binary ./build/bin/notdec \
  --manifest ./test/type-recovery/realworld/manifest.json \
  --project-root . \
  --workdir /tmp/notdec-runner-realworld-case-env

ctest --test-dir build -R notdec.type_recovery.realworld.tr_level_2 --output-on-failure
```

结果：

- suite runner 通过
- `ctest` 通过
