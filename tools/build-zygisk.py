#!/usr/bin/env python3
"""Cross-compile original AY Zygisk API-v5 smoke-test bridge with the official Android NDK."""
import argparse
import os
from pathlib import Path
import shutil
import subprocess

root=Path(__file__).resolve().parent.parent
parser=argparse.ArgumentParser(description='Android NDK build: optional AY Zygisk API-v5 smoke-test plugin')
parser.add_argument('--ndk',default=os.environ.get('ANDROID_NDK_HOME') or os.environ.get('ANDROID_NDK_ROOT'))
parser.add_argument('--abi',choices=['arm64-v8a','armeabi-v7a','x86','x86_64','all'],default='all')
parser.add_argument('--api',type=int,default=26)
args=parser.parse_args()
if not args.ndk: parser.error('NDK path missing: use --ndk /path/to/android-ndk')
ndk=Path(args.ndk).expanduser().resolve()
toolchain=ndk/'build/cmake/android.toolchain.cmake'
if not toolchain.exists(): parser.error('Invalid Android NDK: android.toolchain.cmake missing')
header=root/'native/zygisk/vendor/zygisk.hpp'
if not header.exists(): parser.error('Official zygisk.hpp not found; see native/zygisk/README.md')
if '#define ZYGISK_API_VERSION 5' not in header.read_text(errors='replace'):
    parser.error('Expected official Zygisk API v5 header')
abis=['arm64-v8a','armeabi-v7a','x86','x86_64'] if args.abi=='all' else [args.abi]
for abi in abis:
    build=root/'native/zygisk/out'/abi
    cmd=['cmake','-S',str(root/'native/zygisk'),'-B',str(build),'-G','Ninja',
         f'-DCMAKE_TOOLCHAIN_FILE={toolchain}',f'-DANDROID_ABI={abi}',f'-DANDROID_PLATFORM=android-{args.api}',
         '-DANDROID_STL=c++_static','-DCMAKE_BUILD_TYPE=Release']
    subprocess.run(cmd,check=True)
    subprocess.run(['cmake','--build',str(build),'--parallel','2'],check=True)
    artifact=build/'libayzygisk.so'
    if not artifact.is_file(): raise RuntimeError(f'Missing Android output: {artifact}')
    staged=root/'zygisk'/f'{abi}.so'
    staged.parent.mkdir(parents=True,exist_ok=True)
    shutil.copyfile(artifact,staged)
    print(f'AY Zygisk staged {staged.relative_to(root)} ({staged.stat().st_size:,} bytes)')
print('NEXT: python3 tools/build.py --with-zygisk (explicit opt-in)')
