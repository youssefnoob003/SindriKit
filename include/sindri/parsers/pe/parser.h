#ifndef SND_PARSERS_PE_PARSER_H
#define SND_PARSERS_PE_PARSER_H

#include <sindri/common/buffer.h>
#include <sindri/common/macros.h>
#include <sindri/common/status.h>
#include <windows.h>

SND_BEGIN_EXTERN_C

/**
 * @brief Access utility for clean cross-bitness reads inside the
 * implementation.
 */
#define SND_PE_GET_NT_FIELD(parser, field) ((parser)->is_64bit ? (parser)->nt.nt64->field : (parser)->nt.nt32->field)

/**
 * @warning SAFE HEADER BOOTSTRAP WINDOW & RESIDUAL RISK CAUTIONS
 * This macro establishes a conservative 4KB (one virtual page) size limit.
 * It serves as a secure bootstrap boundary for parsing the initial PE headers
 * of in-memory or system DLLs where the total virtual size is not yet verified.
 *
 * Enforcing a 4KB window guarantees that if the target headers are
 * intentionally truncated, malformed, or weaponized, the parser will fail
 * cleanly inside the first page instead of blindly wandering out-of-bounds into
 * unmapped memory.
 *
 * ! CRITICAL MEMORY FOOTPRINT REQUISITE:
 * This approach assumes the underlying memory region has AT LEAST 4KB of valid
 * backing allocation. If the source buffer was allocated with exact bounds
 * smaller than 4KB (e.g., exactly 3KB) and a malformed header field references
 * an offset within the unallocated window (e.g., 3.5KB), this 4KB boundary
 * check will erroneously pass, triggering a physical Out-of-Bounds read.
 *
 * ! TRUST PARADOX (SizeOfImage Assumption):
 * By implementing this bootstrap, the loader is forced to expand its tracking
 * boundaries by trusting the payload's own `OptionalHeader.SizeOfImage` field.
 * If an adversary feeds the loader an image with a fake, hyper-inflated size
 * field, the parser will adopt it blindly. You must ensure downstream consumers
 * of this parser handle an oversized virtual layout gracefully.
 *
 * ! CRITICAL LIFECYCLE MANDATE:
 * Immediately after `snd_pe_parse` succeeds, you MUST extract the actual
 * `OptionalHeader.SizeOfImage` and update the parser context's buffer size
 * to encompass the full image layout. Leaving it clamped at 0x1000 will cause
 * all subsequent directory parsing (imports, exports, relocations) to be
 * rejected as out-of-bounds.
 */
#define SND_SYS_DLL_SIZE_DEFAULT 0x1000

/**
 * @brief Context structure for the PE parser.
 * @note Pointers inside this structure (dos, nt, section_head) point directly
 * into the memory space backed by the `source` buffer. They do not own memory
 * and their lifecycles are tied entirely to the validity of the underlying
 * `source`.
 */
typedef struct {
    snd_buffer_t source; // Backing buffer containing the raw or mapped PE data.

    PIMAGE_DOS_HEADER dos;

    union {
        PIMAGE_NT_HEADERS32 nt32;
        PIMAGE_NT_HEADERS64 nt64;
    } nt;

    PIMAGE_SECTION_HEADER
    section_head; // Points to the first element in the section header array.

    /**
     * @brief Points to the COFF String Table (used for long section names).
     * @note This is NULL for mapped/loaded images, as the string table is rarely
     * copied to virtual memory space during manual mapping.
     */
    BYTE *string_table;

    BOOL is_64bit;
    BOOL is_dll;

    /**
     * @brief Tracks the structural state of the `source` buffer.
     * TRUE if the payload is aligned to its virtual memory layout (PE Sections
     * mapped to VirtualAddresses). FALSE if the payload is in its raw disk layout
     * (PE Sections mapped to PointerToRawData).
     */
    BOOL is_mapped;

    DWORD sections_count;
    DWORD imports_rva;
    DWORD import_size;
} snd_pe_parser_t;

/**
 * @brief Parses a raw or mapped PE file buffer into a parsed PE representation.
 *
 * This function serves as the primary entry point for payload validation. It
 * executes the initial bootstrap parse, validating the Magic DOS signature (MZ)
 * and the NT headers signature (PE). Based on the headers, it dynamically sets
 * the context bitness (32-bit vs 64-bit) and maps internal pointers differently
 * depending on whether the source buffer is formatted as a raw disk file or an
 * already aligned virtual image.
 *
 * @note This function performs **zero-copy parsing**. The pointers populated
 * inside the `parser` context point directly into the memory space owned by the
 * input `buf`. No heap allocations are performed. Consequently, the lifecycle
 * of the populated `parser` structure is strictly bound to the lifetime and
 * validity of the underlying `buf`.
 *
 * @param buf Raw buffer containing the PE file data.
 * @param is_mapped TRUE if the buffer represents an already virtually aligned
 * image, FALSE if it is a raw disk file layout.
 * @param parser Pointer to the parser context to populate.
 * @return SND_OK on success, or a contextual error status.
 */
snd_status_t snd_pe_parse(const snd_buffer_t *buf, BOOL is_mapped, snd_pe_parser_t *parser);

SND_END_EXTERN_C

#endif // SND_PARSERS_PE_PARSER_H
