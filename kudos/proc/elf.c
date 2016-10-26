/*
 * ELF binary format.
 */
#include <arch.h>
#include "proc/elf.h"
#include "lib/libc.h"

/** @name ELF loader.
 *
 * This module contains a function to parse useful information from
 * ELF headers.
 */

/**
 * Parse useful information from a given ELF file into the ELF info
 * structure.
 *
 * @param file The ELF file
 *
 * @param elf Information found in the file is returned in this
 * structure. In case of error this structure may contain arbitrary
 * values.
 *
 * @return 0 on failure, other values indicate success.
 */
int elf_parse_header(elf_info_t *elf, openfile_t file)
{
  Elf32_Ehdr elf_hdr;
  Elf64_Ehdr elf_hdr64;
  Elf32_Phdr program_hdr;
  Elf64_Phdr program_hdr64;
  uint8_t use64 = 0;

  int i;
  int current_position;
  int segs = 0;
#define SEG_RO 1
#define SEG_RW 2

  /* Read the ELF header into the 32 bit, but size of 64 */
  if (vfs_read(file, &elf_hdr64, sizeof(elf_hdr64))
      != sizeof(elf_hdr64)) {
    return -1;
  }

  /* Nasty hack */
  memcopy(sizeof(elf_hdr), &elf_hdr, &elf_hdr64);

  /* Check that the ELF magic is correct. */
  if (from_big_endian32(elf_hdr.e_ident.i) != ELF_MAGIC) {
    return -2;
  }

  /* Not an executable file */
  if (elf_hdr.e_type != ET_EXEC) {
    return -3;
  }

  /* Now, check architecture */
  if (elf_hdr.e_ident.c[EI_CLASS] & ELFCLASS64) {
    use64 = 1;
  }

  /* No program headers */
  if (use64) {
    if (elf_hdr64.e_phnum == 0) {
      return -4;
    }
  }
  else {
    if (elf_hdr.e_phnum == 0) {
      return -4;
    }
  }

  /* Zero the return structure. Uninitialized data is bad(TM). */
  memoryset(elf, 0, sizeof(*elf));

  /* Get the entry point */
  if (use64)
    elf->entry_point = elf_hdr64.e_entry;
  else
    elf->entry_point = elf_hdr.e_entry;

  /* Seek to the program header table */
  if (use64)
    current_position = elf_hdr64.e_phoff;
  else
    current_position = elf_hdr.e_phoff;

  vfs_seek(file, current_position);

  /* Read the program headers. */
  if (use64) {
    for (i = 0; i < elf_hdr64.e_phnum; i++) {
      if (vfs_read(file, &program_hdr64, sizeof(program_hdr64))
          != sizeof(program_hdr64)) {
        return -6;
      }

      switch (program_hdr64.p_type) {
      case PT_NULL:
      case PT_NOTE:
      case PT_PHDR:
        /* These program headers can be ignored */
        break;
      case PT_LOAD:
        /* These are the ones we are looking for */

        /* The RW segment */
        if (program_hdr64.p_flags & PF_W) {
          if (segs & SEG_RW) { /* already have an RW segment*/
            return -7;
          }
          segs |= SEG_RW;

          elf->rw_location = program_hdr64.p_offset;
          elf->rw_size = program_hdr64.p_filesz;
          elf->rw_vaddr = program_hdr64.p_vaddr;
          /* memory size rounded up to the page boundary, in pages */
          elf->rw_pages =
            (program_hdr64.p_memsz + PAGE_SIZE - 1) / PAGE_SIZE;

          /* The RO segment */
        } else {
          if (segs & SEG_RO) { /* already have an RO segment*/
            return -8;
          }
          segs |= SEG_RO;

          elf->ro_location = program_hdr64.p_offset;
          elf->ro_size = program_hdr64.p_filesz;
          elf->ro_vaddr = program_hdr64.p_vaddr;
          /* memory size rounded up to the page boundary, in pages */
          elf->ro_pages =
            (program_hdr64.p_memsz + PAGE_SIZE - 1) / PAGE_SIZE;
        }

        break;
        /* Other program headers indicate an incompatible file *or* a file
           with extra headers.  Just ignore. */
      }

      /* In case the program header size is non-standard: */
      current_position += sizeof(program_hdr64);
      vfs_seek(file, current_position);
    }
  }
  else {
    for (i = 0; i < elf_hdr.e_phnum; i++) {
      if (vfs_read(file, &program_hdr, sizeof(program_hdr))
          != sizeof(program_hdr)) {
        return -6;
      }

      switch (program_hdr.p_type) {
      case PT_NULL:
      case PT_NOTE:
      case PT_PHDR:
        /* These program headers can be ignored */
        break;
      case PT_LOAD:
        /* These are the ones we are looking for */

        /* The RW segment */
        if (program_hdr.p_flags & PF_W) {
          if (segs & SEG_RW) { /* already have an RW segment*/
            return -7;
          }
          segs |= SEG_RW;

          elf->rw_location = program_hdr.p_offset;
          elf->rw_size = program_hdr.p_filesz;
          elf->rw_vaddr = program_hdr.p_vaddr;
          /* memory size rounded up to the page boundary, in pages */
          elf->rw_pages =
            (program_hdr.p_memsz + PAGE_SIZE - 1) / PAGE_SIZE;

          /* The RO segment */
        } else {
          if (segs & SEG_RO) { /* already have an RO segment*/
            return -8;
          }
          segs |= SEG_RO;

          elf->ro_location = program_hdr.p_offset;
          elf->ro_size = program_hdr.p_filesz;
          elf->ro_vaddr = program_hdr.p_vaddr;
          /* memory size rounded up to the page boundary, in pages */
          elf->ro_pages =
            (program_hdr.p_memsz + PAGE_SIZE - 1) / PAGE_SIZE;
        }

        break;
        /* Other program headers indicate an incompatible file *or* a file
           with extra headers.  Just ignore. */
      }

      /* In case the program header size is non-standard: */
      current_position += sizeof(program_hdr);
      vfs_seek(file, current_position);
    }
  }

  /* Make sure either RW or RO segment is present: */
  return (segs > 0);
}

/** @} */
