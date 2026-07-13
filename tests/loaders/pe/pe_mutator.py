"""
PE Mutation Engine for SindriKit loader stress-testing.

Applies targeted mutations to valid PE files to produce variants that
exercise the pe loader's parser and loading pipeline. Mutations
fall into two categories:

  - **Benign Edge-Cases**: The PE layout is unusual or hyper-optimized, but
    Windows can still load and run it. The custom loader MUST adapt and succeed.
  - **Breaking Stressors**: The PE is structurally corrupted. The loader's
    parser must detect this boundaries violation and reject it gracefully (NO CRASHES).

Requires: pip install pefile
"""

import os
import struct
from dataclasses import dataclass
from typing import Callable, List

try:
    import pefile
except ImportError:
    pefile = None  # Checked at runtime by the test runner.


class MutationError(Exception):
    """Raised when a mutation cannot be applied."""


# ── Safety Boundary Helpers ──────────────────────────────────────────────────


def _read(path: str) -> bytearray:
    if not os.path.exists(path):
        raise MutationError(f"Source file does not exist: {path}")
    if os.path.getsize(path) < 0x40:
        raise MutationError(
            f"Source file too small to contain a valid PE DOS Header: {path}"
        )
    with open(path, "rb") as f:
        return bytearray(f.read())


def _write(path: str, data: bytes) -> None:
    with open(path, "wb") as f:
        f.write(data)


def _pe_modify(src: str, dst: str, modifier: Callable) -> None:
    """Load a PE with safety checks, apply structural modifier, write the result."""
    if pefile is None:
        raise MutationError("pefile is not installed in the execution environment")

    try:
        pe = pefile.PE(src)
    except Exception as e:
        raise MutationError(f"pefile failed to parse baseline image: {e}")

    try:
        modifier(pe)
        _write(dst, pe.write())
    except Exception as e:
        raise MutationError(f"Modification execution error: {e}")
    finally:
        pe.close()


# ── Benign Edge-Cases (Windows Loads Them, Your Loader Must Too) ─────────────


def unaligned_sections(src: str, dst: str) -> None:
    """Force equal Section and File alignments (0x200).
    Commonly seen in hyper-optimized or packed malware binaries. If your loader
    hardcodes assumptions about 4KB (0x1000) page alignments, it will fail to map this.
    """

    def _apply(pe):
        if hasattr(pe, "OPTIONAL_HEADER") and pe.OPTIONAL_HEADER:
            pe.OPTIONAL_HEADER.SectionAlignment = 0x200
            pe.OPTIONAL_HEADER.FileAlignment = 0x200

    _pe_modify(src, dst, _apply)


def strip_relocs_from_exe(src: str, dst: str) -> None:
    """Strip the relocation directory entirely and set IMAGE_FILE_RELOCS_STRIPPED.
    Windows can run EXEs without a .reloc section if they manage to land at their
    preferred ImageBase. Your loader shouldn't aggressively demand relocations for EXEs.
    """

    def _apply(pe):
        pe.FILE_HEADER.Characteristics |= 0x0001  # IMAGE_FILE_RELOCS_STRIPPED
        if len(pe.OPTIONAL_HEADER.DATA_DIRECTORY) > 5:
            pe.OPTIONAL_HEADER.DATA_DIRECTORY[5].VirtualAddress = 0
            pe.OPTIONAL_HEADER.DATA_DIRECTORY[5].Size = 0

    _pe_modify(src, dst, _apply)


def garbage_dos_stub(src: str, dst: str) -> None:
    """Overwrite the DOS stub with deterministic garbage.
    Only bytes 0-1 (MZ) and 0x3C-0x3F (e_lfanew) matter. Tests if your loader's parser
    correctly seeks using offsets instead of reading linearly through the DOS stub.
    """
    data = _read(src)
    e_lfanew = struct.unpack_from("<I", data, 0x3C)[0]

    upper_bound = min(e_lfanew, len(data))
    for i in range(2, upper_bound):
        if 0x3C <= i <= 0x3F:
            continue  # Protect the e_lfanew pointer
        data[i] = (i * 7 + 0xAB) & 0xFF
    _write(dst, data)


def append_overlay(src: str, dst: str) -> None:
    """Append 4 KB of junk after the last section.
    Legitimate installers and SFX archives append data beyond declared section boundaries.
    The loader must map the image based strictly on headers, ignoring raw trailing bytes.
    """
    data = _read(src)
    overlay = bytes([(i * 13 + 0x37) & 0xFF for i in range(4096)])
    _write(dst, data + overlay)


