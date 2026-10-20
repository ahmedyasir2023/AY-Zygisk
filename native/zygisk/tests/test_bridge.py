#!/usr/bin/env python3
"""Host-only callback test using a tiny fake API. NOT an Android/real Zygisk test."""
import os
from pathlib import Path
import subprocess
import tempfile
root = Path(__file__).resolve().parents[1]
with tempfile.TemporaryDirectory(prefix='ay-zygisk-bridge-test-') as td:
    exe = Path(td)/'test_bridge'
    subprocess.run([os.environ.get('CXX','c++'), '-std=c++17', '-O2', '-Wall', '-Wextra', '-Werror', '-I',str(root/'tests/include'), str(root/'ay_bridge.cpp'),str(root/'tests/bridge_test.cpp'),'-o',str(exe)],check=True)
    subprocess.run([str(exe)], check=True)
