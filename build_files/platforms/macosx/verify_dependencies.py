#!/usr/bin/env python3
"""Check that Mach-O dependencies resolve inside the app in their load context."""

import os
import re
import subprocess
import sys
from pathlib import Path

app = Path(sys.argv[1]).resolve()
files = [Path(os.fsdecode(p)) for p in Path(sys.argv[2]).read_bytes().split(b'\0') if p]
files = {p.resolve() for p in files}
main = app / 'Contents/MacOS/luminance-hdr'
helper = app / 'Contents/Frameworks/QtWebEngineCore.framework/Versions/A/Helpers/QtWebEngineProcess.app/Contents/MacOS/QtWebEngineProcess'
cache = {}
errors = set()
visited = set()
seen_files = set()


def inspect(path):
    if path not in cache:
        deps_output = subprocess.check_output(['otool', '-L', str(path)], text=True)
        load_output = subprocess.check_output(['otool', '-l', str(path)], text=True)
        id_output = subprocess.run(['otool', '-D', str(path)], text=True, capture_output=True).stdout.splitlines()
        install_id = id_output[1].strip() if len(id_output) > 1 else ''
        deps = []
        for line in deps_output.splitlines()[1:]:
            match = re.match(r'^\s*(.*?)\s+\(compatibility version ', line)
            if match and match.group(1) != install_id:
                deps.append(match.group(1))
        rpaths = re.findall(r'\bcmd LC_RPATH\s+cmdsize \d+\s+path (.*?) \(offset \d+\)', load_output)
        maximum = os.environ.get('LHDR_MAX_MINIMUM_OS')
        versions = re.findall(r'\bcmd LC_BUILD_VERSION\s+cmdsize \d+\s+platform (?:MACOS|1)\s+minos ([0-9.]+)', load_output)
        versions += re.findall(r'\bcmd LC_VERSION_MIN_MACOSX\s+cmdsize \d+\s+version ([0-9.]+)', load_output)
        if maximum and not versions:
            errors.add(f'{path.relative_to(app)}: missing macOS minimum version')
        if maximum:
            for version in versions:
                current = tuple(map(int, version.split('.')))
                limit = tuple(map(int, maximum.split('.')))
                if (current + (0, 0, 0))[:3] > (limit + (0, 0, 0))[:3]:
                    errors.add(f'{path.relative_to(app)}: minimum macOS {version} exceeds {maximum}')
        cache[path] = deps, rpaths, install_id
    return cache[path]


def expand(value, loader, executable):
    if value.startswith('@loader_path/'):
        return loader.parent / value[len('@loader_path/'):]
    if value.startswith('@executable_path/'):
        return executable.parent / value[len('@executable_path/'):]
    if value.startswith('/'):
        return Path(value)
    return None


def visit(path, executable, inherited):
    path = path.resolve()
    deps, own_rpaths, install_id = inspect(path)
    paths = []
    for item in own_rpaths:
        expanded = expand(item, path, executable)
        if expanded is not None:
            paths.append(expanded)
    paths = tuple(dict.fromkeys((*paths, *inherited)))
    key = (path, executable, paths)
    if key in visited:
        return
    visited.add(key)
    seen_files.add(path)
    if install_id.startswith('/') and not install_id.startswith(('/usr/lib/', '/System/Library/')):
        errors.add(f'{path.relative_to(app)}: external install name {install_id}')
    for dep in deps:
        if dep.startswith(('/usr/lib/', '/System/Library/')):
            continue
        if dep.startswith('@rpath/'):
            targets = [root / dep[len('@rpath/'):] for root in paths]
        else:
            target = expand(dep, path, executable)
            targets = [target] if target is not None else []
        target = next((p.resolve() for p in targets if p.exists()), None)
        if target is None:
            errors.add(f'{path.relative_to(app)}: missing or unsupported dependency {dep}')
        elif not target.is_relative_to(app):
            errors.add(f'{path.relative_to(app)}: external dependency {dep} resolves to {target}')
        elif target not in files:
            errors.add(f'{path.relative_to(app)}: dependency is not a bundled Mach-O: {dep}')
        else:
            visit(target, executable, paths)


# Plugins are loaded by the GUI; the helper and command-line tools are separate
# processes with their own @executable_path and run-path stacks.
executables = [p for p in files if p.parent == main.parent.resolve()]
if helper.resolve() in files:
    executables.append(helper.resolve())
for executable in executables:
    visit(executable, executable, ())
main_rpaths = tuple(filter(None, (expand(r, main, main) for r in inspect(main.resolve())[1])))
for path in files - seen_files:
    visit(path, main.resolve(), main_rpaths)

if errors:
    print('Bundle dependency verification failed:', file=sys.stderr)
    for error in sorted(errors):
        print(f'  - {error}', file=sys.stderr)
    sys.exit(1)
