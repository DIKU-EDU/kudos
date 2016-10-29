#ifndef KUDOS_PROC_X86_64_ELF_INFO_H
#define KUDOS_PROC_X86_64_ELF_INFO_H

typedef struct {
    uint64_t entry_point; /* Entry point for the code */

    uint64_t ro_location; /* Location of RO segment in the file */
    uint64_t ro_size;     /* Size of RO segment in the file */
    uint64_t ro_pages;    /* Pages needed by the RO segment */
    uint64_t ro_vaddr;    /* Virtual address of the RO segment */

    uint64_t rw_location; /* Location of RW segment in the file */
    uint64_t rw_size;     /* Size of RW segment in the file */
    uint64_t rw_pages;    /* Pages needed by the RW segment */
    uint64_t rw_vaddr;    /* Virtual address of the RW segment */
} elf_info_t;

#endif // KUDOS_PROC_X86_64_ELF_INFO_H
