import glob
import subprocess
import sys
import os.path

optimize = True
cpp_args = ['-Wall', '-Wextra', '-Wno-mismatched-tags', '-std=c++17']
if optimize:
    cpp_args.append('-O')
cpp_args = tuple(cpp_args)
cpp_name = 'clang'
ld_args = '-static',
exe_file = 'mal_repl.exe'
source = 'src/*.cpp'
src_path = 'src/'
out_path = 'out/'

obj_files = []

def comp_source(src_file, out_file):
    print('Compiling file {} ---> {}'.format(src_file, out_file))
    proc = subprocess.run((cpp_name,) + cpp_args + (src_file, '-c', '-o', out_file))
    if proc.returncode != 0:
        print('Compilation failed')
        sys.exit(1)
    print('====================')

comp = True
if len(sys.argv) == 2:
    comp = False
    out_file = sys.argv[1]
    if out_file != exe_file:
        src_file = src_path + os.path.basename(out_file).split('.')[0] + '.cpp'
        comp_source(src_file, out_file)

for src_file in glob.iglob(source):
    out_file = out_path + os.path.basename(src_file).split('.')[0] + '.o'
    obj_files.append(out_file)
    if comp:
        comp_source(src_file, out_file)

subprocess.run((cpp_name, '-o', exe_file) + ld_args + tuple(obj_files))
print('Compilation completed')