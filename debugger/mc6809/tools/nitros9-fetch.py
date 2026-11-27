#!/usr/bin/env python3
"""Fetch the NitrOS-9 sources and the tools that build them.

    debugger/mc6809/tools/nitros9-fetch.py DIR [PORT]

Downloads each archive below into DIR/dl/ (kept, so later runs work
offline), checks its sha256, and unpacks: the NitrOS-9 tree into
DIR/nitros9/, ToolShed's os9 command into DIR/bin/, and LWTOOLS, built
from source, into DIR/bin/ too. The port directory PORT, samples/
mc6809/nitros9/ beside DIR unless given, is linked into the tree as
level1/bionic, where NitrOS-9's own rules find a port's modules. Writes
DIR/.fetched when done.

NitrOS-9 is the NitrOS-9 Project's, GPLv2 (https://nitros9.org/);
LWTOOLS is William Astle's (http://www.lwtools.ca/), GPLv3; ToolShed is
the NitrOS-9 Project's too.
"""
import hashlib
import io
import os
import shutil
import subprocess
import sys
import tarfile
import urllib.error
import urllib.request

NITROS9_COMMIT = '361dded240afab133216aa7f5cdab2681a29f6a3'  # 2026-11-25
NITROS9 = ('https://github.com/nitros9project/nitros9/archive/'
           f'{NITROS9_COMMIT}.tar.gz',
           '5307acdc6d3fbdb9d7fe3a73075ac427761746ea6cb27cd8dfecb529cccdcc8f')
LWTOOLS = ('http://www.lwtools.ca/releases/lwtools/lwtools-4.25.tar.gz',
           '9bb97e987d486d7abd85d04cb98a53f666f02785c108647b795c4a111c5eb09f')
TOOLSHED = ('https://github.com/nitros9project/toolshed/releases/download/'
            'v2.7.0/toolshed-unix64-2.7.0.tgz',
            '13ab012902af31cbe7cd05f0d3447267e2150f1ab6a884ba75e9d3634f41d072')


def download(url, sha, cache):
    path = os.path.join(cache, url.rsplit('/', 1)[1])
    if os.path.exists(path):
        with open(path, 'rb') as f:
            data = f.read()
        if hashlib.sha256(data).hexdigest() == sha:
            return data
    print('fetch', url)
    req = urllib.request.Request(url, headers={'User-Agent': 'nitros9-fetch'})
    try:
        with urllib.request.urlopen(req) as r:
            data = r.read()
    except urllib.error.URLError as e:
        sys.exit(f'{url}: {e.reason}')
    if hashlib.sha256(data).hexdigest() != sha:
        sys.exit(f'{url}: sha256 mismatch')
    with open(path, 'wb') as f:
        f.write(data)
    return data


def unpack(data, dest):
    """Extract a tarball's single top directory as |dest|, afresh."""
    shutil.rmtree(dest, ignore_errors=True)
    with tarfile.open(fileobj=io.BytesIO(data), mode='r:gz') as tar:
        members = tar.getmembers()
        top = members[0].name.split('/', 1)[0]
        for m in members:
            if m.name == top:
                continue
            if not m.name.startswith(top + '/') or '..' in m.name.split('/'):
                sys.exit(f'{dest}: unexpected member {m.name}')
            m.name = m.name[len(top) + 1:]
        tar.extractall(dest, members=[m for m in members if m.name],
                       filter='data')


def main(argv):
    if len(argv) not in (1, 2):
        sys.exit(__doc__)
    top = os.path.abspath(argv[0])
    port = os.path.abspath(argv[1]) if len(argv) == 2 else \
        os.path.join(os.path.dirname(top), 'nitros9')
    cache = os.path.join(top, 'dl')
    bindir = os.path.join(top, 'bin')
    os.makedirs(cache, exist_ok=True)
    os.makedirs(bindir, exist_ok=True)

    tree = os.path.join(top, 'nitros9')
    unpack(download(*NITROS9, cache), tree)
    link = os.path.join(tree, 'level1', 'bionic')
    os.symlink(os.path.relpath(port, os.path.dirname(link)), link)

    toolshed = os.path.join(top, 'toolshed')
    unpack(download(*TOOLSHED, cache), toolshed)
    shutil.copy2(os.path.join(toolshed, 'os9'), os.path.join(bindir, 'os9'))

    lwtools = os.path.join(top, 'lwtools')
    unpack(download(*LWTOOLS, cache), lwtools)
    print('build lwtools')
    log = os.path.join(top, 'lwtools-build.log')
    with open(log, 'w') as out:
        r = subprocess.run(['make', '-j', str(os.cpu_count() or 1),
                            f'PREFIX={top}', 'install'],
                           cwd=lwtools, stdout=out, stderr=subprocess.STDOUT)
    if r.returncode != 0:
        sys.exit(f'lwtools build failed, see {log}')
    open(os.path.join(top, '.fetched'), 'w').close()


if __name__ == '__main__':
    main(sys.argv[1:])
