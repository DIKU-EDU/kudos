/* yams  -- Yet Another Machine Simulator
   Copyright (C) 2002 Juha Aatrokoski, Timo Lilja, Leena Salmela,
   Teemu Takanen, Aleksi Virtanen

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307,
   USA.

   $Id: cp0.h,v 1.4 2005/04/09 14:47:50 jaatroko Exp $
*/


#ifndef YAMS_CP0_H
#define YAMS_CP0_H

#include "cpu_defs.h"


extern char *cp0_register_names[];




#define CP0_CAUSE_IP(cpu) \
  ((0x0000ff00 & (cpu)->cp0->registers[Cause]) >> 8)

#define CP0_CAUSE_EXCCODE(cpu) \
  ((0x0000003c & (cpu)->cp0->registers[Cause]) >> 2)

#define CP0_CAUSE_IV(cpu) \
  ((0x00800000 & (cpu)->cp0->registers[Cause]) >> 23)

#define CP0_STATUS_IM(cpu) \
  ((0x0000ff00 & (cpu)->cp0->registers[Status]) >> 8)

#define CP0_STATUS_IE(cpu) \
  (0x00000001 & (cpu)->cp0->registers[Status])

#define CP0_STATUS_EXL(cpu) \
  ((0x00000002 & (cpu)->cp0->registers[Status]) >> 1)

#define CP0_STATUS_ERL(cpu) \
  ((0x00000004 & (cpu)->cp0->registers[Status]) >> 2)

#define CP0_STATUS_BEV(cpu) \
  ((0x00400000 & (cpu)->cp0->registers[Status]) >> 22)
  
#define CP0_STATUS_RP(cpu) \
  ((0x08000000 & (cpu)->cp0->registers[Status]) >> 27)


#define CP0_ENTRYHI_VPN2(cpu) \
  ((0xffffe000 & (cpu)->cp0->registers[EntryHi]) >> 13)

#define CP0_ENTRYHI_ASID(cpu) \
  ((0x000000ff & (cpu)->cp0->registers[EntryHi]))


#define CP0_ENTRYLO1_PFN(cpu) \
  ((0xcfffffc0 & (cpu)->cp0->registers[EntryLo1]) >> 6)

#define CP0_ENTRYLO1_C(cpu) \
  ((0x00000038 & (cpu)->cp0->registers[EntryLo1]) >> 3)

#define CP0_ENTRYLO1_D(cpu) \
  ((0x00000004 & (cpu)->cp0->registers[EntryLo1]) >> 2)

#define CP0_ENTRYLO1_V(cpu) \
  ((0x00000002 & (cpu)->cp0->registers[EntryLo1]) >> 1)

#define CP0_ENTRYLO1_G(cpu) \
  ((0x00000001 & (cpu)->cp0->registers[EntryLo1]))


#define CP0_ENTRYLO0_PFN(cpu) \
  ((0xcfffffc0 & (cpu)->cp0->registers[EntryLo0]) >> 6)

#define CP0_ENTRYLO0_C(cpu) \
  ((0x00000038 & (cpu)->cp0->registers[EntryLo0]) >> 3)

#define CP0_ENTRYLO0_D(cpu) \
  ((0x00000004 & (cpu)->cp0->registers[EntryLo0]) >> 2)

#define CP0_ENTRYLO0_V(cpu) \
  ((0x00000002 & (cpu)->cp0->registers[EntryLo0]) >> 1)

#define CP0_ENTRYLO0_G(cpu) \
  ((0x00000001 & (cpu)->cp0->registers[EntryLo0]))


/* (EXL == 1 or ERL == 1) or KSU == 0 */
#define CP0_KERNEL_MODE(cpu) \
  ((((cpu)->cp0->registers[Status] & 0x00000006) != 0) || \
   (((cpu)->cp0->registers[Status] & 0x00000010) == 0) )

#define SET_CP0_CAUSE_IP_HW(cpu, value) \
  (cpu)->cp0->registers[Cause] = ((cpu)->cp0->registers[Cause] & ~0x0000fc00) \
  | ((value)<<10) 

#define SET_CP0_CAUSE_IP7(cpu, value) \
  (cpu)->cp0->registers[Cause] = ((cpu)->cp0->registers[Cause] & ~0x00008000) \
  | ((value)<<15) 

#define SET_CP0_CAUSE_IP(cpu, which, value) \
  (cpu)->cp0->registers[Cause] = \
        ((cpu)->cp0->registers[Cause] & ~(1 << ((which) + 8))) \
        | ((value)<< (which + 8)) 

#define SET_CP0_CAUSE_CE(cpu, value) \
  (cpu)->cp0->registers[Cause] = ((cpu)->cp0->registers[Cause] & ~0x30000000) \
  | ((value)<<28) 


#define SET_CP0_CAUSE_BD(cpu, value) \
  (cpu)->cp0->registers[Cause] = ((cpu)->cp0->registers[Cause] & ~0x80000000) \
  | ((value)<<31) 


#define SET_CP0_CAUSE_EXCODE(cpu, value) \
  (cpu)->cp0->registers[Cause] = ((cpu)->cp0->registers[Cause] & ~0x0000007c) \
  | ((value)<<2) 


