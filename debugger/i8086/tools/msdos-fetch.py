#!/usr/bin/env python3
"""Fetch the files the MS-DOS disks are built from.

    debugger/i8086/tools/msdos-fetch.py DIR
    debugger/i8086/tools/msdos-fetch.py --source DIR

Downloads each file below from github.com/microsoft/MS-DOS at a pinned
commit (kept, so later runs work offline), checks its sha256, and puts
the system files in DIR/ and the utilities in DIR/bin/. Writes
DIR/.fetched when done.

--source fetches, instead, what msdos-build.py assembles MS-DOS from
on the board, into DIR/src/: Microsoft's 2.11 sources of COMMAND.COM,
SYSINIT and the utilities (ms/), MASM 5.10 and LINK 3.65 from the 4.0 source tree
(tools/), and John Elliott's compiling trees of the MSDOS.SYS kernel,
for 2.11 (jce211/) and 2.00 (jce200/), which complete Microsoft's with
the STDIO.ASM, IO.ASM and IO2.ASM it lacks, and Microsoft C 5.10 from
the 4.0 tree (msc/). Writes DIR/.fetched-source.

The system files are MSDOS.SYS and COMMAND.COM, and SYSINIT.OBJ and
SYSIMES.OBJ, Microsoft's halves of IO.SYS, which samples/i8086/Makefile
links with msdosbio.asm. The binaries are the 2.0 release's: MSDOS.SYS
says "version 2.00", and the OEM kit's SYSINIT.OBJ matches it.

All are Microsoft's, MIT licensed (LICENSE in the repository); so are
John Elliott's trees (COPYING.TXT in each), from www.seasip.info/DOS/.
"""
import hashlib
import io
import os
import sys
import urllib.error
import urllib.request
import zipfile

COMMIT = '2d04cacc5322951f187bb17e017c12920ac8ebe2'
REPO = f'https://raw.githubusercontent.com/microsoft/MS-DOS/{COMMIT}/'
RAW = REPO + 'v2.0/bin/'
SRC = REPO + 'v2.0/source/'
TOOLS = REPO + 'v4.0/src/TOOLS/'
SEASIP = 'https://www.seasip.info/DOS/'

# (name, sha256, subdirectory of DIR)
SOURCES = [
    ('MSDOS.SYS',
     '1edf5190671ed4edcc9e6cc095e5094749c58f1833a2e26e40ba7d3389e276d1', ''),
    ('COMMAND.COM',
     '4cc71b3692b894eef9a7c8b3ac0fc63a71bdfa08175089562fdc4c7c5b038e8a', ''),
    ('SYSINIT.OBJ',
     '56d9d59d8cf6dbec648c3b391d3532bf4dffa8b615a096aca76312acbeecc7aa', ''),
    ('SYSIMES.OBJ',
     '336f44f68eac8061970dcb3b3c4a97558e792cdad4ebfd32720e40174b031a26', ''),
    ('CHKDSK.COM',
     '75be6db096873aed6525e71dcacfe81a56ba038699600d5f203bb40d6a308201', 'bin'),
    ('DEBUG.COM',
     'cbb171acca2a5581d926b6782aebeaad49111ecd359e64cf6f2e5dd0a000b4ca', 'bin'),
    ('EDLIN.COM',
     '799767e5bc6727fd3fb1b0c54a2099c5c53d46dcab5b7b5012b688f3c718b444', 'bin'),
    ('EXE2BIN.EXE',
     '80ac337843421764bb019845f9ef802d35ef642045026b1788373cea756201e2', 'bin'),
    ('FC.EXE',
     '6bf625fcfcd05466f07f3cb7585c22031523d3981cee0f252cf39b7fb4a173e0', 'bin'),
    ('FIND.EXE',
     'd57bab73ba6a77be1aa8d2d3f1fd7d1d6ce6d459b2ab067d26cf63228e940198', 'bin'),
    ('MORE.COM',
     '56830889fef22d56ef5938df0394bf30a8416cc06225b8dc35adc74bb13416b7', 'bin'),
    ('PRINT.COM',
     'b17575ff302d64ba92ec5c26d80355775652eef073c5a3fe099d951553886492', 'bin'),
    ('RECOVER.COM',
     '115a2a262579d1c8831bd6ffaed8ea31e2d156ad5ba399fc365a0ab7d843e8d2', 'bin'),
    ('SORT.EXE',
     'f2135e16cd1ebff6233b4d01fcb7f73b311a1f5524ddca21056248b29fa015b8', 'bin'),
    ('LINK.EXE',
     'fe52892da7bd6832cb88a959cccd230f307e30c7c83a225b2e638f4ea25d76be', 'bin'),
    ('MASM.EXE',
     'bf3399d0ec33fd97c3789887b336208d9f4900524609c04985e16a124cbf4449', 'bin'),
    ('CREF.EXE',
     '9d38031ea566ed01e8c2157f37120e21fd3aa6af2482f0dc329b5ac5bb0fc95e', 'bin'),
]


