"""PlatformIO extra script: build libasm without LTO so that
scripts/teensy41.ld can keep its code and tables in flash, out of RAM1.
libasm serves only the assembler and disassembler commands."""
Import("env")

for lb in env.GetLibBuilders():
    if lb.name == "libasm":
        lb.env.Replace(CCFLAGS=[f for f in lb.env["CCFLAGS"] if "lto" not in f])
