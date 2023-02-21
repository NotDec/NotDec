import unittest
import os,sys,subprocess

RED='\033[0;34m'
NC='\033[0m' # No Color

def get_decompile_commands(wasm_path, out_path):
    cwd = os.path.dirname(os.path.realpath(__file__))
    return [f'{cwd}/../../../build/bin/notdec', '-i', wasm_path, '-o', out_path, "--test-mode", "--recompile"]

def get_run_commands(ir_path):
    cwd = os.path.dirname(os.path.realpath(__file__))
    return ['lli-13', ir_path, os.path.join(cwd, 'sylib.ll')]

def do_decompile(wasm_path, out_path):
    command = get_decompile_commands(wasm_path, out_path)
    return os.system(' '.join(command)) == 0

def test_run(target, out_file, in_file=None):
    in_str = b""
    if (in_file != None):
        with open(in_file, 'rb') as f:
            in_str = f.read()

    from subprocess import Popen,PIPE,STDOUT

    commands = get_run_commands(target)
    print(' '.join(commands))

    p = Popen(commands, stdout=PIPE,  stdin=PIPE, stderr=PIPE)
    out, err = p.communicate(input=in_str)
    code = p.returncode
    if len(out) > 0 and out[-1] != ord('\n'):
        out = out + b'\n'
    out = out + str(code).encode()

    with open(out_file, 'rb') as f:
        s = f.read()

    print(out)
    print(s)
    if out.strip() == s.strip():
        print(RED+"=========== Pass! ==============" +NC)
        return True
    s = s.rsplit(b"\n", 1)
    if len(s) > 1:
        s[0] = s[0].strip()
    s = b"\n".join(s)

    print(err, file=sys.stderr) # perfomance test打印所花时间
    if out.strip() == s.strip():
        print(RED+"=========== Pass! ==============" +NC)
        return True
    else:
        print(RED+"Result Mismatch"+NC)
        return False

class DefaultTestCase(unittest.TestCase):
    def test_functional(self):
        cwd = os.path.dirname(os.path.realpath(__file__))
        dir = os.path.join(cwd, "functional")
        outdir = os.path.join(cwd, "out_functional")
        for file in sorted(os.listdir(dir)):
            if not file.endswith(".c"):
                continue
            file_no_suffix = os.path.join(dir, file.removesuffix(".c"))
            wasm = os.path.join(outdir, f'{file}.wasm')
            ir = os.path.join(outdir, f'{file}.ll')

            print(RED+f'=========== decompiling {file} =========='+NC)
            self.assertTrue(do_decompile(wasm, ir), "decompilation error")

            print(RED+f'=========== running {file} =========='+NC)
            in_file = file_no_suffix+".in"
            if not os.path.exists(file_no_suffix+".in"):
                in_file = None
            ret = test_run(ir, file_no_suffix+".out", in_file)
            self.assertTrue(ret)

if __name__ == '__main__':
    import unittest
    unittest.main()