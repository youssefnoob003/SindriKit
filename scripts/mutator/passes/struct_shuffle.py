import os
import random

def shuffle_struct_block(lines):
    """
    Given a list of lines representing the inner members of a struct,
    shuffles them. It treats inner structs/unions as single atomic blocks
    so their internal braces don't get scrambled.
    """
    if any(line.strip().startswith('#') for line in lines):
        return lines

    blocks = []
    current_block = []
    brace_depth = 0

    for line in lines:
        if not line.strip():
            continue

        current_block.append(line)
        brace_depth += line.count('{') - line.count('}')

        stripped = line.strip()
        if '//' in stripped:
            stripped = stripped[:stripped.find('//')].strip()

        if brace_depth == 0 and stripped.endswith(';'):
            blocks.append(current_block)
            current_block = []

    if current_block:
        blocks.append(current_block)

    random.shuffle(blocks)

    shuffled_lines = []
    for block in blocks:
        shuffled_lines.extend(block)

    return shuffled_lines

def mutate_header_file(filepath):
    """
    Reads a header file, looks for SND_SHUFFLE_START and SND_SHUFFLE_END,
    and shuffles the fields of structs contained within.
    """
    if not filepath.endswith(".h"):
        return

    with open(filepath, 'r') as f:
        lines = f.readlines()

    mutated_lines = []
    in_shuffle_block = False
    in_struct_body = False
    current_struct_lines = []
    brace_depth = 0

    for line in lines:
        stripped = line.strip()

        if stripped == "SND_SHUFFLE_START":
            in_shuffle_block = True
            continue
        elif stripped == "SND_SHUFFLE_END":
            in_shuffle_block = False
            continue

        if in_shuffle_block:
            if not in_struct_body:
                if "{" in line:
                    in_struct_body = True
                    brace_depth = line.count('{') - line.count('}')
                    mutated_lines.append(line)
                else:
                    mutated_lines.append(line)
            else:
                brace_depth += line.count('{') - line.count('}')

                if brace_depth == 0 and "}" in line:
                    in_struct_body = False

                    shuffled = shuffle_struct_block(current_struct_lines)
                    mutated_lines.extend(shuffled)
                    current_struct_lines = []
                    mutated_lines.append(line)
                else:
                    current_struct_lines.append(line)
        else:
            mutated_lines.append(line)

    with open(filepath, 'w') as f:
        f.writelines(mutated_lines)

def run(build_dir):
    """
    Entry point for the pass. Iterates through the include directory in the build_tmp.
    """
    include_dir = os.path.join(build_dir, "include")
    if not os.path.exists(include_dir):
        return

    for root, _, files in os.walk(include_dir):
        for file in files:
            if file.endswith(".h"):
                filepath = os.path.join(root, file)
                with open(filepath, 'r') as f:
                    content = f.read()
                if "SND_SHUFFLE_START" in content:
                    mutate_header_file(filepath)
                    print(f"[struct_shuffle] Shuffled {filepath}")