# Microsoft's 2.11 sources for COMMAND.COM, SYSINIT and the utilities,
# into DIR/src/ms/.
MS_SOURCES = [
    ('COMMAND.ASM',
     'c1302764c2bcd002f3cb45801860052c288879bc0b9851efb7a479c63910c1fa'),
    ('RUCODE.ASM',
     '259cb5a93d1eabf41dffde26fba6981bf0b8478c02dab579788ae73806a8d58f'),
    ('RDATA.ASM',
     'd8c604ce01ac79168812c475f533413b8f1e989726282aa8715573d1e4c436b1'),
    ('INIT.ASM',
     '71b37dff30134898618d86683544fad78f5b994bbd7a10db7766505399a97e2c'),
    ('UINIT.ASM',
     '90eb745deec13cab96d62de0659d6fdee9957e29798cf72a550d0095a4b82d65'),
    ('TCODE.ASM',
     '3366ef918c3a1db855a6235b56324754d56def3292d230c1ffe1c70c1326f805'),
    ('TCODE2.ASM',
     'd15b9224adadc523f8614a28bc26e85cee9ee2349436b2b49997daeffa37c770'),
    ('TCODE3.ASM',
     '702c57c7c2baa093220f56a3fd963472321921ce9a65dd3d15f867c6f50358fc'),
    ('TCODE4.ASM',
     '87220d898495e20353c0e9cc9ce18c6aa9667968bb0803320faf8e89f9daf31f'),
    ('TCODE5.ASM',
     'cddfcb3264c9113fc9efee558cfce40f6e43401ca8049c0b129a341e1146977d'),
    ('TUCODE.ASM',
     '6b1ce4e01108b591ef6f81242260d6db0f6a858f3e8e3cfbd3c5b2ec5a305625'),
    ('COPY.ASM',
     '6805f81f95f599d0ac5c9e96d518635068534c759dfb1c0c93520ddf54e3ab5c'),
    ('COPYPROC.ASM',
     'c57272ff63a3365af1b0c3563faa9bfc6cf881dc077aa219208b409dd7f7c0e7'),
    ('CPARSE.ASM',
     'ca03024598c8e0f6339cf2ca9f7fb4345270115cca85238294f75c8d515de262'),
    ('TDATA.ASM',
     '457bc51d8deb1e67be3467a3e2456f242789f44bafba394ef9b61a8e9faa53e2'),
    ('TSPC.ASM',
     '9a7b3ff369d81b6cec091c237930394e73f2427c19cc5a3a8a88cd070203cec9'),
    ('COMEQU.ASM',
     '63517d9b74f32b268f57ddf95d1493e04d1a36143794d1c91194535740cc5c1e'),
    ('COMSEG.ASM',
     'd8579d60d5ff1b454f224c37275bfcb9ad2fe0eae6e07c54e980cae1abab6c19'),
    ('COMSW.ASM',
     '97765c99de1978164fa1ed59ac387981c19955fbaa7bad2ac55e1d4e0d3e41f2'),
    ('IFEQU.ASM',
     '15209d8c38ee649415395197f41c4d78f5137a4b783abf19c08a1ecb6122c589'),
    ('SYSINIT.ASM',
     'a2366ea8f9a8b4ab6fd2d6cfd73b76068312d627368ebbffe05f872a416ddf99'),
    ('SYSIMES.ASM',
     'cd00f00e30a1057b1d668051b81221ff90d0e71726471ba1d68c2724f2d4bdac'),
    ('CHKDSK.ASM',
     '2a503e8f1f582e0bcf4323d77edd533d6bc613458db66606814196af37039053'),
    ('CHKPROC.ASM',
     'a7afdf37d464ecf9bf4d006b5d7f3a7b07f1c72bfb51a6f4ab0c68d478348984'),
    ('CHKMES.ASM',
     'dae8a032c27cbc2f5e3519104aea2c451bfc5768705ffcf3d24626fc637e38b2'),
    ('DEBUG.ASM',
     'c498a091b8e11d63852deabba65b60c581d42fe16abd6e7d32f5f184f6fd7663'),
    ('DEBCOM1.ASM',
     'a3dbb05b1aa8fcb37358298d6ac9bd5736f3a0a20958ace9c84a469221c8909a'),
    ('DEBCOM2.ASM',
     '2d73b90047da97678e056e5625ce7332ddff870d52a1f41f41039d2f80647652'),
    ('DEBUASM.ASM',
     '7ab87bb6f4e70f863de78a5f87955d4fc57f42811a63d8cb2cc66a3b31635fc1'),
    ('DEBASM.ASM',
     'f218367a68974c133f47bd0aff11d033dbbc059a9fffae82f8213f4e28d959fb'),
    ('DEBCONST.ASM',
     '73ca6b149f03b61a4c446a94958c0675f94fc64afdb8449c396dc125ced3b616'),
    ('DEBDATA.ASM',
     '2f186bb1e9a7945a1028dfb07fa7879ebddc215836e042540c214bc310733c00'),
    ('DEBMES.ASM',
     '0cc4d6d5058041fa7c6e90fd73fec8dfcdcbb418b4757dadb1298da27c6b6c30'),
    ('DEBEQU.ASM',
     'ba5b781bf9e4b52e9f15eae4a894090da96d166c6cc0852c1ffadaa9eb13937a'),
    ('EDLIN.ASM',
     '5751619c71228b8ceaba3866056118f1ab12d1b05ab90b8c89f8abac5f8a7c62'),
    ('EDLPROC.ASM',
     'dc425bef63c4c9c7639cd9ba5ec4b63f1440d96900c4670239b50662124e2f43'),
    ('EDLMES.ASM',
     '2c244df61181f7035911f2c51789e0f8d60ac73c706f4e5035f3f28ad8a925b0'),
    ('EXE2BIN.ASM',
     'e043066827c01284df4a0b68c30f16af61c831791bfb64b7be661f8181136b68'),
    ('EXEMES.ASM',
     '07e46739bb3cf67f9ba655fc83195d96227dbda5e47d49b68bdfcb0fba7f98b9'),
    ('FC.ASM',
     '82e0291ae04fe476e1d25116ee319eaf5409fb00c8f7aea101d7cfcc20e339f9'),
    ('FCMES.ASM',
     'f46401d14aaeb837b591a62cc6f452584c738cb23ded54dd530488e011f72415'),
    ('FIND.ASM',
     '487307c6982c72f544b6f8a89dd99ff16ee949a8f00fa298f5c7048a94edaba4'),
    ('FINDMES.ASM',
     '4ab87d946138566b9907bdbf72f1e31f945fdacd3bfb606e3bd20211ee916b77'),
    ('MORE.ASM',
     'b3b56fad5c9938a08de03209c21085e814b74f4299d152cd5fe2ec490c50305f'),
    ('MOREMES.ASM',
     '093f39cf84b87e96b86e3da288803a4ae2fb293cfb7b494cf5359f3fc265aded'),
    ('SORT.ASM',
     '4d99cb8ab2772d7f228d5cab27f3650e40728c5738865ae2db6aad67b3cf3b49'),
    ('SORTMES.ASM',
     '2fc7c59d4e9712e29702814870b7d129a940dd19e21733f65d05198ae9603f3c'),
    ('RECOVER.ASM',
     '1aad5a9de16078546060da18ea62b1a27b2fa1627afcbb5106bb07c3b18b5a0a'),
    ('RECMES.ASM',
     '5cc03e48de375e5fe5bded15c646d620bba86cf9f972b087340c2a92928ef716'),
    ('PRINT_v211.ASM',
     '78154eb64fc55dbbc8d729514fa16fea80c185ca4f0cfbf5fa6baab50c6346ef'),
]

