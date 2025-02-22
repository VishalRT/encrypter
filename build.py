import subprocess
import shlex
import os
import sys
import shutil
from pathlib import Path

# CONSTANTS START
CURRENT_PROJECT_DIR = Path(os.getcwd()).as_posix() 
OUTPUT_DIR = CURRENT_PROJECT_DIR + "/build"

print("Current Working Directory -> " + CURRENT_PROJECT_DIR)
print("Output Directory -> " + OUTPUT_DIR)

# CONSTANTS END

def run_cmd(cmd):
    subprocess.run(shlex.split(cmd), stdout=None, stderr=None, text=True)

def clean():
    print("----------- Performing CMake Clean --------------")
    cmd=f'cmake --build {OUTPUT_DIR} --target clean'
    run_cmd(cmd)
    print("----------- CMake Clean Done -------------")

def fresh():
    print("----------- Cleaning Build Directory --------------")

    if os.path.exists(OUTPUT_DIR):
        shutil.rmtree(OUTPUT_DIR)
        os.mkdir(OUTPUT_DIR)
    else:
        print("Build Directory Already Clean!")


def build():
    print("----------- Starting Build --------------")

    cmd = f'cmake -G \"MinGW Makefiles\" --trace-expand  \
            -DCMAKE_C_COMPILER=clang.exe -DCMAKE_CXX_COMPILER=clang++.exe \
            -S {CURRENT_PROJECT_DIR} -B {OUTPUT_DIR}'
    # For Ninja Build System in case required in future
    # cmake -G "Ninja Multi-Config" -DCMAKE_C_COMPILER=clang.exe -DCMAKE_CXX_COMPILER=clang++.exe -S . -B build/
    run_cmd(cmd)
    print("CMake Completed, Build Files Generated!")

    # cmd = f'make -C {OUTPUT_DIR}'
    #Alias to the above
    cmd = f'cmake --build {OUTPUT_DIR} --verbose'
    run_cmd(cmd)
    print("----------- Build Finished --------------")

def main():
    print("----------- Starting Python Script --------------")

    if(sys.argv[1] == "build"):
        build()
    elif(sys.argv[1] == "clean-build"):
        clean()
        build()
    elif(sys.argv[1] == "fresh"):
        fresh()
    else:
        print("Invalid parameters")

    print("---------- Python Script Ended ------------")

if(__name__ == "__main__"):
    main()
