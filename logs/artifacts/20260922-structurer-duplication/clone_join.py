#!/usr/bin/env python3
"""Clone a shared join block for one of its predecessors, to test whether the
structurer's goto fallback is triggered by the shared tail (the duplicating
direction of angr's ISD family) rather than by duplicated code."""
import re, sys

src, func_key, join_label, pred_label = sys.argv[1], sys.argv[2], sys.argv[3], sys.argv[4]
text = open(src, errors='replace').read()

# locate function
m = re.search(r'^define [^\n]*@' + re.escape(func_key) + r'\(.*?^\}', text, re.S | re.M)
assert m, 'function not found'
body = m.group(0)

# split into blocks
lines = body.split('\n')
head_end = 0
for i, l in enumerate(lines):
    if re.match(r'^[-a-zA-Z$._0-9]+:', l):
        head_end = i; break
head = '\n'.join(lines[:head_end])
blocks = []
cur = None
for l in lines[head_end:]:
    if re.match(r'^[-a-zA-Z$._0-9]+:', l):
        if cur: blocks.append(cur)
        cur = [l]
    elif cur is not None:
        cur.append(l)
if cur: blocks.append(cur)
# the closing brace of the function is appended to the last block; peel it off
closing = []
if blocks and blocks[-1][-1].strip() == '}':
    closing = [blocks[-1].pop()]
    if not blocks[-1][1:]:
        blocks.pop()

def label_of(b): return b[0].split(':')[0]
idx = {label_of(b): i for i, b in enumerate(blocks)}
assert join_label in idx and pred_label in idx, (list(idx), join_label, pred_label)

# clone the join block with renamed SSA defs
join = blocks[idx[join_label]]
new_label = join_label + 'b'
defs = set(re.findall(r'^\s*(%[-a-zA-Z$._0-9]+)\s*=', '\n'.join(join), re.M))
clone = []
for l in join:
    l = l.replace(join_label + ':', new_label + ':', 1)
    for d in defs:
        pat = r'(?<![A-Za-z0-9_.])' + re.escape(d) + r'(?![A-Za-z0-9_.])'
        repl = '%v' + d[1:] + 'b' if d[1:].isdigit() else d + 'b'
        l = re.sub(pat, repl, l)
    clone.append(l)
clone = [l for l in clone]
# fix the preds comment
clone = [re.sub(r'; preds = .*$', '; preds = %' + pred_label, l) if l.startswith(new_label + ':') else l for l in clone]

# rewire the predecessor terminator
pb = blocks[idx[pred_label]]
for i, l in enumerate(pb):
    if re.match(r'^\s*br\s+label\s+%' + re.escape(join_label) + r'\s*$', l.strip()):
        pb[i] = l.replace('%' + join_label, '%' + new_label)
assert any(new_label in l for l in pb), 'rewire failed'

new_blocks = blocks + [clone]
new_body = head + '\n' + '\n'.join('\n'.join(b) for b in new_blocks) + '\n' + '\n'.join(closing)
open(src.replace('.ll', '.' + sys.argv[5] + '.ll'), 'w').write(text[:m.start()] + new_body + text[m.end():])
print('wrote', src.replace('.ll', '.' + sys.argv[5] + '.ll'))
