import os
import subprocess
Import("env")

# Add 'target', 'source', and 'env' to the arguments list
def generate_compiledb(source, target, env):
    print("[LSP] Updating compile_commands.json...")
    subprocess.run(["pio", "run", "-t", "compiledb"])

# Register the action
env.AddPreAction("buildprog", generate_compiledb)