def null_section_names(src: str, dst: str) -> None:
    """Zero out every section name string.
    Section names are advisory and non-functional. If your loader matches strings
    un-safely or logs names via unchecked printFs, this exposes string boundaries bugs.
    """

    def _apply(pe):
        for section in pe.sections:
            section.Name = b"\x00" * 8

    _pe_modify(src, dst, _apply)


# ── Breaking Stressors (Parser Verification, to be Rejected) ──────────────────


def corrupt_reloc_block_size(src: str, dst: str) -> None:
    """Set the first Base Relocation block's SizeOfBlock to 0xFFFFFFFF.
    If your loader parses relocations using a `while` loop or basic pointer addition
    without tracking boundaries against the data directory's size, this triggers a
    catastrophic 0xC0000005 pointer overflow loop.
    """

    def _apply(pe):
        if len(pe.OPTIONAL_HEADER.DATA_DIRECTORY) > 5:
            dir_reloc = pe.OPTIONAL_HEADER.DATA_DIRECTORY[5]
            if dir_reloc.VirtualAddress != 0 and dir_reloc.Size > 0:
                offset = pe.get_offset_from_rva(dir_reloc.VirtualAddress)
                if offset and (offset + 8 <= len(pe.__data__)):
                    # Use pefile's built-in memory-safe modifier instead of direct mmap assignment
                    pe.set_bytes_at_offset(offset + 4, b"\xff\xff\xff\xff")

    _pe_modify(src, dst, _apply)


def massive_size_of_headers(src: str, dst: str) -> None:
    """Set SizeOfHeaders to 0xFFFFFFFF.
    Verifies your loader checks limits before executing:
    `memcpy(allocated_memory, raw_buffer, nt_headers->OptionalHeader.SizeOfHeaders);`
    """

    def _apply(pe):
        if hasattr(pe, "OPTIONAL_HEADER") and pe.OPTIONAL_HEADER:
            pe.OPTIONAL_HEADER.SizeOfHeaders = 0xFFFFFFFF

    _pe_modify(src, dst, _apply)


def section_truncated_raw_data(src: str, dst: str) -> None:
    """Inflate a section's SizeOfRawData beyond the physical boundaries of the file.
    Tests if the loader maps sections using `SizeOfRawData` blindly without ensuring
    the backing source file buffer actually holds those bytes.
    """

    def _apply(pe):
        if pe.sections:
            pe.sections[0].SizeOfRawData = 0x7FFFFFFF

    _pe_modify(src, dst, _apply)


def section_integer_overflow(src: str, dst: str) -> None:
    """Set a section's VirtualSize and SizeOfRawData to 0xFFFFFFFF.
    Exercises whether allocation arithmetic formulas inside your engine, such as
    `NextSectionVA = CurrentSectionVA + ALIGN(VirtualSize, Alignment)`, can be exploited
    via integer wrap-arounds.
    """

    def _apply(pe):
        if pe.sections:
            pe.sections[0].SizeOfRawData = 0xFFFFFFFF
            pe.sections[0].Misc_VirtualSize = 0xFFFFFFFF

    _pe_modify(src, dst, _apply)


def section_overlap_corruption(src: str, dst: str) -> None:
    """Collapse all section file offsets back down to zero.
    Forces headers and execution directories to overlap within memory mappings.
    Catches file alignment normalization bugs inside custom tracking systems.
    """

    def _apply(pe):
        for section in pe.sections:
            section.PointerToRawData = 0x00000000

    _pe_modify(src, dst, _apply)


def corrupt_pe_signature(src: str, dst: str) -> None:
    """Replace the PE\\0\\0 signature with invalid garbage."""
    data = _read(src)
    e_lfanew = struct.unpack_from("<I", data, 0x3C)[0]
    if e_lfanew + 4 <= len(data):
        data[e_lfanew : e_lfanew + 4] = b"\xde\xad\xbe\xef"
    else:
        raise MutationError("Parsed e_lfanew boundary points past physical EOF")
    _write(dst, data)


def invalid_e_lfanew(src: str, dst: str) -> None:
    """Point e_lfanew 1 MB past the end of the file buffer."""
    data = _read(src)
    struct.pack_into("<I", data, 0x3C, len(data) + 0x100000)
    _write(dst, data)


