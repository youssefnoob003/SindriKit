import os
import shutil
import sys
import argparse

import passes.masm_mutate
import passes.struct_shuffle
import passes.junk_c

def parse_args():
    parser = argparse.ArgumentParser(description="SindriKit Pre-Compilation Polymorphic Mutation Engine")
    parser.add_argument("--in-dir", required=True, help="source directory")
    parser.add_argument("--out-dir", required=True, help="Output directory for morphed source")
    return parser.parse_args()

def setup_tmp_workspace(src_dir, build_tmp_dir):
    """
    Creates the output directory and copies necessary directories into it.
    """
    print(f"[*] Setting up morphed workspace at {build_tmp_dir}...")
    if os.path.exists(build_tmp_dir):
        shutil.rmtree(build_tmp_dir)

    os.makedirs(build_tmp_dir)

    dirs_to_copy = ["src", "include", "asm", "tests", "pocs", "scripts", "config"]
    for d in dirs_to_copy:
        src_path = os.path.join(src_dir, d)
        dst_path = os.path.join(build_tmp_dir, d)
        if os.path.exists(src_path):
            shutil.copytree(src_path, dst_path)

def run_mutation_passes(build_tmp_dir):
    """
    Executes the configured mutation passes against the tmp workspace.
    """
    print("[*] Running Mutation Passes...")

    print("  [-] Executing pass: masm_mutate")
    passes.masm_mutate.run(build_tmp_dir)

    print("  [-] Executing pass: struct_shuffle")
    passes.struct_shuffle.run(build_tmp_dir)

    print("  [-] Executing pass: junk_c")
    passes.junk_c.run(build_tmp_dir)

def main():
    args = parse_args()

    try:
        setup_tmp_workspace(args.in_dir, args.out_dir)
        run_mutation_passes(args.out_dir)
        print("[+] Morphing completed successfully.")
    except Exception as e:
        print(f"[-] Morphing failed: {e}")
        sys.exit(1)

if __name__ == "__main__":
    main()
