#!/usr/bin/env python3
import subprocess
import re
import sys

def patch_binary(bin_path):
    try:
        out = subprocess.check_output(['otool', '-L', bin_path]).decode('utf-8', errors='ignore')
    except Exception:
        return

    changes = []
    for line in out.splitlines():
        # Match any Homebrew/Qt absolute framework path: e.g. /usr/local/opt/qtbase/lib/QtCore.framework/Versions/A/QtCore
        m = re.search(r'(/[^ \t]+/(Qt[A-Za-z0-9_]+)\.framework/Versions/A/\2)', line)
        if m:
            old_path = m.group(1)
            name = m.group(2)
            new_path = f'@rpath/{name}.framework/Versions/A/{name}'
            changes.extend(['-change', old_path, new_path])

    if changes:
        # Ensure @executable_path/../Frameworks is in rpath
        subprocess.run(
            ['install_name_tool', '-add_rpath', '@executable_path/../Frameworks', bin_path],
            capture_output=True
        )
        # Change absolute links to @rpath links
        subprocess.run(
            ['install_name_tool'] + changes + [bin_path],
            capture_output=True
        )

if __name__ == '__main__':
    if len(sys.argv) > 1:
        patch_binary(sys.argv[1])