def zero_size_of_image(src: str, dst: str) -> None:
    """Set SizeOfImage to 0. Processing allocation maps must be rejected."""

    def _apply(pe):
        if hasattr(pe, "OPTIONAL_HEADER") and pe.OPTIONAL_HEADER:
            pe.OPTIONAL_HEADER.SizeOfImage = 0

    _pe_modify(src, dst, _apply)


def corrupt_import_rva(src: str, dst: str) -> None:
    """Point the import directory directory entry to an unmapped RVA address (0xDEAD0000)."""

    def _apply(pe):
        if hasattr(pe, "OPTIONAL_HEADER") and pe.OPTIONAL_HEADER:
            if len(pe.OPTIONAL_HEADER.DATA_DIRECTORY) > 1:
                pe.OPTIONAL_HEADER.DATA_DIRECTORY[1].VirtualAddress = 0xDEAD0000

    _pe_modify(src, dst, _apply)


def corrupt_export_rva(src: str, dst: str) -> None:
    """Point the export directory entry to an unmapped RVA address."""

    def _apply(pe):
        if hasattr(pe, "OPTIONAL_HEADER") and pe.OPTIONAL_HEADER:
            if len(pe.OPTIONAL_HEADER.DATA_DIRECTORY) > 0:
                pe.OPTIONAL_HEADER.DATA_DIRECTORY[0].VirtualAddress = 0xDEAD0000

    _pe_modify(src, dst, _apply)


def huge_sections_count(src: str, dst: str) -> None:
    """Set NumberOfSections to 0xFFFF to trigger parsing bounds protection."""
    data = _read(src)
    e_lfanew = struct.unpack_from("<I", data, 0x3C)[0]
    offset = e_lfanew + 4 + 2
    if offset + 2 <= len(data):
        struct.pack_into("<H", data, offset, 0xFFFF)
    else:
        raise MutationError("PE header offset structure mismatch")
    _write(dst, data)


def unterminated_imports(src: str, dst: str) -> None:
    """Overwrite the null-terminating IMAGE_IMPORT_DESCRIPTOR with garbage.
    Catches loaders that iterate imports with a `while(entry->Name != 0)` loop
    without checking if they have exceeded the Import Directory's boundary.
    """

    def _apply(pe):
        if hasattr(pe, "DIRECTORY_ENTRY_IMPORT") and pe.DIRECTORY_ENTRY_IMPORT:
            import_dir_rva = pe.OPTIONAL_HEADER.DATA_DIRECTORY[1].VirtualAddress
            offset = pe.get_offset_from_rva(import_dir_rva)
            if offset:
                # Calculate where the null terminator descriptor is located.
                # Each IMAGE_IMPORT_DESCRIPTOR is exactly 20 bytes.
                num_imports = len(pe.DIRECTORY_ENTRY_IMPORT)
                null_desc_offset = offset + (num_imports * 20)
                if null_desc_offset + 20 <= len(pe.__data__):
                    # Overwrite the null terminator using safe memory map API
                    pe.set_bytes_at_offset(null_desc_offset, b"\xff" * 20)

    _pe_modify(src, dst, _apply)


def massive_export_count(src: str, dst: str) -> None:
    """Set NumberOfFunctions and NumberOfNames in the Export Directory to 0xFFFFFFFF.
    Catches loaders that try to allocate tracking arrays based on these values
    or iterate over them without bounds checking.
    """

    def _apply(pe):
        if hasattr(pe, "DIRECTORY_ENTRY_EXPORT") and pe.DIRECTORY_ENTRY_EXPORT:
            export_dir_rva = pe.OPTIONAL_HEADER.DATA_DIRECTORY[0].VirtualAddress
            offset = pe.get_offset_from_rva(export_dir_rva)
            if offset and (offset + 40 <= len(pe.__data__)):
                # NumberOfFunctions is at offset + 20
                # NumberOfNames is at offset + 24
                pe.set_bytes_at_offset(offset + 20, b"\xff\xff\xff\xff")
                pe.set_bytes_at_offset(offset + 24, b"\xff\xff\xff\xff")

    _pe_modify(src, dst, _apply)