#define SET_CP0_STATUS_EXL(cpu, value) \
  (cpu)->cp0->registers[Status] = ((cpu)->cp0->registers[Status] & ~0x00000002) \
  | ((value)<<1) 


#define SET_CP0_STATUS_ERL(cpu, value) \
  (cpu)->cp0->registers[Status] = ((cpu)->cp0->registers[Status] & ~0x00000004) \
  | ((value)<<2) 

#define SET_CP0_STATUS_RP(cpu, value) \
 (cpu)->cp0->registers[Status] = ((cpu)->cp0->registers[Status] & ~0x08000000) \
  | ((value)<<27)

#define SET_CP0_CONTEXT_BADVPN2(cpu, addr) \
  (cpu)->cp0->registers[Context] = ((cpu)->cp0->registers[Context] &  ~0x007ffff0) \
  | (((addr) & 0xffffe000) >> 9)

#define SET_CP0_ENTRYHI_BADVPN2(cpu, addr) \
  (cpu)->cp0->registers[EntryHi] = ((cpu)->cp0->registers[EntryHi] &  ~0xffffe000) \
  | ((addr) & 0xffffe000)


#define TLB_G(cp0, n) \
 (((cp0)->tlb[n].vp & 0x00000100) >> 8)

#define TLB_ASID(cp0, n) \
 (((cp0)->tlb[n].vp & 0x000000ff))

#define TLB_VPN2(cp0, n) \
 (((cp0)->tlb[n].vp & 0xffffe000) >> 13)

#define TLB_PFN0(cp0, n) \
 (((cp0)->tlb[n].even_entry & 0x3fffffc0) >> 6)

#define TLB_C0(cp0, n) \
 (((cp0)->tlb[n].even_entry & 0x00000038) >> 3)

#define TLB_D0(cp0, n) \
 (((cp0)->tlb[n].even_entry & 0x00000004) >> 2)

#define TLB_V0(cp0, n) \
 (((cp0)->tlb[n].even_entry & 0x00000002) >> 1)


#define TLB_PFN1(cp0, n) \
 (((cp0)->tlb[n].odd_entry & 0x3fffffc0) >> 6)

#define TLB_C1(cp0, n) \
 (((cp0)->tlb[n].odd_entry & 0x00000038) >> 3)

#define TLB_D1(cp0, n) \
 (((cp0)->tlb[n].odd_entry & 0x00000004) >> 2)

#define TLB_V1(cp0, n) \
 (((cp0)->tlb[n].odd_entry & 0x00000002) >> 1)


/* Macros to set TLB fields. */

#define SET_TLB_G(cp0, n, value) \
  (cp0)->tlb[n].vp = ((cp0)->tlb[n].vp & ~0x00000100) \
  | ((value)<<8)

#define SET_TLB_ASID(cp0, n, value) \
  (cp0)->tlb[n].vp = ((cp0)->tlb[n].vp & ~0x000000ff) \
  | (value)

#define SET_TLB_VPN2(cp0, n, value) \
  (cp0)->tlb[n].vp =((cp0)->tlb[n].vp & ~0xffffe000) \
  | ((value) << 13)

#define SET_TLB_PFN0(cp0, n, value) \
  (cp0)->tlb[n].even_entry = ((cp0)->tlb[n].even_entry & ~0x3fffffc0) \
  | ((value) << 6)

#define SET_TLB_C0(cp0, n, value) \
  (cp0)->tlb[n].even_entry = ((cp0)->tlb[n].even_entry & ~0x00000038) \
  | ((value) << 3)

#define SET_TLB_D0(cp0, n, value) \
  (cp0)->tlb[n].even_entry = ((cp0)->tlb[n].even_entry & ~0x00000004) \
  | ((value) << 2)

#define SET_TLB_V0(cp0, n, value) \
  (cp0)->tlb[n].even_entry = ((cp0)->tlb[n].even_entry & ~0x00000002) \
  | ((value) << 1)


#define SET_TLB_PFN1(cp0, n, value) \
  (cp0)->tlb[n].odd_entry = ((cp0)->tlb[n].odd_entry & ~0x3fffffc0) \
  | ((value) << 6)

#define SET_TLB_C1(cp0, n, value) \
  (cp0)->tlb[n].odd_entry = ((cp0)->tlb[n].odd_entry & ~0x00000038) \
  | ((value) << 3)

#define SET_TLB_D1(cp0, n, value) \
  (cp0)->tlb[n].odd_entry = ((cp0)->tlb[n].odd_entry & ~0x00000004) \
  | ((value) << 2)

#define SET_TLB_V1(cp0, n, value) \
  (cp0)->tlb[n].odd_entry = ((cp0)->tlb[n].odd_entry & ~0x00000002) \
  | ((value) << 1)

#define WRITE_CP0_REG(cp0ptr, reg, value) \
    ((cp0ptr)->registers[(reg)] = (value))
    
#define READ_CP0_REG(cp0ptr, reg) \
    ((cp0ptr)->registers[(reg)])


cp0_t *cp0_create(int cpu_id);

/* Translate virtual address into physical address */
exception_t tlb_translate(cpu_t *cpu, uint32_t va, uint32_t *pa, int reftype);

uint32_t cp0_register_to_number(char * name);

#endif
