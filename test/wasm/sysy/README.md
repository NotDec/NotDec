# sysY test cases
The Python script is used to test the decompilation process:
- Requirements: use 

1. the `compiles.sh` compiles sources in `functional` to wasm and wat files in out_functional
2. 

Other scripts

- `add_include.sh`: this script add `#include "sylib.h"` to the first line of the sy test cases and rename it to `.c`. Used initially to introduce the test cases.
- `clean.sh` remove the `out_*` folders.

## Logs

**2024-04-18**

54_hidden_var.c等测试用例出现问题：

libc定义了memset，然后我里面调用了llvm memset intrinsic，然后intrinsic调用了反编译文件里的memset（这个memset甚至还是internal的）。关键在intrinsic的实现上不能引用现有的memset。解决方法：在`compat_mode`时，重命名main函数的逻辑那里，额外增加一下重命名memcpy，memset的逻辑。
