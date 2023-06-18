#!/usr/bin/python3
import unittest
import os,sys,subprocess


file_path = os.path.realpath(__file__)
module_path = file_path[:file_path.rfind('wasm')+4]
sys.path.insert(1, module_path)

from t_utils import *

class DefaultTestCase(unittest.TestCase):
    def test_functional(self):
        cwd = os.path.dirname(os.path.realpath(__file__))
        dir = os.path.join(cwd, "functional")
        outdir = os.path.join(cwd, "out_functional")
        os.makedirs(outdir, exist_ok=True)
        for file in sorted(os.listdir(dir)):
            if not file.endswith(".c"):
                continue
            file_no_suffix = os.path.join(dir, file.removesuffix(".c"))
            wasm = os.path.join(outdir, f'{file}.wasm')
            ir = os.path.join(outdir, f'{file}.ll')

            print(RED+f'=========== decompiling {file} =========='+NC)
            self.assertTrue(do_decompile(wasm, ir), "decompilation error")

            print(RED+f'=========== running {file} =========='+NC)
            self.assertTrue(do_compile(ir, [os.path.join(cwd, 'sylib.ll')]), "recompilation error")
            in_file = file_no_suffix+".in"
            if not os.path.exists(file_no_suffix+".in"):
                in_file = None
            ret = test_run(ir, file_no_suffix+".out", in_file, add_returncode=True)
            self.assertTrue(ret)

if __name__ == '__main__':
    import unittest
    unittest.main()
