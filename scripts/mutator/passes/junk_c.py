import os
import random
import re

def generate_dead_functions(count):
    funcs = []
    names = []
    for _ in range(count):
        name = f"snd_dead_func_{random.randint(0x1000, 0xFFFF):x}{random.randint(0x1000, 0xFFFF):x}"
        names.append(name)

        val1 = random.randint(3, 999)
        val2 = random.randint(11, 4000)
        val3 = random.randint(2, 8)

        func_body = f"""
static int {name}(int a, int b) {{
    int v = a ^ b;
    for(int i = 0; i < {val3}; i++) {{
        v = (v << {random.randint(1, 3)}) | (v >> {random.randint(28, 31)});
        v += (b * {val1});
    }}
    return v ^ {val2};
}}
"""
        funcs.append(func_body)
    return funcs, names

def get_opaque_predicate_block(dead_funcs=None):
    """
    Returns a string containing a C block with a volatile-backed opaque predicate.
    Now supports multiple shapes to increase polymorphic entropy and optional
    call graph splitting using dead function names.
    """
    entropy = random.randint(0x10000000, 0x7FFFFFFF)
    entropy2 = random.randint(1000, 9999999)
    magic_cmp = random.randint(0x10000000, 0x7FFFFFFF)

    shape_choice = random.randint(0, 3)

    call_graph_injection = ""
    if dead_funcs:
        func_name = random.choice(dead_funcs)
        arg1_val = random.randint(1, 100)
        call_graph_injection = f"v_sink_{entropy:x} += {func_name}(v_sink_{entropy:x}, {arg1_val});"

    if shape_choice == 0:
        # Shape A: simple if
        return f"""
    {{
        // [mutator: opaque predicate shape A + call graph]
        volatile int v_sink_{entropy:x} = __LINE__;
        if (v_sink_{entropy:x} == {hex(magic_cmp)}) {{
            v_sink_{entropy:x} ^= {hex(entropy2)};
            {call_graph_injection}
            v_sink_{entropy:x} = 0;
        }}
    }}
"""
    elif shape_choice == 1:
        # Shape B: Opaque while loop
        return f"""
    {{
        // [mutator: opaque predicate shape B + call graph]
        volatile int v_sink_{entropy:x} = __LINE__;
        while (v_sink_{entropy:x} == {hex(magic_cmp)}) {{
            v_sink_{entropy:x} ^= {hex(entropy2)};
            {call_graph_injection}
            v_sink_{entropy:x} = 0;
        }}
    }}
"""
    elif shape_choice == 2:
        # Shape C: Dummy switch
        return f"""
    {{
        // [mutator: opaque predicate shape C + call graph]
        volatile int v_sink_{entropy:x} = __LINE__;
        switch(v_sink_{entropy:x}) {{
            case {hex(magic_cmp)}:
                v_sink_{entropy:x} = {hex(entropy2)};
                {call_graph_injection}
                break;
            default:
                break;
        }}
    }}
"""
    else:
        # Shape D: Opaque do-while
        return f"""
    {{
        // [mutator: opaque predicate shape D + call graph]
        volatile int v_sink_{entropy:x} = 0;
        do {{
            v_sink_{entropy:x} = 1;
        }} while (v_sink_{entropy:x} == {hex(magic_cmp)});
    }}
"""

def mutate_c_file(filepath):
    """
    Reads a C file and injects opaque predicates randomly inside function bodies.
    """
    if not filepath.endswith(".c"):
        return

    with open(filepath, 'r') as f:
        lines = f.readlines()

    last_include_idx = -1
    for i, line in enumerate(lines):
        if line.strip().startswith('#include'):
            last_include_idx = i

    dead_funcs_bodies, dead_func_names = generate_dead_functions(random.randint(1, 3))

    mutated_lines = []

    block_stack = []
    brace_depth = 0
    prev_line = ""
    current_statement = ""

    for i, line in enumerate(lines):
        stripped = line.strip()

        clean_line = stripped
        if '//' in clean_line:
            clean_line = clean_line[:clean_line.find('//')].strip()
        clean_line = re.sub(r'".*?"', '', clean_line)

        if stripped and not stripped.startswith('#') and not stripped.startswith('//'):
            current_statement += " " + clean_line

        brace_depth += clean_line.count('{')
        brace_depth -= clean_line.count('}')

        for char in clean_line:
            if char == '{':
                context = prev_line + " " + clean_line
                if any(k in context for k in ['struct', 'enum', 'typedef', 'union', '=']):
                    block_stack.append('DATA')
                else:
                    block_stack.append('CODE')
            elif char == '}':
                if len(block_stack) > 0:
                    block_stack.pop()

        if brace_depth == 0:
            block_stack = []

        is_block_end = clean_line.startswith('}')
        if is_block_end or clean_line.endswith('}') or clean_line.endswith('{'):
            current_statement = ""

        mutated_lines.append(line)

        if i == last_include_idx:
            mutated_lines.append("\n// [mutator: dead function generation]\n")
            mutated_lines.extend(dead_funcs_bodies)
            mutated_lines.append("\n")

        if len(block_stack) > 0 and block_stack[-1] == 'CODE':
            if not stripped.startswith('#') and clean_line.endswith(';'):
                context = prev_line + " " + clean_line
                if not any(k in context for k in ['return', 'break', 'continue', 'goto']):
                    pass

                if not any(k in current_statement for k in ['return', 'break', 'continue', 'goto']):
                    if random.random() < 0.05:
                        indent = line[:len(line) - len(line.lstrip())]
                        predicate_block = get_opaque_predicate_block(dead_funcs=dead_func_names)
                        indented_block = "\n".join([indent + l if l.strip() else l for l in predicate_block.split('\n')])
                        mutated_lines.append(indented_block + "\n")

                current_statement = ""

        if clean_line:
            prev_line = clean_line

    if last_include_idx == -1:
        prepend_lines = ["\n// [mutator: dead function generation]\n"] + dead_funcs_bodies + ["\n"]
        mutated_lines = prepend_lines + mutated_lines

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
            if file.endswith(".c"):
                filepath = os.path.join(root, file)
                mutate_c_file(filepath)
                print(f"[junk_c] Mutated {filepath}")
