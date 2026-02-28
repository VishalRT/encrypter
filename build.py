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
    REBUILD = "rebuild"
    CLEAN = "clean"
    FRESH = "fresh"

def run_cmd(cmd):
    subprocess.run(shlex.split(cmd), stdout=None, stderr=None, text=True)

def clean():
    print("----------- Performing CMake Clean --------------")
    cmd=f'cmake --build {OUTPUT_DIR} --target clean' 
    # cmd=f'cmake --build {OUTPUT_DIR} --target clean VERBOSE=1'
    run_cmd(cmd)
    print("----------- CMake Clean Done --------------")

def build(build_type, args):
    print("----------- Starting CmakeConfig Generation --------------")
    build_type_str = build_type.to_cmake() if isinstance(build_type, BuildType) else str(build_type)
    
    config_cmd = f'cmake -G "MinGW Makefiles" \
            -DCMAKE_BUILD_TYPE={build_type_str} \
            -S {CURRENT_PROJECT_DIR} -B {OUTPUT_DIR}'
    
    # Commenting Verbose command, enable based on options later
    # cmd = f'cmake -G \"MinGW Makefiles\" --trace-expand  \
    #         -DCMAKE_C_COMPILER=clang.exe -DCMAKE_CXX_COMPILER=clang++.exe \
    #         -S {CURRENT_PROJECT_DIR} -B {OUTPUT_DIR}'
    # For Ninja Build System in case required in future
    # cmake -G "Ninja Multi-Config" -DCMAKE_C_COMPILER=clang.exe -DCMAKE_CXX_COMPILER=clang++.exe -S . -B build/
    
    # Always export compile commands for clangd IntelliSense
    config_cmd += ' -DCMAKE_EXPORT_COMPILE_COMMANDS:BOOL=ON'

    # clean cmake cache from build diretory
    if hasattr(args, 'fresh') and args.fresh:
        config_cmd += ' --fresh'

    # Enable clang_tidy option in CMake File
    if hasattr(args, 'clang_tidy_check') and args.clang_tidy_check:
        config_cmd += ' -DENABLE_CLANG_TIDY_CHECKS:BOOL=ON'
    
    # Run CMake configuration first
    run_cmd(config_cmd)
    print("CMake Configuration Completed, Build Files Generated!")
    print("----------- CMake Config Finished --------------")

    print("----------- Starting Build --------------")
    # Then build project
    build_cmd = f'cmake --build {OUTPUT_DIR}'

    # Run clang-tidy if --clang-tidy-check is passed as argument
    if hasattr(args, 'clang_tidy_check') and args.clang_tidy_check:
        build_cmd += ' --target clang-tidy-check'
    
    # Add clean-first if fresh build
    if hasattr(args, 'fresh') and args.fresh:
        build_cmd += ' --clean-first'

    run_cmd(build_cmd)
    print("----------- Build Finished --------------")
    
    
    

def help():
    help_text = '''\
--------------- Python CMake Build Helper --------------
Usage:
  python build.py [--build-action {rebuild,clean,fresh}] [--build-type {debug,release,relwithdebinfo}] [--clang-tidy-check]

Options:
  --build-action    Build action to perform:
                     - rebuild          : Normal incremental build
                     - clean            : Clean build targets then rebuild
                     - fresh            : Complete build directory cleanup with fresh build

  --build-type      CMake build type    :
                     - debug            : Debug build (default: release)
                     - release          : Release build
                     - relwithdebinfo   : Release with debug info

  --clang-tidy-check Run clang-tidy checks during build.

Examples:
  python build.py --build-action rebuild --build-type debug
  python build.py --build-action clean --build-type release
  python build.py --build-action fresh --build-type relwithdebinfo
  python build.py --build-action fresh --clang-tidy-check

--------------- Python CMake Build Helper --------------
'''
    print(help_text)


def main():
    parser = argparse.ArgumentParser(description="CMake Python Build Helper", add_help=False)
    parser.add_argument(
        "--build-action",
        type=lambda s: BuildAction(s.lower()),
        choices=list(BuildAction),
        default=BuildAction.CLEAN,
        help="Build action: rebuild (incremental), clean (cmake clean + build), fresh (cleanup & build)"
    )
    parser.add_argument(
        "--build-type",
        type=lambda s: BuildType[s.upper()],
        choices=list(BuildType),
        default=BuildType.RELEASE,
        help="CMake build type: debug, release, relwithdebinfo (default: release)"
    )
    parser.add_argument("--clang-tidy-check", action="store_true", help="Run clang-tidy checks.")
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
    if args.build_action == BuildAction.REBUILD:
        build(args.build_type, args)
    elif args.build_action == BuildAction.CLEAN:
        clean()
        build(args.build_type, args)
    elif args.build_action == BuildAction.FRESH:
        args.fresh = True  # Add fresh attribute to args
        build(args.build_type, args)

    print("---------- Python Script Ended --------------")

if(__name__ == "__main__"):
    main()
