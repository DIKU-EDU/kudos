/*
 * ELF binary format.
 */

#ifndef KUDOS_PROC_ELF_H
#define KUDOS_PROC_ELF_H

#include "lib/types.h"
#include "fs/vfs.h"
#include <elf_info.h>


/* Return data type for the ELF header parser. This structure
 * contains the information about RO and RW segments and the entry point.
 *
 * In theory, there are four kinds of segments that could be in the
 * executable: the program code (.text), read-only data (.rodata),
 * read-write initialized data (.data) and read-write uninitialized
 * data (.bss, not stored in the binary). However, the MIPS memory
 * architecture can only differentiate between RO and RW pages. Thus
 * there are only two segment types in practice: RO, which includes
 * program code and read-only data, and RW, which includes all
 * writable data. The GNU ELF ld will do this grouping into program
 * segments even if not specifically instructed so in the linker
 * script. This is why this data type only needs two segments.
 *
 * NB: since the RW segment includes bss, pages needed may (of course)
 * be much larger than indicated by the size field.
 */

int elf_parse_header(elf_info_t *elf, openfile_t file);


/* These are directly from the TIS/ELF 1.2 specification */

#define ET_EXEC 2
#define EM_MIPS 8

#define EI_CLASS 4
#define EI_DATA 5
#define EI_VERSION 6

#define ELFCLASS32 1
#define ELFCLASS64 2
#define ELFDATA2MSB 2
#define EV_CURRENT 1

/* ELF magic, "\177ELF", big-endian format */
#define ELF_MAGIC 0x7f454c46

/* ELF file header. The structure fields are aligned by data type
 * size, so the packed-attribute is not really necessary. 
 */
#define EI_NIDENT 16
typedef struct {
    /* ELF identification data */
    union {
        unsigned char c[EI_NIDENT];
        uint32_t i;
    } e_ident                        __attribute__ ((packed));
    /* Object file type (executable, relocatable...) */
    uint16_t e_type                  __attribute__ ((packed));
    /* Machine architecture (we want MIPS) */
    uint16_t e_machine               __attribute__ ((packed));
    /* ELF version */
    uint32_t e_version               __attribute__ ((packed));
    /* Program entry point virtual address */
    uint32_t e_entry                 __attribute__ ((packed));
    /* Program header table's file offset */
    uint32_t e_phoff                 __attribute__ ((packed));
    /* Section header table's file offset */
    uint32_t e_shoff                 __attribute__ ((packed));
    /* Processor specific flags */
    uint32_t e_flags                 __attribute__ ((packed));
    /* ELF header size in bytes */
    uint16_t e_ehsize                __attribute__ ((packed));
    /* Program header entry size */
    uint16_t e_phentsize             __attribute__ ((packed));
    /* Number of program headers */
    uint16_t e_phnum                 __attribute__ ((packed));
    /* Section header entry size */
    uint16_t e_shentsize             __attribute__ ((packed));
    /* Number of section headers */
    uint16_t e_shnum                 __attribute__ ((packed));
    /* Section header index of the section name string table */
    uint16_t e_shstrndx              __attribute__ ((packed));
} Elf32_Ehdr;

typedef struct {
    /* ELF identification data */
    unsigned char e_ident[EI_NIDENT];
    /* Object file type (executable, relocatable...) */
    uint16_t e_type                  __attribute__ ((packed));
    /* Machine architecture (we want MIPS) */
    uint16_t e_machine               __attribute__ ((packed));
    /* ELF version */
    uint32_t e_version               __attribute__ ((packed));
    /* Program entry point virtual address */
    uint64_t e_entry                 __attribute__ ((packed));
    /* Program header table's file offset */
    uint64_t e_phoff                 __attribute__ ((packed));
    /* Section header table's file offset */
    uint64_t e_shoff                 __attribute__ ((packed));
    /* Processor specific flags */
    uint32_t e_flags                 __attribute__ ((packed));
    /* ELF header size in bytes */
    uint16_t e_ehsize                __attribute__ ((packed));
    /* Program header entry size */
    uint16_t e_phentsize             __attribute__ ((packed));
    /* Number of program headers */
    uint16_t e_phnum                 __attribute__ ((packed));
    /* Section header entry size */
    uint16_t e_shentsize             __attribute__ ((packed));
    /* Number of section headers */
    uint16_t e_shnum                 __attribute__ ((packed));
    /* Section header index of the section name string table */
    uint16_t e_shstrndx              __attribute__ ((packed));
} Elf64_Ehdr;

/* Segment types */
#define PT_NULL 0
#define PT_LOAD 1
#define PT_DYNAMIC 2
#define PT_INTERP 3
#define PT_NOTE 4
#define PT_SHLIB 5
#define PT_PHDR 6
#define PT_LOPROC 0x70000000
#define PT_HIPROC 0x7fffffff

/* Segment flags */
#define PF_X 0x1
#define PF_W 0x2
#define PF_R 0x4

/* ELF program header */
typedef struct {
    /* Segment type */
    uint32_t p_type   __attribute__ ((packed));
    /* Location in the file */
    uint32_t p_offset __attribute__ ((packed));
    /* Virtual address */
    uint32_t p_vaddr  __attribute__ ((packed));
    /* Physical address (not used on many platforms) */
    uint32_t p_paddr  __attribute__ ((packed));
    /* Number of bytes stored in the file */
    uint32_t p_filesz __attribute__ ((packed));
    /* Number of bytes this segment occupies in memory */
    uint32_t p_memsz  __attribute__ ((packed));
    /* Segment flags */
    uint32_t p_flags  __attribute__ ((packed));
    /* Alignment in file and memory */
    uint32_t p_align  __attribute__ ((packed));
} Elf32_Phdr;

/* ELF program header */
typedef struct {
    /* Segment type */
    uint32_t p_type   __attribute__ ((packed));
    /* Location in the file */
    uint32_t p_flags __attribute__ ((packed));
    /* Virtual address */
    uint64_t p_offset  __attribute__ ((packed));
    /* Physical address (not used on many platforms) */
    uint64_t p_vaddr  __attribute__ ((packed));
    /* Number of bytes stored in the file */
    uint64_t p_paddr __attribute__ ((packed));
    /* Number of bytes this segment occupies in memory */
    uint64_t p_filesz  __attribute__ ((packed));
    /* Segment flags */
    uint64_t p_memsz  __attribute__ ((packed));
    /* Alignment in file and memory */
    uint64_t p_align  __attribute__ ((packed));
} Elf64_Phdr;

#endif // KUDOS_PROC_ELF_H