def oob_entry_point(src: str, dst: str) -> None:
    """Point AddressOfEntryPoint outside the mapped SizeOfImage boundaries.
    The loader must validate that the entry point RVA lands inside a valid,
    mapped executable section before attempting to pass execution flow.
    """

    def _apply(pe):
        if hasattr(pe, "OPTIONAL_HEADER") and pe.OPTIONAL_HEADER:
            # Set EP 4KB past the end of the entire loaded image
            pe.OPTIONAL_HEADER.AddressOfEntryPoint = (
                pe.OPTIONAL_HEADER.SizeOfImage + 0x1000
            )

    _pe_modify(src, dst, _apply)


# ── Mutation Registry ───────────────────────────────────────────────────────


@dataclass
class Mutation:
    """Describes a single PE mutation configuration structure."""

    name: str  # Short string identifier
    description: str  # Human-readable status label
    apply: Callable  # Target modification logic function pointer
    expect_loadable: (
        bool  # True = Matrix expects operational success; False = Graceful rejection
    )
    applies_to: str = "both"


BENIGN_MUTATIONS: List[Mutation] = [
    Mutation(
        "unaligned_sec",
        "Unaligned sections (0x200 alignment)",
        unaligned_sections,
        True,
    ),
    Mutation(
        "stripped_relocs",
        "Stripped reloc directory (.reloc removed)",
        strip_relocs_from_exe,
        True,
        "exe",
    ),
    Mutation("garbage_dos_stub", "Garbage DOS stub overwrites", garbage_dos_stub, True),
    Mutation(
        "append_overlay", "4KB unmapped trailing overlay appended", append_overlay, True
    ),
    Mutation(
        "null_sec_names", "Zeroed programmatic section names", null_section_names, True
    ),
]

BREAKING_MUTATIONS: List[Mutation] = [
    Mutation(
        "oob_reloc_size",
        "OOB Relocation Block Size (0xFFFFFFFF)",
        corrupt_reloc_block_size,
        False,
    ),
    Mutation(
        "massive_headers",
        "Massive SizeOfHeaders (0xFFFFFFFF)",
        massive_size_of_headers,
        False,
    ),
    Mutation(
        "truncated_sec",
        "Truncated Section Raw Boundary",
        section_truncated_raw_data,
        False,
    ),
    Mutation(
        "int_overflow",
        "Section Allocation Integer Overflow",
        section_integer_overflow,
        False,
    ),
    Mutation(
        "overlap_sections",
        "Collapsing Section Offset Intersections",
        section_overlap_corruption,
        False,
    ),
    Mutation(
        "corrupt_pe_sig",
        "Corrupted PE magic identification bytes",
        corrupt_pe_signature,
        False,
    ),
    Mutation(
        "bad_e_lfanew", "Invalid out-of-bounds e_lfanew values", invalid_e_lfanew, False
    ),
    Mutation(
        "zero_imgsize", "SizeOfImage set directly to zero", zero_size_of_image, False
    ),
    Mutation(
        "bad_import",
        "Import Directory maps to unallocated RVA",
        corrupt_import_rva,
        False,
    ),
    Mutation(
        "bad_export",
        "Export Directory maps to unallocated RVA",
        corrupt_export_rva,
        False,
        "dll",
    ),
    Mutation(
        "huge_sections",
        "NumberOfSections set to maximum 0xFFFF",
        huge_sections_count,
        False,
    ),
    Mutation(
        "unterminated_imports",
        "Missing null terminator in Import array",
        unterminated_imports,
        False,
    ),
    Mutation(
        "massive_exports",
        "Export NumberOfFunctions = 0xFFFFFFFF",
        massive_export_count,
        False,
        "dll",
    ),
    Mutation(
        "oob_entry_point", "EntryPoint RVA outside SizeOfImage", oob_entry_point, False
    ),
]

ALL_MUTATIONS = BENIGN_MUTATIONS + BREAKING_MUTATIONS


def mutate_pe(src_path: str, mutation: Mutation, output_dir: str) -> str:
    """Apply a specified mutation parameters block to a base file,
    writing the final asset output into target folders.
    """
    base = os.path.basename(src_path)
    name, ext = os.path.splitext(base)
    dst_path = os.path.join(output_dir, f"{name}_{mutation.name}{ext}")

    try:
        mutation.apply(src_path, dst_path)
    except Exception as e:
        if os.path.exists(dst_path):
            try:
                os.remove(dst_path)
            except OSError:
                pass
        raise MutationError(
            f"Failed to apply '{mutation.name}' mutation matrix to {base}: {e}"
        ) from e
    return dst_path
