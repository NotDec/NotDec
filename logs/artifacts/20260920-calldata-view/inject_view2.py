#!/usr/bin/env python3
import re, sys
src = open(sys.argv[1]).read()
lines = src.splitlines(keepends=True)
out = []
pending = False; seen = False; injected = 0
CAL = re.compile(r'@(evm_calldataload|evm_calldatacopy|evm_calldatasize|notdec_evm_calldata_min_size)\b')
for line in lines:
    m = re.match(r'define\s+.*@(public__[A-Za-z0-9_.$]+)\(', line)
    if m and 'function_selector' not in m.group(1):
        pending = True; seen = False; out.append(line); continue
    if pending:
        if line.startswith('}'):
            pending = False; out.append(line); continue
        if re.match(r'^[A-Za-z0-9_.$]+:', line):
            out.append(line)
            if not seen:
                seen = True
                out.append('  %notdec.abiview = call ptr @notdec_evm_calldata_view(ptr %calldata)\n')
                injected += 1
            continue
        if seen and CAL.search(line):
            line = re.sub(r'%calldata\b', '%notdec.abiview', line)
        out.append(line); continue
    out.append(line)
text = ''.join(out)
text = text.replace('declare void @evm_return(ptr, i256, i256)',
                    'declare void @evm_return(ptr, i256, i256)\ndeclare ptr @notdec_evm_calldata_view(ptr)', 1)
open(sys.argv[2], 'w').write(text)
print('injected into', injected, 'entries (only evm_calldata* operands)')
