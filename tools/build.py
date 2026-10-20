#!/usr/bin/env python3
from pathlib import Path
from zipfile import ZipFile, ZIP_DEFLATED, ZipInfo
import hashlib
import argparse

root=Path(__file__).resolve().parent.parent
out=root.parent/'AY-Zygisk-V2-Controller-2.0.0.zip'
parser=argparse.ArgumentParser()
parser.add_argument('--with-zygisk',action='store_true',help='OPT-IN: include ALL four verified Android Zygisk bridge libraries')
args=parser.parse_args()
entries=['module.prop','customize.sh','action.sh','service.sh','uninstall.sh','skip_mount',
         'integrity.sha256',
         'README.md','CHANGELOG.md','config/settings.conf','scripts/ayctl.sh',
         'webroot/index.html','webroot/assets/app.css','webroot/assets/app.js','webroot/assets/mark.svg']
# Only include real Android NDK builds when explicitly staged in bin/<ABI>.
for abi in ('arm64-v8a','armeabi-v7a','x86','x86_64','riscv64'):
    staged = f'bin/{abi}/ay-native'
    if (root/staged).is_file():
        data=(root/staged).read_bytes()
        if data[:4] != b'\x7fELF' or len(data)<64:
            raise SystemExit(f'Expected Android ELF in {staged}')
        expected = {'arm64-v8a':(2,183),'armeabi-v7a':(1,40),'x86':(1,3),
                    'x86_64':(2,62),'riscv64':(2,243)}[abi]
        cls, machine = data[4], int.from_bytes(data[18:20],'little')
        if data[5]!=1 or (cls,machine)!=expected:
            raise SystemExit(f'Wrong CPU or ELF format for {staged}')
        # Reject dynamically linked glibc host executables accidentally copied into bin/.
        # This is not a full signature/NDK provenance check.
        phoff=int.from_bytes(data[32:36] if cls==1 else data[32:40],'little')
        phentsize=int.from_bytes(data[42:44] if cls==1 else data[54:56],'little')
        phnum=int.from_bytes(data[44:46] if cls==1 else data[56:58],'little')
        if phnum>4096 or phentsize<32 or phoff+phentsize*phnum>len(data):
            raise SystemExit(f'Invalid program headers for {staged}')
        for idx in range(phnum):
            hdr=phoff+idx*phentsize
            if int.from_bytes(data[hdr:hdr+4],'little') != 3: continue  # PT_INTERP
            if cls==1:
                offset=int.from_bytes(data[hdr+4:hdr+8],'little')
                size=int.from_bytes(data[hdr+16:hdr+20],'little')
            else:
                offset=int.from_bytes(data[hdr+8:hdr+16],'little')
                size=int.from_bytes(data[hdr+32:hdr+40],'little')
            if offset+size>len(data) or not data[offset:offset+size].startswith(b'/system/bin/linker'):
                raise SystemExit(f'Non-Android dynamic loader in {staged}')
        entries.append(staged)
# Safety gate: do not accidentally package a partial or host-compiled bridge.
zygisk_dir=root/'zygisk'
if args.with_zygisk:
    expected={'arm64-v8a':(2,183),'armeabi-v7a':(1,40),'x86':(1,3),'x86_64':(2,62)}
    for abi,(cls,machine) in expected.items():
        p=zygisk_dir/f'{abi}.so'
        if not p.is_file(): raise SystemExit(f'Cannot package partial Zygisk bridge; missing {p}')
        data=p.read_bytes()
        if len(data)<64 or data[:4]!=b'\x7fELF' or data[4]!=cls or data[5]!=1 or int.from_bytes(data[18:20],'little')!=machine:
            raise SystemExit(f'Wrong Android ABI or invalid ELF for {p}')
        if int.from_bytes(data[16:18],'little')!=3: raise SystemExit(f'ELF is not a shared object: {p}')
        if len(data)<4096: raise SystemExit(f'Suspiciously small shared library: {p}')
        if any(x in data for x in (b'libc.so.6',b'libstdc++.so.6',b'GLIBC_',b'ld-linux-')):
            raise SystemExit(f'Host Linux dynamic dependency detected: {p}')
        # Shared object should not ask the OS to execute through a process interpreter.
        phoff=int.from_bytes(data[28:32] if cls==1 else data[32:40],'little')
        phentsize=int.from_bytes(data[42:44] if cls==1 else data[54:56],'little')
        phnum=int.from_bytes(data[44:46] if cls==1 else data[56:58],'little')
        if phnum>512 or phentsize<(32 if cls==1 else 56) or phoff+phentsize*phnum>len(data):
            raise SystemExit(f'Bad program headers: {p}')
        for i in range(phnum):
            offset=phoff+i*phentsize
            if int.from_bytes(data[offset:offset+4],'little')==3:
                raise SystemExit(f'Shared library has a program interpreter; reject: {p}')
        entries.append(f'zygisk/{abi}.so')
else:
    print('[AY] Controller-only ZIP (Zygisk .so disabled by default).')
# Generate and stage the release integrity manifest over 10 critical files.
critical=['module.prop','customize.sh','action.sh','service.sh','uninstall.sh',
          'scripts/ayctl.sh','webroot/index.html','webroot/assets/app.css',
          'webroot/assets/app.js','webroot/assets/mark.svg']
manifest=''.join(f'{hashlib.sha256((root/name).read_bytes()).hexdigest()}  {name}\n' for name in critical)
(root/'integrity.sha256').write_text(manifest)
with ZipFile(out,'w',compression=ZIP_DEFLATED,compresslevel=9) as z:
    for name in entries:
        path=root/name
        info=ZipInfo(name,date_time=(2026,10,20,0,0,0))
        info.create_system=3
        executable=name.endswith('.sh') or name.startswith('bin/')
        info.external_attr=(0o100755 if executable else 0o100644)<<16
        info.compress_type=ZIP_DEFLATED
        z.writestr(info,path.read_bytes(),compress_type=ZIP_DEFLATED,compresslevel=9)
print(f'{out.name} | {out.stat().st_size:,} bytes | sha256={hashlib.sha256(out.read_bytes()).hexdigest()}')
