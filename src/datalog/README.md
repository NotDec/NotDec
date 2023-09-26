# NotDec Datalog fact generator for LLVM

This directory contains a Datalog(especially, Souffle) fact extractor for LLVM IR.

## Related Works

- [dwad](https://gitlab.com/lbrande/dwad): A webassembly decompiler based on Datalog(Souffle)
- [Vandal ir](https://github.com/vandaltool/vandalir): LLVM IR bug detector based on Datalog(Souffle). Its exporter is written in Python, based on llvmlite. (Probably due to the lack of API to manipulate IR) Many logic is done by parsing the string representation.
- [cclyzer](https://github.com/plast-lab/cclyzer): Analyzing LLVM IR using Datalog(LogicBox)

## Design Considerations

See https://notdec.github.io/NotDec/datalog.html

### Debug Datalog Rules

```
souffle /sn640/NotDec/src/datalog/rules/pointer_main.dl -F /tmp/pointer_main-b395a4 -D /tmp/pointer_main-b395a4
```
