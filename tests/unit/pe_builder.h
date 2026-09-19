#ifndef SND_TEST_PE_BUILDER_H
#define SND_TEST_PE_BUILDER_H

#include <sindri/common/buffer.h>
#include <sindri/internal/windows/pe.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#define SND_TEST_PE_DOS_SIZE   0x40u
#define SND_TEST_PE_LFANEW     0x80u
#define SND_TEST_PE_HDR_SIZE   0x200u
#define SND_TEST_PE_SEC_BASE   0x1000u
#define SND_TEST_PE_FILE_ALIGN 0x200u

typedef struct {
    int   is_64bit;        /* PE32+ vs PE32                 */
    int   is_mapped;       /* aligned image vs raw file     */
    int   is_dll;          /* IMAGE_FILE_DLL characteristic*/
    DWORD entry_point_rva; /* 0 for none                    */
} snd_test_pe_spec_t;

typedef struct {
    DWORD rva;
    DWORD size;
} snd_test_pe_region_t;

typedef struct {
    unsigned char *data;
    size_t         size;
    size_t         cap;
} snd_test_mem_t;

typedef struct {
    snd_test_mem_t     mem;
    snd_test_pe_spec_t spec;
    DWORD              section_off; /* buffer offset of the section raw data */
    size_t             section_len; /* bytes written into the section         */
    DWORD              dir_rva[SND_IMAGE_NUMBEROF_DIRECTORY_ENTRIES];
    DWORD              dir_size[SND_IMAGE_NUMBEROF_DIRECTORY_ENTRIES];
} snd_test_pe_t;

/* Allocates and initializes the headers. Returns 0 on success. */
int snd_test_pe_build(snd_test_pe_t *img, const snd_test_pe_spec_t *spec);

/* Appends bytes into the single section. Returns the region descriptor (rva,
 * size) or a zeroed descriptor on failure. */
snd_test_pe_region_t snd_test_pe_add_region(snd_test_pe_t *img, const void *bytes, size_t len);

/* Points arrive exactly at the region RVA with the region's size. */
void snd_test_pe_set_directory(snd_test_pe_t *img, int index, const snd_test_pe_region_t *region);

/* Free-form variant of snd_test_pe_set_directory. */
void snd_test_pe_set_directory_rva(snd_test_pe_t *img, int index, DWORD rva, DWORD size);

/* Patches section header, SizeOfImage / entry point and data directories. */
int snd_test_pe_finalize(snd_test_pe_t *img);

/* Adopts the image for a parser; the parser will NOT free this buffer. */
snd_buffer_t snd_test_pe_as_buffer(snd_test_pe_t *img);

/* Raw mutable access to the image bytes and its length. */
unsigned char *snd_test_pe_data(snd_test_pe_t *img);
size_t         snd_test_pe_size(snd_test_pe_t *img);

void snd_test_pe_free(snd_test_pe_t *img);

/* Little-endian byte writers for composing table blobs. */
void snd_test_put16(unsigned char *p, size_t off, WORD v);
void snd_test_put32(unsigned char *p, size_t off, DWORD v);
void snd_test_put64(unsigned char *p, size_t off, ULONGLONG v);

#ifdef __cplusplus
}
#endif

#endif /* SND_TEST_PE_BUILDER_H */