#!/bin/bash
set -u
cd /sn640/NotDec
BIN=./build-notdec-nothreads/bin/notdec
for alg in sailr structured-phoenix structured-goto; do
  rm -f /tmp/ab_${alg}_*.sol
  for c in 0118 0637 0657 2002 0029; do
    f=$(ls test/evm/solidity-patterns/cases/${c}_*.ll)
    NOTDEC_STRUCTURER=$alg $BIN "$f" -o /tmp/ab_${alg}_${c}.sol --tr-level=2 >/dev/null 2>&1
  done
  python3 -c "
import glob
g=t=0
for p in glob.glob('/tmp/ab_${alg}_*.sol'):
    s=open(p,errors='replace').read(); g+=s.count('// goto block_'); t+=s.count('false /* TODO')
print('  ${alg}: total_goto=%d cond_todo=%d files=%d'%(g,t,len(glob.glob('/tmp/ab_${alg}_*.sol'))))
"
done