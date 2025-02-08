import subprocess
import shlex
import os
from pathlib import Path

# CONSTANTS START
CURRENT_PROJECT_DIR = Path(os.getcwd()).as_posix() 
OUTPUT_DIR = CURRENT_PROJECT_DIR + "/build"

print("Current Working Directory -> " + CURRENT_PROJECT_DIR)
print("Output Directory -> " + OUTPUT_DIR)

# CONSTANTS END

def run_cmd(cmd):
    subprocess.run(shlex.split(cmd), stdout=None, stderr=None, text=True)


print("----------- Starting Python Script --------------")
cmd = f'cmake -G \"MinGW Makefiles\" -DCMAKE_C_COMPILER=clang.exe -DCMAKE_CXX_COMPILER=clang++.exe -S {CURRENT_PROJECT_DIR} -B {OUTPUT_DIR}'
run_cmd(cmd)
print("CMake Completed!")
cmd = f'make -C {OUTPUT_DIR}'
run_cmd(cmd)
print("---------- Python Build Script Ended ------------")