# (name, sha256): MASM 5.10 and LINK 3.65, into DIR/src/tools/.
TOOL_SOURCES = [
    ('MASM.EXE',
     '1c6286c69b616160b8475ad17453ee872702a2c98075c2159ec792a1c745275f'),
    ('LINK.EXE',
     '124a3c800edc16b60f696d35a9dfec798b68b78fe2ca90c5988ea76bdeab1a8a'),
]

# Microsoft C 5.10 for DOS from the 4.0 source tree, into DIR/src/msc/:
# the driver and its passes, then the headers and the small model's
# libraries, whose floating point uses an 8087 when there is one.
MSC_TOOLS = [
    ('CL.EXE',
     'cad40cef732beea3455225fbd62286c664166db7c3f87e2402e22b2e3f107771'),
    ('C1.EXE',
     '8d33c6d87aa74b042b7d7cb01a23241c4cc3b1b8b446b8f1548bd167e3c7bf62'),
    ('C1L.EXE',
     'bdd8189be005009331ff61dd3cb7a0ce9b246ea522dd8ab2c83a8339e5c30f09'),
    ('C2.EXE',
     'a513c98bc2ce11bd00901b560252b5727ab9b6bacd9319b9eaba08f9e314b99b'),
    ('C3.EXE',
     'c1269628534d5deb48171ba9e42f761dcb1c28a0087c73780db0ee2e4352e6fa'),
    ('CL.ERR',
     'ce4269dfd6cab37278cfa18f071fb95f355dbee6fbcb6e3f6092688867c0d458'),
    ('C1.ERR',
     '37a2ac69c89d1e65c6596cfb3860eee33009b47c1c4ed1360a87a8a7ad78cace'),
    ('C23.ERR',
     '4f0ab17a19cee78a46644c3493570c4f9b3cbbdb501672814e4bfe00041e0648'),
]
MSC_INC = [
    ('ASSERT.H',
     '4ded40835050eb5ec16e8f8d45c46c235d17d93f28f91e944130c7b99ac280f1'),
    ('BIOS.H',
     '89153ce0f56d360c018af7dd2036d38b8d723dc77b01e35f43571408b431ad0c'),
    ('CONIO.H',
     'bd992d2e2f6133eb6383dd69e18722b8a96fe64d7bcac378493c5676eacdde98'),
    ('CTYPE.H',
     '70f06f727032405dc6a150d9994bceed16c20dfdefebf05621537082cd693a34'),
    ('DIRECT.H',
     'cd0521ff760aa0071fddb1f67ef20df050caebe0f0ece117a376afe33df10748'),
    ('DOS.H',
     '9e9f6a4aaccebc7e5b06aec492b39eb24b27286b651372d19ff931bddd867707'),
    ('ERRNO.H',
     'e1c43fcb2ae6ef855c9a54c65aed9cf06cd521e44d0470b19105cd54c5c89216'),
    ('FCNTL.H',
     '7df7ed093daa2e6a91d6072bdb10616d3c16a1a895aa82523afb249540b07c51'),
    ('FLOAT.H',
     '255d5b4c34dfd9440d111b15acbd65fde913e31fd97bed2d9a1426f45e0230f9'),
    ('GRAPH.H',
     '5904565b68e520b8f64d70c9f289b87a63b7cddc50276c27f5adf1a235753c2e'),
    ('IO.H',
     '514b4095c7d3cd370169bb1fe0e50c3c3a3695ea40edab0e524eb3fd1ec53467'),
    ('LIMITS.H',
     '7ff78b457d63a96c016d7f431560cd5046412270a429361e04f40d3febcb89c3'),
    ('MALLOC.H',
     '73631695ee7ee28fae982a7e0d80192e8ad063b9e370671288d1b657651721e1'),
    ('MATH.H',
     'bff4866d697796b643e85f1e83869d204c6feb3426e472cea2cae01f20189157'),
    ('MEMORY.H',
     '55711253b29758819da849f30d0c9e69f3388fe00e685063a8da2c9269834fd9'),
    ('PROCESS.H',
     'be5b37fb74d055d38888f6c0e410e9aea659f8077f1713feb7b0868671f59c58'),
    ('SEARCH.H',
     '47c3f32afef98a94a243f1791d629582d0fa8a79dd9554f2f489482022b3d138'),
    ('SETJMP.H',
     '28849f94840b09d5b41c4d66dfa54eeda34987e64e989693ec10aca8de6c8463'),
    ('SHARE.H',
     '654d0947374bbed1e250c6a42583039c76bae85be288532c9067ebcef72c10e5'),
    ('SIGNAL.H',
     'd16f94694e3da2249eb65bebc677187185bc613658cd1f031605ace61cc4f839'),
    ('STDARG.H',
     '6225bde6e8b0f94c1fdfb91104adb04814ba6faa812b121e456e9f03f56c90e4'),
    ('STDDEF.H',
     'cd0e5227826164d5e71e3af5e3f6e0b0fec1035f8693c382d9de9a9879f49217'),
    ('STDIO.H',
     'c59530359c3724f956e60eb018c0879c8fc81f99723bb7005e450f08d6458a15'),
    ('STDLIB.H',
     'f645286d3ea232ba8d77ece58192d8a2c28c5e5c88575c9488787469203cfc9e'),
    ('STRING.H',
     'ce145da80a8f5dd87dbef199584d2a12d93148093a5c5e0af328e5bb43c873ef'),
    ('TIME.H',
     'b08ec495aeae36dbf6cb3ecdf80d6e8e6549095940466c6e32f8adacfa953d20'),
    ('VARARGS.H',
     '83a3b3abe0cb4940183efdfee1181c6cd8d6174a0695a02dd85124e2e25e9aa9'),
]
MSC_LIB = [
    ('SLIBCE.LIB',
     'd077b91d63d051d3333ca53f81cb15b0597cfdb6ec8a7fadc60f76ab6883e24f'),
    ('EM.LIB',
     '77620fa639ac91fad940c3539a66f47adbbb124958b38e540d386de79f582403'),
    ('LIBH.LIB',
     '6729e396014d37921b0b331cd0f9a3a821abb725fd2ee0e795d71b0e4b627e9c'),
]

