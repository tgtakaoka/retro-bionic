"""PlatformIO extra script for the memory layout in scripts/teensy41.ld.

libasm builds without LTO so that the linker script can keep its code and
tables in flash; it serves only the assembler and disassembler commands.

With ITCM_OVERLAY defined, each target directory's code goes to an ITCM
overlay, which Identity::instance() loads for the one target a power-up
runs. The overlays are generated here from the namespaces each directory
declares, since link-time function sections carry namespace-mangled
names. Without it, overlays.ld is empty and every target stays in ITCM."""
import os
import re

Import("env")

for lb in env.GetLibBuilders():
    if lb.name == "libasm":
        lb.env.Replace(CCFLAGS=[f for f in lb.env["CCFLAGS"] if "lto" not in f])

overlay = "ITCM_OVERLAY" in env.get("CPPDEFINES", [])


def target_namespaces(src_dir):
    targets = {}
    for d in sorted(os.listdir(src_dir)):
        path = os.path.join(src_dir, d)
        if not os.path.isdir(path):
            continue
        names = set()
        for f in os.listdir(path):
            if f.endswith((".cpp", ".h")):
                with open(os.path.join(path, f)) as src:
                    names.update(re.findall(r"^namespace (\w+) \{", src.read(), re.M))
        names.discard("debugger")
        if names:
            targets[d] = sorted(names)
    return targets


def overlay_script(targets):
    lines = ["OVERLAY : NOCROSSREFS AT(LOADADDR(.text.progmem) + SIZEOF(.text.progmem)) {"]
    for d, names in targets.items():
        patterns = []
        for ns in names:
            m = "8debugger%d%s" % (len(ns), ns)
            patterns += [".text._ZN%s*" % m, ".text._ZNK%s*" % m,
                         ".text._ZZN%s*" % m, ".text._ZZNK%s*" % m,
                         ".text._ZThn*_N%s*" % m, ".text._ZThn*_NK%s*" % m]
        lines.append("\t.ovl_%s { *(%s) }" % (d, " ".join(patterns)))
    lines.append("} > ITCM")
    first, last = list(targets)[0], list(targets)[-1]
    lines.append("__overlay_vma = ADDR(.ovl_%s);" % first)
    lines.append(".text.overlays (NOLOAD) : {")
    lines.append("\t. = . + (__load_stop_ovl_%s - __load_start_ovl_%s);" % (last, first))
    lines.append("} > FLASH")
    return "\n".join(lines) + "\n"


build_dir = env.subst("$BUILD_DIR")
os.makedirs(build_dir, exist_ok=True)
with open(os.path.join(build_dir, "overlays.ld"), "w") as ld:
    if overlay:
        ld.write(overlay_script(target_namespaces(env.subst("$PROJECT_SRC_DIR"))))
env.Append(LIBPATH=[build_dir])


def check_overlay_calls(target, source, env):
    """Fail if code outside the overlays calls into them: it would run
    whichever target the overlay holds, or none before it is loaded."""
    import subprocess
    elf = target[0].get_abspath()
    tools = env.subst("$CC").replace("gcc", "")
    nm = subprocess.run([tools + "nm", elf], capture_output=True, text=True).stdout
    syms = dict(l.split()[::-2] for l in nm.splitlines() if len(l.split()) == 3)
    start, stop = int(syms["__overlay_vma"], 16), int(syms["_stext"], 16)
    relocs = subprocess.run([tools + "readelf", "-rW", elf],
                            capture_output=True, text=True).stdout
    section, calls = None, []
    for line in relocs.splitlines():
        m = re.match(r"Relocation section '\.rel(\S+)'", line)
        if m:
            section = m.group(1)
            continue
        f = line.split()
        if (len(f) >= 5 and not section.startswith(".ovl_")
                and re.match(r"R_ARM_THM_(CALL|JUMP)", f[2])
                and start <= int(f[3], 16) < stop):
            calls.append("%s calls %s" % (section, f[4]))
    if calls:
        print("\n".join(calls))
        env.Exit(1)


if overlay:
    env.Append(LINKFLAGS=["-ffunction-sections", "-fdata-sections",
                          "-Wl,--emit-relocs"])
    env.AddPostAction("$BUILD_DIR/${PROGNAME}.elf",
                      env.VerboseAction(check_overlay_calls, "Checking overlay calls"))
