/* yams -- Yet Another Machine Simulator
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

   $Id: cpu_defs.h,v 1.3 2002/11/23 16:52:52 lsalmela Exp $
*/

#ifndef CPU_DEFS_H
#define CPU_DEFS_H

#define NUM_TLB_ENTRIES 16  /* Must be a power of 2 */

typedef enum {
    NoException,
    TLBModification,
    TLBLoad,
    TLBStore,
    AddressLoad,
    AddressStore,
    BusErrorInstr,
    BusErrorData,
    Syscall,
    Breakpoint,
    ReservedInstruction,
    CoprocessorUnusable,
    ArithmeticOverflow,
    Trap,
    TLBLoadInvalid = 32, /* virtual entries */
    TLBStoreInvalid = 33
} exception_t;

enum {
    Index,
    Random,
    EntryLo0,
    EntryLo1,
    Context,
    PageMask,
    Wired,
    CP0R7,
    BadVAddr,
    Count,
    EntryHi,
    Compare,
    Status,
    Cause,
    EPC,
    PRId,
    Config,
    LLAddr,
    WatchLo,
    WatchHi,
    CP020,
    CP021,
    CP022,
    Debug,
    DEPC,
    PerfCnt,
    ErrCtl,
    CacheErr,
    CP0Lo,
    CP0Hi,
    ErrorEPC,
    DESAVE,
    NumCP0Regs
};

#define Config1 CP022  /* Config reg selection 1 */


struct _hardware_t;

typedef uint32_t cpu_register_t;
typedef uint32_t cp0_register_t;

typedef struct {
    /* note that PageMask is not implemented*/
    uint32_t vp;
    uint32_t even_entry;
    uint32_t odd_entry;
} tlb_t;

typedef struct _cp0_t {
    cp0_register_t registers[NumCP0Regs];
    tlb_t tlb[NUM_TLB_ENTRIES];

    uint8_t req_timer_interrupt : 1;
} cp0_t;

typedef struct {
    cpu_register_t registers[NumCpuRegs];
    cp0_t *cp0;
    
    uint32_t cpu_id;
    /* interrupt lines */

    /* needed to handle delay slots */
    uint32_t next_pc;

    /* needed for exceptions and traps */
    uint32_t exception_pending;
} cpu_t;

#endif

