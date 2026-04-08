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
        dir = os.path.join(cwd, "filtered")
        outdir = os.path.join(cwd, "out_ll")
        if not os.path.exists(outdir):
            os.makedirs(outdir)
        for file in sorted(os.listdir(dir)):
            if not file.endswith(".wasm"):
                continue
            file_no_suffix = file.removesuffix(".wasm")
            wasm = os.path.join(dir, f'{file_no_suffix}.wasm')
            ir = os.path.join(outdir, f'{file_no_suffix}.ll')
            print(RED+f'=========== lifting {file} =========='+NC)
            ret = do_decompile(wasm, ir)
            if(not ret):
                print(f"{file} lift error")
            break


if __name__ == '__main__':
    import unittest
    unittest.main()