# (archive, sha256, subdirectory of DIR/src it is unpacked into)
KERNEL_TREES = [
    ('msdos211.zip',
     'ed47b2725289c78fa32b6c96bc8c3cf7e3d929c6d51e118b581f35aa31a5a049',
     'jce211'),
    ('msdos200.zip',
     '7d5e81a10e2692fece52231da9636baaef1e0d8394205c4f6027d2774a2ca340',
     'jce200'),
]


def fetch(name, sha, path, base=RAW):
    """Leaves |path| holding |name| from |base|, and returns its bytes."""
    if os.path.exists(path):
        with open(path, 'rb') as f:
            data = f.read()
        if hashlib.sha256(data).hexdigest() == sha:
            return data
    url = base + name
    print('fetch', url)
    req = urllib.request.Request(url, headers={'User-Agent': 'msdos-fetch'})
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


def fetch_source(top):
    src = os.path.join(top, 'src')
    for sub, base, files in (('ms', SRC, MS_SOURCES),
                             ('tools', TOOLS, TOOL_SOURCES),
                             ('msc', TOOLS, MSC_TOOLS),
                             ('msc', TOOLS + 'BLD/INC/', MSC_INC),
                             ('msc', TOOLS + 'BLD/LIB/', MSC_LIB)):
        out = os.path.join(src, sub)
        os.makedirs(out, exist_ok=True)
        for name, sha in files:
            fetch(name, sha, os.path.join(out, name), base)
    for name, sha, sub in KERNEL_TREES:
        data = fetch(name, sha, os.path.join(src, name), SEASIP)
        out = os.path.join(src, sub)
        os.makedirs(out, exist_ok=True)
        with zipfile.ZipFile(io.BytesIO(data)) as z:
            for info in z.infolist():
                name = os.path.basename(info.filename)
                if name and not info.is_dir():
                    with open(os.path.join(out, name.upper()), 'wb') as f:
                        f.write(z.read(info))
    open(os.path.join(top, '.fetched-source'), 'w').close()


def main(argv):
    if len(argv) == 2 and argv[0] == '--source':
        fetch_source(argv[1])
        return
    if len(argv) != 1:
        sys.exit(__doc__)
    top = argv[0]
    for name, sha, sub in SOURCES:
        out = os.path.join(top, sub)
        os.makedirs(out, exist_ok=True)
        fetch(name, sha, os.path.join(out, name))
    open(os.path.join(top, '.fetched'), 'w').close()


if __name__ == '__main__':
    main(sys.argv[1:])
