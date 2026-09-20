#!/usr/bin/env python3
import json, os, re, subprocess, time
ROOT="/sn640/NotDec"
src=open('/tmp/reduce_24259.py').read().split('if __name__')[0]; ns={}; exec(src,ns)
text=open(ns['SRC']).read(); preamble, blocks = ns['parse'](text)
names=[b[0] for b in blocks]; full={n:blk for n,blk,_,_ in blocks}
helpers=["private__0xcc3_0xcc3","private__0xb2f_0xb2f","private__0xdae_0xdae","private__0xcfb_0xcfb",
         "private__0xd1b_0xd1b","private__0xd2f_0xd2f","private__0xaf9_0xaf9","private__0xe54_0xe54",
         "private__0xc2c_0xc2c","private__0xd74_0xd74","private__0xb14_0xb14","private__0xba6_0xba6",
         "private__0xbc9_0xbc9","private__0xbee_0xbee","private__0xb50_0xb50","private__0xcda_0xcda"]
def build(keep,path):
    parts=[preamble]
    for n in names: parts.append(full[n] if n in keep else ns['declare_version'](full[n]))
    open(path,'w').write(''.join(parts))
env=dict(os.environ); env['NOTDEC_EVM_POLY_ENTRIES']='1'; env['NOTDEC_WORKLIST_ABORT_ADDS']='1000000'
os.makedirs('/tmp/onehelper', exist_ok=True)
for h in helpers:
    keep={"public___function_selector___0x0","public__0x4a00cc48_0x74f",h}
    p='/tmp/onehelper/%s.ll'%h; build(keep,p)
    t0=time.time()
    try:
        r=subprocess.run([ns['BIN'],p,'-o','/tmp/onehelper/out.sol','--tr-level=2'],env=env,cwd=ROOT,capture_output=True,timeout=60)
        rc=r.returncode; out=(r.stdout+r.stderr).decode(errors='replace')
    except subprocess.TimeoutExpired as e:
        rc=124; out=((e.stdout or b'')+(e.stderr or b'')).decode(errors='replace')
    m=re.search(r'\[worklist-abort\] add_calls=(\d+) nodes=(\d+) vars=(\d+)', out)
    print(json.dumps(dict(helper=h, rc=rc, sec=round(time.time()-t0,2),
                          nodes=int(m.group(2)) if m else None, vars=int(m.group(3)) if m else None)), flush=True)
