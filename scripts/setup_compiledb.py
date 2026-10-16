"""PlatformIO extra script: regenerate compile_commands.json for clangd
before each build (not loaded in CI; see platformio.ini)."""
import os
import subprocess
Import("env")

def generate_compiledb(source, target, env):
    print("[LSP] Updating compile_commands.json...")
    subprocess.run(["pio", "run", "-t", "compiledb"])

env.AddPreAction("buildprog", generate_compiledb)
