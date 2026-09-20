#!/usr/bin/env python3
"""Inject a per-entry calldata view call into every public entry of an EVM module."""
import re, sys
src = open(sys.argv[1]).read()
lines = src.splitlines(keepends=True)
out = []
pending_entry = False
seen_label = False
injected = 0
for line in lines:
    m = re.match(r'define\s+.*@(public__[A-Za-z0-9_.$]+)\(', line)
    if m and 'function_selector' not in m.group(1):
        pending_entry = True
        seen_label = False
        out.append(line)
        continue
    if pending_entry:
        if line.startswith('}'):
            pending_entry = False
            out.append(line)
            continue
        if re.match(r'^[A-Za-z0-9_.$]+:', line):
            out.append(line)
            if not seen_label:
                seen_label = True
                out.append('  %notdec.abiview = call ptr @notdec_evm_calldata_view(ptr %calldata)\n')
                injected += 1
            continue
        if seen_label:
            line = re.sub(r'%calldata\b', '%notdec.abiview', line)
        out.append(line)
        continue
    out.append(line)
text = ''.join(out)
text = text.replace('declare void @evm_return(ptr, i256, i256)',
                    'declare void @evm_return(ptr, i256, i256)\ndeclare ptr @notdec_evm_calldata_view(ptr)', 1)
open(sys.argv[2], 'w').write(text)
print('injected into', injected, 'entries')
