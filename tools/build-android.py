#!/usr/bin/env python3
"""Build AY Android read-only preflight (runtime host tests excluded) for Android ABIs using a locally installed Android NDK."""
from pathlib import Path
import argparse
import os
import shutil
import subprocess
import sys

root=Path(__file__).resolve().parent.parent
parser=argparse.ArgumentParser(description='Build Android AY ELF scanner binaries using the Android NDK')
parser.add_argument('--abi', choices=['arm64-v8a','armeabi-v7a','x86','x86_64','riscv64','all'], default='all')
parser.add_argument('--ndk', default=os.environ.get('ANDROID_NDK_HOME') or os.environ.get('ANDROID_NDK_ROOT'))
parser.add_argument('--api', type=int, default=26)
args=parser.parse_args()
if not args.ndk:
    parser.error('Android NDK missing. Supply --ndk /path/to/android-ndk or ANDROID_NDK_HOME')
ndk=Path(args.ndk).expanduser().resolve()
toolchain=ndk/'build/cmake/android.toolchain.cmake'
if not toolchain.is_file():
    parser.error(f'Could not find official NDK CMake toolchain: {toolchain}')
abis=['arm64-v8a','armeabi-v7a','x86','x86_64'] if args.abi=='all' else [args.abi]
for abi in abis:
    print(f'\n[AY] Building {abi} for Android API {args.api}', flush=True)
    out=root/'native'/'out'/abi
    command=['cmake','-S',str(root/'native'),'-B',str(out),'-G','Ninja',
             f'-DCMAKE_TOOLCHAIN_FILE={toolchain}',f'-DANDROID_ABI={abi}',
             f'-DANDROID_PLATFORM=android-{args.api}',
             '-DANDROID_STL=c++_static','-DCMAKE_BUILD_TYPE=Release']
    subprocess.run(command,check=True)
    subprocess.run(['cmake','--build',str(out),'--parallel','2'],check=True)
    built=out/'ay-native'
    if not built.is_file():
        raise RuntimeError(f'Compiler did not produce {built}')
    staged=root/'bin'/abi/'ay-native'
    staged.parent.mkdir(parents=True,exist_ok=True)
    shutil.copyfile(built,staged)
    staged.chmod(0o755)
    print(f'[AY] Staged {staged.relative_to(root)} ({staged.stat().st_size:,} bytes)')
print('\n[AY] Next: python3 tools/build.py to package the installers')
