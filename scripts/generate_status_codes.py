import os
import re

INCLUDE_DIR = 'include/sindri'
FACILITY_FILE = os.path.join(INCLUDE_DIR, 'status', 'facility.h')
OUTPUT_FILE = os.path.join('docs', 'status_codes.md')

def parse_facilities():
    facilities = {}
    current_val = 0
    with open(FACILITY_FILE, 'r') as f:
        in_enum = False
        for line in f:
            line = line.strip()
            if 'enum _SND_FACILITY_ID' in line:
                in_enum = True
                continue
            if in_enum and '}' in line:
                break
            if in_enum and line.startswith('SND_FACILITY_'):
                parts = line.split('=')
                name = parts[0].strip().rstrip(',')
                if len(parts) > 1:
                    val_str = parts[1].strip().rstrip(',')
                    current_val = int(val_str, 0)
                facilities[name] = current_val
                current_val += 1
    return facilities

def generate_status_codes(facilities):
    status_codes = []
    make_status_re = re.compile(r'SND_MAKE_STATUS\s*\(\s*([^,]+?)\s*,\s*([^)]+?)\s*\)')

    for root, _, files in os.walk(INCLUDE_DIR):
        for file in sorted(files):
            if not file.endswith('.h'):
                continue

            filepath = os.path.join(root, file)
            with open(filepath, 'r') as f:
                content = f.read()

            lines = content.split('\n')
            in_enum = False
            current_facility = None
            current_val = 0

            for line in lines:
                stripped = line.strip()

                if 'enum' in stripped and not stripped.startswith('//'):
                    in_enum = True
                    continue
                if in_enum and '}' in stripped:
                    in_enum = False
                    current_facility = None
                    continue

                if not in_enum:
                    continue

                # Skip comments and blank lines
                if not stripped or stripped.startswith('//') or stripped.startswith('/*'):
                    continue

                # Extract the enum entry name (strip trailing comma)
                entry_name = stripped.split('=')[0].strip().rstrip(',')
                if not entry_name.startswith('SND_'):
                    continue

                match = make_status_re.search(stripped)
                if match:
                    fac_name = match.group(1).strip()
                    code_val = int(match.group(2).strip(), 0)
                    fac_val = facilities.get(fac_name, 0)
                    current_val = (fac_val << 16) | (code_val & 0xFFFF)
                    current_facility = fac_name
                elif '=' in stripped:
                    # Explicit value without SND_MAKE_STATUS (e.g. SND_SUCCESS = 0)
                    val_str = stripped.split('=')[1].strip().rstrip(',').strip()
                    try:
                        current_val = int(val_str, 0)
                        current_facility = 'SND_FACILITY_GENERIC'
                    except ValueError:
                        pass
                else:
                    # Auto-increment
                    current_val += 1

                # Only emit SND_STATUS_* entries (not SND_SUCCESS, SND_ERROR_GENERIC, etc.)
                if not entry_name.startswith('SND_STATUS_'):
                    continue

                fac_display = current_facility if current_facility else 'SND_FACILITY_GENERIC'
                hex_val = f"0x{current_val:08X}"
                dec_val = str(current_val)
                status_codes.append((hex_val, dec_val, entry_name, fac_display))

    status_codes.sort(key=lambda x: int(x[0], 16))
    return status_codes

def main():
    facilities = parse_facilities()
    status_codes = generate_status_codes(facilities)

    with open(OUTPUT_FILE, 'w') as f:
        f.write("# SindriKit Status Codes\n\n")
        f.write("> [!NOTE]\n")
        f.write("> This file is **automatically generated** at CMake configuration time by\n")
        f.write("> `scripts/generate_status_codes.py`. Do not edit it manually.\n\n")
        f.write("| Hex Code | Decimal | Status Name | Facility |\n")
        f.write("|----------|---------|-------------|----------|\n")
        for hex_val, dec_val, name, fac in status_codes:
            f.write(f"| `{hex_val}` | `{dec_val}` | `{name}` | `{fac}` |\n")

if __name__ == '__main__':
    main()
