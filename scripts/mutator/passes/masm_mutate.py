import os
import random
import re

def get_junk_math(reg):
    return [
        f"lea {reg}, [{reg}]",
        f"lea {reg}, [{reg} + 0]",
        f"lea {reg}, [{reg}*1]",
        f"mov {reg}, {reg}"
    ]

def mutate_masm_file(filepath):
    """
    Reads an assembly file, injects metamorphic mutations, and overwrites it.
    """
    if not filepath.endswith(".asm"):
        return

    is_x64 = "x64" in filepath

    if is_x64:
        volatile_registers = ['r8', 'r9', 'rax']
        functional_nops = [
            "nop",
            "xchg rax, rax",
            "xchg rbx, rbx",
            "xchg rdx, rdx"
        ]
    else:
        volatile_registers = ['eax', 'edx']
        functional_nops = [
            "nop",
            "xchg eax, eax",
            "xchg ebx, ebx",
            "xchg edx, edx"
        ]

    with open(filepath, 'r') as f:
        lines = f.readlines()

    mutated_lines = []

    instr_pattern = re.compile(r'^\s*[a-zA-Z]+\s+[^:]*$')

    has_ended = False
    skip_next = False

    for line in lines:
        stripped = line.strip()
        lower_stripped = stripped.lower()

        mutated_lines.append(line)

        if lower_stripped == "end":
            has_ended = True

        is_cmp_or_test = lower_stripped.startswith(('test', 'cmp'))

        is_control_flow = lower_stripped.startswith((
            'ret', 'call', 'jmp',
            'je', 'jz', 'jne', 'jnz',
            'jg', 'jnle', 'jge', 'jnl',
            'jl', 'jnge', 'jle', 'jng',
            'ja', 'jnbe', 'jae', 'jnb',
            'jb', 'jnae', 'jbe', 'jna',
            'js', 'jns', 'jc', 'jnc', 'jo',
            'jno', 'jp', 'jpe', 'jnp', 'jpo'
        ))

        can_mutate = not has_ended and instr_pattern.match(line) and not is_control_flow and not skip_next

        skip_next = is_cmp_or_test

        if can_mutate:
            if random.random() < 0.50:
                choice = random.choice(['nop', 'math'])
                if choice == 'nop':
                    nop_instr = random.choice(functional_nops)
                    indent = line[:len(line) - len(line.lstrip())]
                    mutated_lines.append(f"{indent}{nop_instr} ; [mutator: functional nop]\n")
                else:
                    reg = random.choice(volatile_registers)
                    math_instr = random.choice(get_junk_math(reg))
                    indent = line[:len(line) - len(line.lstrip())]
                    mutated_lines.append(f"{indent}{math_instr} ; [mutator: junk math]\n")

    with open(filepath, 'w') as f:
        f.writelines(mutated_lines)

def run(build_dir):
    """
    Entry point for the pass. Iterates through the src directory in the build_tmp.
    """
    src_dir = os.path.join(build_dir, "src")
    if not os.path.exists(src_dir):
        return

    for root, _, files in os.walk(src_dir):
        for file in files:
            if file.endswith(".asm"):
                filepath = os.path.join(root, file)
                mutate_masm_file(filepath)
                print(f"[masm_mutate] Mutated {filepath}")
