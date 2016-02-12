/* yams -- Yet Another Machine Simulator
   Copyright (C) 2002-2010 Juha Aatrokoski, Timo Lilja, Leena Salmela,
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

   $Id: cp0.c,v 1.7 2010/11/23 12:20:26 jaatroko Exp $
*/

#include "includes.h"
#include "cpu.h"
#include "simulator.h"
#include "memory.h"
#include "misc.h"

char *cp0_register_names[] = {
  "Index",
  "Random",
  "EntLo0",
  "EntLo1",
  "Contxt",
  "PgMask",
  "Wired",
  "CP0R7",
  "BadVAd",
  "Count",
  "EntrHi",
  "Compar",
  "Status",
  "Cause",
  "EPC",
  "PRId",
  "Conf0",
  "LLAddr",
  "WtchLo",
  "WtchHi",
  "CP020",
  "CP021",
  "Conf1",
  "Debug",
  "DEPC",
  "PrfCnt",
  "ErrCtl",
  "CacheE",
  "CP0Lo",
  "CP0Hi",
  "ErrEPC",
  "DESAVE",
  NULL
};


uint32_t cp0_register_to_number(char * name){
  int i;

  if (!strncmp(name, "cp0r", 4)){
    return atoi(&name[4]);
  }

  for(i = 0; i < NumCP0Regs; i++) {
    if(!strcmp(name, cp0_register_names[i])) {
      return i;
    }
  }

  return 0xffffffff;
}

cp0_t *cp0_create(int cpu_id) {
  cp0_t *cp0;
  int i;

  cp0 = smalloc(sizeof(cp0_t));

  /* Reset CP0 registers */
  cp0->registers[Index] = 0;
  cp0->registers[Random] = NUM_TLB_ENTRIES - 1;
  cp0->registers[EntryLo0] = 0;
  cp0->registers[EntryLo1] = 0;
  cp0->registers[Context] = 0;
  cp0->registers[PageMask] = 0;
  cp0->registers[Wired] = 0;
  cp0->registers[CP0R7] = 0;  /* reserved */
  cp0->registers[BadVAddr] = 0;
  cp0->registers[Count] = 0;
  cp0->registers[EntryHi] = 0;
  cp0->registers[Compare] = 0;

  /* Status register:
   * CU    31-28   b0001   CP0 access enabled
   * RP    27    0
   * FR    26    0
   * RE    25    0
   * MX    24    0
   * PX    23    0
   * BEV   22    1     Bootstrap mode
   * TS    21    0
   * SR    20    0
   * NMI   19    0
   * 0     18    0
   * Impl  17-16   b00
   * IM    15-8  b00000000
   * KX    7     0
   * SX    6     0
   * UX    5     0
   * UM    4     0
   * R0    3     0
   * ERL   2     1
   * EXL   1     0
   * IE    0     0
   */
  cp0->registers[Status] = 0x10400004;
  /* b 0001 0000 0100 0000 0000 0000 0000 0100 */

  cp0->registers[Cause] = 0;
  cp0->registers[EPC] = 0;

  cp0->registers[PRId] = 0x00ff0000 | (cpu_id << 24);
  /* processor id + 255 + 0 +  0*/


  /* Config register:
   * M 1
   * Impl b0000000
   * BE 1/0
   * AT b00
   * AR b000
   * MT b001
   * VI 0
   * K0 b000
   */
  if (simulator_bigendian)
    cp0->registers[Config] = 0x80008080;
  else
    cp0->registers[Config] = 0x80000080;
  /* b 1000 0000 0000 0000 x000 0000 1000 0000 */

  /* Config1 register:
   * M 0
   * MMU NUM_TLB_ENTRIES - 1
   * IS 0
   * IL 0  no cache
   * IA 0
   * DS 0
   * DL 0  no cache
   * DA 0
   * C2 0 no CP2
   * MD 0 no MDMX ASE
   * PC 0 no performance counter registers
   * WR 0 no watch registers
   * CA 0 no 16-bit MIPS
   * EP 0 no EJTAG
   * FP 0 no FPU
   */
  cp0->registers[Config1] = 0 | ((NUM_TLB_ENTRIES - 1) << 25);

  cp0->registers[LLAddr] = 0xffffffff;  /* invalid */

  cp0->registers[WatchLo] = 0;  /* not implemented? */
  cp0->registers[WatchHi] = 0;  /* not implemented? */
  cp0->registers[CP020] = 0;    /* reserved */
  cp0->registers[CP021] = 0;    /* reserved */
  /* CP022 is Config1 */
  cp0->registers[Debug] = 0;    /* EJTAG stuff */
  cp0->registers[DEPC] = 0;     /* EJTAG stuff */
  cp0->registers[PerfCnt] = 0;  /* not implemented? */
  cp0->registers[ErrCtl] = 0;   /* not implemented*/
  cp0->registers[CacheErr] = 0;   /* no cache */
  cp0->registers[CP0Lo] = 0;    /* no cache */
  cp0->registers[CP0Hi] = 0;    /* no cache */
  cp0->registers[ErrorEPC] = 0;
  cp0->registers[DESAVE] = 0;   /* EJTAG stuff */

  for (i = 0; i < NUM_TLB_ENTRIES; i++) {
    cp0->tlb[i].vp = 0;
    cp0->tlb[i].even_entry = 0;
    cp0->tlb[i].odd_entry = 0;
  }

  return cp0;
}

/* translate given virtual address to a physical page.

   returns: physical page or 0xffffffff on failure.

*/

exception_t tlb_translate(cpu_t *cpu, uint32_t va, uint32_t *pa, int reftype) {
  int i;

  int v, d;
  uint32_t pfn;

  cp0_t *cp0 = cpu->cp0;


  for(i=0; i <NUM_TLB_ENTRIES; i++) {
    if( TLB_VPN2(cp0, i) == ((va & 0xffffe000) >> 13) &&
      (TLB_G(cp0, i) || TLB_ASID(cp0, i) == CP0_ENTRYHI_ASID(cpu)) ) {

      if((va & 0x00001000) == 0) {
        pfn = TLB_PFN0(cp0, i);
        v   = TLB_V0(cp0, i);
        d   = TLB_D0(cp0, i);
      } else {
        pfn = TLB_PFN1(cp0, i);
        v   = TLB_V1(cp0, i);
        d   = TLB_D1(cp0, i);
      }

      if(v == 0) {
        if(reftype == MEM_LOAD) {
          return TLBLoadInvalid;
        } else {
          return TLBStoreInvalid;
        }
      }

      if(d == 0 && reftype == MEM_STORE) {
        return TLBModification;
      }

      *pa = (pfn << 12) | (va & 0x00000fff);
      return NoException;
    }
  }

  if(reftype == MEM_LOAD) {
    return TLBLoad;
  } else {
    return     TLBStore;
  }
}

