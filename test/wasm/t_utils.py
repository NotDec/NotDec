
import os,sys

CC='clang-14'

RED='\033[0;34m'
NC='\033[0m' # No Color

def get_decompile_commands(wasm_path, out_path):
    cwd = os.path.dirname(os.path.realpath(__file__))
    return [f'{cwd}/../../build/bin/notdec', wasm_path, '-o', out_path, "--disable-all-pass", "--fix-names"]

def get_run_commands(ir_path):
    # cwd = os.path.dirname(os.path.realpath(__file__))
    # return ['lli-13', ir_path, os.path.join(cwd, 'sylib.ll')]
    return [ir_path+'.elf']

def do_compile(ir_path, other_libs=[]):
    # cwd = os.path.dirname(os.path.realpath(__file__))
    command = [CC, ir_path, *other_libs, '-g', '-o', ir_path+'.elf']
    print(' '.join(command))
    return os.system(' '.join(command)) == 0

def do_decompile(wasm_path, out_path):
    command = get_decompile_commands(wasm_path, out_path)
    print(' '.join(command))
    return os.system(' '.join(command)) == 0

def test_run(target, out_file, in_file=None, add_returncode=False):
    in_str = b""
    if (in_file != None):
        with open(in_file, 'rb') as f:
            in_str = f.read()

    from subprocess import Popen,PIPE,STDOUT

    commands = get_run_commands(target)
    # print(' '.join(commands))

    p = Popen(' '.join(commands), stdout=PIPE,  stdin=PIPE, stderr=PIPE, shell=True)
    out, err = p.communicate(input=in_str)
    if add_returncode:
        code = p.returncode
        if len(out) > 0 and out[-1] != ord('\n'):
            out = out + b'\n'
        out = out + str(code).encode()

    with open(out_file, 'rb') as f:
        s = f.read()

    print('result: ',out)
    print('excepted: ',s)
    if out.replace(b'\r\n', b'\n').strip() == s.replace(b'\r\n', b'\n').strip():
        print(RED+"=========== Pass! ==============" +NC)
        return True
    s = s.rsplit(b"\n", 1)
    if len(s) > 1:
        s[0] = s[0].strip()
    s = b"\n".join(s)

    print("stderr: ", err, file=sys.stderr) # perfomance test打印所花时间
    if out.strip() == s.strip():
        print(RED+"=========== Pass! ==============" +NC)
        return True
    else:
        print(RED+"Result Mismatch"+NC)
        return False
