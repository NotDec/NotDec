# EVM Solidity Source Regression Suite

This suite is for Solidity examples that have been manually checked after a
fix.  Do not bulk-copy Solidity compiler tests here.  Add one small case only
after the NotDec Solidity output is good enough to keep as a regression oracle.

Default CTest runs the stable part of the chain:

```bash
ctest --test-dir build -R notdec.evm.solidity_source --output-on-failure
```

Apehex source-backed candidates should be picked from smaller runtime bytecode
to larger contracts:

```bash
python3 scripts/apehex-solidity-source-candidates.py \
  --limit 30 \
  --output /tmp/apehex-solidity-candidates.csv \
  --export-dir /tmp/apehex-solidity-candidates
```

The script defaults to `/sn640/NotDecChainExp/apehex_evm_contracts`, reads
`hex/ethereum/cleaned`, keeps Solidity-looking sources, deduplicates runtime
bytecode, and sorts by `runtime_bytes`.  The exported files are only a staging
area.  Promote a case here after its IR and NotDec output have been checked.

Each case keeps the source-level evidence and the frozen input separately:

- `cases/`: small Solidity source used to explain the case.
- `bytecode/`: optional runtime bytecode used to generate the frozen IR.
- `ir/`: frozen evm2llvm LLVM IR consumed by the automated test.
- `expected/`: expected NotDec Solidity-like output.

The automated test does not invoke `solc` or Gigahorse.  Those tools are outside
the default NotDec build and can make the suite depend on local versions.  When a
case needs to be refreshed, regenerate `ir/*.ll` manually, inspect the NotDec
output, then update `expected/*.sol`.

For a bytecode-backed case, the manual refresh shape is:

```bash
python3 external/NotDec-evm2llvm/scripts/notdec-evm2llvm.py \
  test/evm/solidity-source/bytecode/<case>.hex \
  -o test/evm/solidity-source/ir/<case>.ll \
  --gigahorse-dir /sn640/gigahorse-toolchain \
  --evm2llvm external/NotDec-evm2llvm/build/bin/evm2llvm \
  --work-dir /tmp/notdec-evm2llvm-<case>

./build/bin/notdec test/evm/solidity-source/ir/<case>.ll \
  -o /tmp/<case>.sol --tr-level=2
```
