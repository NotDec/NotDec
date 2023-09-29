# NotDec Datalog fact generator for LLVM

This directory contains a Datalog(especially, Souffle) fact extractor for LLVM IR.

## Related Works

- [dwad](https://gitlab.com/lbrande/dwad): A webassembly decompiler based on Datalog(Souffle)
- [Vandal ir](https://github.com/vandaltool/vandalir): LLVM IR bug detector based on Datalog(Souffle). Its exporter is written in Python, based on llvmlite. (Probably due to the lack of API to manipulate IR) Many logic is done by parsing the string representation.
- [cclyzer](https://github.com/plast-lab/cclyzer): Analyzing LLVM IR using Datalog(LogicBox)

## Design Considerations & Architecture

See https://notdec.github.io/NotDec/datalog.html

Custom code generation:
- Use `gen_header.py` to extract fact names for use in code. (automatically done by cmake)
- Use souffle to compile datalog rules to generate pointer_main.cpp

To unify return instruction and instruction, create abstract vale for each return value, like `llvm::Argument*`. An abstract value is a constant string attached to a llvm value.

### Debug Datalog Rules

```
souffle /sn640/NotDec/src/datalog/rules/pointer_main.dl -F /tmp/pointer_main-b395a4 -D /tmp/pointer_main-b395a4
```
