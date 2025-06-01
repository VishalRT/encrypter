import subprocess
import shlex
import os
import sys
import shutil
from pathlib import Path
from enum import Enum
import argparse

# CONSTANTS START
CURRENT_PROJECT_DIR = Path(os.getcwd()).as_posix() 
OUTPUT_DIR = CURRENT_PROJECT_DIR + "/build"

print("Current Working Directory -> " + CURRENT_PROJECT_DIR)
print("Output Directory -> " + OUTPUT_DIR)

# CONSTANTS END

class BuildType(Enum):
    DEBUG = 0
    RELEASE = 1
    RELWITHDEBINFO = 2

    def to_cmake(self):
        if self == BuildType.DEBUG:
            return "Debug"
        elif self == BuildType.RELEASE:
            return "Release"
        elif self == BuildType.RELWITHDEBINFO:
            return "RelWithDebInfo"
        else:
            return "Release"

class BuildAction(Enum):
    BUILD = "build"
    CLEAN_BUILD = "clean-build"
    FRESH = "fresh"

def run_cmd(cmd):
    subprocess.run(shlex.split(cmd), stdout=None, stderr=None, text=True)

def clean():
    print("----------- Performing CMake Clean --------------")
    cmd=f'cmake --build {OUTPUT_DIR} --target clean' 
    # cmd=f'cmake --build {OUTPUT_DIR} --target clean VERBOSE=1'
    run_cmd(cmd)
    print("----------- CMake Clean Done -------------")

def fresh():
    print("----------- Cleaning Build Directory --------------")

    if os.path.exists(OUTPUT_DIR):
        shutil.rmtree(OUTPUT_DIR)
        os.mkdir(OUTPUT_DIR)
    else:
        print("Build Directory Already Clean!")


def build(build_type):
    build_type_str = build_type.to_cmake() if isinstance(build_type, BuildType) else str(build_type)
    print(f"----------- Starting Build [{build_type_str}] --------------")
    cmd = f'cmake -G "MinGW Makefiles"  \
            -DCMAKE_C_COMPILER=clang.exe -DCMAKE_CXX_COMPILER=clang++.exe \
            -DCMAKE_BUILD_TYPE={build_type_str} \
            -S {CURRENT_PROJECT_DIR} -B {OUTPUT_DIR}'
    # Commenting Verbose command, enable based on options later
    # cmd = f'cmake -G \"MinGW Makefiles\" --trace-expand  \
    #         -DCMAKE_C_COMPILER=clang.exe -DCMAKE_CXX_COMPILER=clang++.exe \
    #         -S {CURRENT_PROJECT_DIR} -B {OUTPUT_DIR}'
    # For Ninja Build System in case required in future
    # cmake -G "Ninja Multi-Config" -DCMAKE_C_COMPILER=clang.exe -DCMAKE_CXX_COMPILER=clang++.exe -S . -B build/
    run_cmd(cmd)
    print("CMake Completed, Build Files Generated!")

    # cmd = f'make -C {OUTPUT_DIR}'
    #Alias to the above
    cmd = f'cmake --build {OUTPUT_DIR}'
    # cmd = f'cmake --build {OUTPUT_DIR} --verbose'
    run_cmd(cmd)
    print("----------- Build Finished --------------")

def help():
    help_text = '''\
--------------- Python CMake Build Helper ---------------
Usage:
  python build.py [--build-action {build,clean-build,fresh}] [--build-type {debug,release,relwithdebinfo}]

Options:
  --build-action    Build action to perform:
                     - build        : Do only a build (incremental)
                     - clean-build  : CMake clean then build
                     - fresh        : Clean the OUTPUT_DIR and then do a build
  --build-type      CMake build type:
                     - debug        : Debug build (default: release)
                     - release      : Release build
                     - relwithdebinfo : Release with debug info

Examples:
  python build.py --build-action build --build-type debug
  python build.py --build-action clean-build --build-type release
  python build.py --build-action fresh --build-type relwithdebinfo

--------------- Python CMake Build Helper ---------------
'''
    print(help_text)


def main():
    parser = argparse.ArgumentParser(description="CMake Python Build Helper", add_help=False)
    parser.add_argument(
        "--build-action",
        type=lambda s: BuildAction(s.lower()),
        choices=list(BuildAction),
        default=BuildAction.CLEAN_BUILD,
        help="Build action: build (incremental), clean-build (cmake clean + build), fresh (delete build dir + build)"
    )
    parser.add_argument(
        "--build-type",
        type=lambda s: BuildType[s.upper()],
        choices=list(BuildType),
        default=BuildType.RELEASE,
        help="CMake build type: debug, release, relwithdebinfo (default: release)"
    )
    parser.add_argument("-h", "--help", action="store_true", help="Show this help message and exit.")
    
    try:
        args, unknown = parser.parse_known_args()
        if args.help or unknown:
            help()
            sys.exit(1)
    except Exception as e:
        print(f"Error: {e}\n")
        help()
        sys.exit(1)

    print("----------- Starting Python Script --------------")
    if args.build_action == BuildAction.BUILD:
        build(args.build_type)
    elif args.build_action == BuildAction.CLEAN_BUILD:
        clean()
        build(args.build_type)
    elif args.build_action == BuildAction.FRESH:
        fresh()
        build(args.build_type)
    print("---------- Python Script Ended ------------")

if(__name__ == "__main__"):
    main()
