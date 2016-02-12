/*
 * Interrupt handling
 */

#include "kernel/mips32/asm.h"

        .text
        .align  2

# void _tlb_get_exception_state(tlb_exception_state_t *state);
#
# Fetch CP0 register BadVAddr to state->badvaddr, field VPN2 from register
# EntryHi to state->badvpn2 and field ASID from EntryHi to state->asid
#
        .globl  _tlb_get_exception_state
        .ent    _tlb_get_exception_state
_tlb_get_exception_state:
  # Get BadVAddr and VPN2+ASID (in EntryHi) from CP0 and insert
  # into the struct members.
  mfc0  t0, BadVAd, 0
  sw  t0, 0(a0)
  mfc0  t0, EntrHi, 0
  srl  t1, t0, 13
  sw  t1, 4(a0)
  andi  t0, t0, 0x00ff
  sw  t0, 8(a0)
        j ra
        .end    _tlb_get_exception_state



# void _tlb_set_asid(uint32_t asid);
#
# Set the ASID field of the CP0 EntryHi register to asid (8 lowest bits)
#
        .globl  _tlb_set_asid
        .ent    _tlb_set_asid
_tlb_set_asid:
  mfc0  t0, EntrHi, 0
  li  t1, 0xffffe000
  and  t0, t0, t1
  andi  a0, a0, 0x00ff
  or  t0, t0, a0
  mtc0  t0, EntrHi, 0
        j ra
        .end    _tlb_set_asid



# uint32_t _tlb_get_maxindex(void);
#
# Returns the maximum row number (index) possible in the TLB.
# In other words, number of rows - 1.
#
        .globl  _tlb_get_maxindex
        .ent    _tlb_get_maxindex
_tlb_get_maxindex:
  mfc0  v0, Conf, 1
  srl  v0, v0, 25
  andi  v0, v0, 0x003f
        j ra
        .end    _tlb_get_maxindex



# int _tlb_probe(tlb_entry_t *entry);
#
# Probe an entry matching the VPN2 and ASID fields in entry from the TLB.
# The index of the matching entry is returned, or a negative value if
# there was no match.
#
        .globl  _tlb_probe
        .ent    _tlb_probe
_tlb_probe:
  # Fill EntryHi with VPN2 and ASID
  lw  t0, 0(a0)
  mtc0  t0, EntrHi, 0
  # and probe
  tlbp
  mfc0  v0, Index, 0
        j ra
        .end    _tlb_probe



# int _tlb_read(tlb_entry_t *entries, uint32_t index, uint32_t num);
#
# Read 'num' entries from the TLB, starting from the TLB entry 'index'. The
# entries are stored into the table 'entries'. Returns the number of
# entries actually read.
#
        .globl  _tlb_read
        .ent    _tlb_read
_tlb_read:
  # Get the maximum TLB entry index
  mfc0  t1, Conf, 1
  srl  t1, t1, 25
  andi  t1, t1, 0x0000003f
  li  v0, 0    # retval = 0

tlb_read_one:
  subu  t2, t1, a1    # Check index <= TLB_MAX
  bltz  t2, tlb_read_end
  blez  a2, tlb_read_end  # Check num > 0

  # Read TLB[index] and store to *entries
  mtc0  a1, Index, 0
  tlbr
  mfc0  t0, EntrHi, 0
  sw  t0, 0(a0)
  mfc0  t0, EntLo0, 0
  sw  t0, 4(a0)
  mfc0  t0, EntLo1, 0
  sw  t0, 8(a0)

  addi  a0, a0, 12  # entries++
  addi  v0, v0, 1  # retval++
  addi  a2, a2, -1  # num--
  addi  a1, a1, 1  # index++
  j tlb_read_one

tlb_read_end:
        j ra
        .end    _tlb_read



# int _tlb_write(tlb_entry_t *entries, uint32_t index, uint32_t num);
#
# Write 'num' entries into the TLB, starting from the TLB entry 'index'. The
# entries are read from the table 'entries'. Returns the number of
# entries actually written.
#
        .globl  _tlb_write
        .ent    _tlb_write
_tlb_write:
  # Get the maximum TLB entry index
  mfc0  t1, Conf, 1
  srl  t1, t1, 25
  andi  t1, t1, 0x0000003f
  li  v0, 0    # retval = 0

tlb_write_one:
  subu  t2, t1, a1    # Check index <= TLB_MAX
  bltz  t2, tlb_write_end
  blez  a2, tlb_write_end  # Check num > 0

  # Write *entries to TLB[index]
  mtc0  a1, Index, 0
  lw  t0, 0(a0)
  mtc0  t0, EntrHi, 0
  lw  t0, 4(a0)
  mtc0  t0, EntLo0, 0
  lw  t0, 8(a0)
  mtc0  t0, EntLo1, 0
  tlbwi

  addi  a0, a0, 12  # entries++
  addi  v0, v0, 1  # retval++
  addi  a2, a2, -1  # num--
  addi  a1, a1, 1  # index++
  j tlb_write_one

tlb_write_end:
        j ra
        .end    _tlb_write



# void _tlb_write_random(tlb_entry_t *entry);
#
# Write 'entry' to a "random" row in the TLB.
#
        .globl  _tlb_write_random
        .ent    _tlb_write_random
_tlb_write_random:
  lw  t0, 0(a0)
  mtc0  t0, EntrHi, 0
  lw  t0, 4(a0)
  mtc0  t0, EntLo0, 0
  lw  t0, 8(a0)
  mtc0  t0, EntLo1, 0
  tlbwr
        j ra
        .end    _tlb_write_random
