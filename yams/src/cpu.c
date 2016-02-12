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

   $Id: cpu.c,v 1.110 2005/06/05 15:00:18 jaatroko Exp $
*/

#include "includes.h"
#include "cpu.h"
#include "simulator.h"
#include "memory.h"
#include "opcodes.h"
#include "disasm.h"

char *cpu_register_names [] = {
  "zero",
  "at",
  "v0",
  "v1",
  "a0",
  "a1",
  "a2",
  "a3",
  "t0",
  "t1",
  "t2",
  "t3",
  "t4",
  "t5",
  "t6",
  "t7",
  "s0",
  "s1",
  "s2",
  "s3",
  "s4",
  "s5",
  "s6",
  "s7",
  "t8",
  "t9",
  "k0",
  "k1",
  "gp",
  "sp",
  "fp",
  "ra",
  "pc",
  "hi",
  "lo",
  NULL
};

/*
char * op_codes[] = {
  "SPECIAL",
  "REGIMM",
  "J",
  "JAL",
  "BEQ",
  "BNE",
  "BLEZ",
  "BGTZ",
  "ADDI",
  "ADDIU",
  "SLTI",
  "SLTIU",
  "ANDI",
  "ORI",
  "XORI",
  "LUI",
  "COP0",
  "COP1",
  "COP2",
  "COP3",
  "BEQL",
  "BNEL",
  "BLEZL",
  "BGTZL",
  "Invalid instr",
  "Invalid instr",
  "Invalid instr",
  "Invalid instr",
  "SPECIAL2",
  "Invalid instr",
  "Invalid instr",
  "Invalid instr",
  "LB",
  "LH",
  "LWL",
  "LW",
  "LBU",
  "LHU",
  "LWR",
  "Invalid instr",
  "SB",
  "SH",
  "SWL",
  "SW",
  "Invalid instr",
  "Invalid instr",
  "SWR",
  "CACHE",
  "LL",
  "LWC1",
  "LWC2",
  "PREF",
  "Invalid instr",
  "LDC1",
  "LDC2",
  "Invalid instr",
  "SC",
  "SWC1",
  "SWC2",
  "Invalid instr",
  "Invalid instr",
  "SDC1",
  "SDC2",
  "Invalid instr",
  NULL
};

char *special_op_codes[] = {
  "SLL",
  "MOVC1",
  "SRL",
  "SRA",
  "SLLV",
  "Invalid special opcode",
  "SRLV",
  "SRAV",
  "JR",
  "JALR",
  "MOVZ",
  "MOVN",
  "SYSCALL",
  "BREAK",
  "Invalid special opcode",
  "SYNC",
  "MFHI",
  "MTHI",
  "MFLO",
  "MTLO",
  "Invalid special opcode",
  "Invalid special opcode",
  "Invalid special opcode",
  "Invalid special opcode",
  "MULT",
  "MULTU",
  "DIV",
  "DIVU",
  "Invalid special opcode",
  "Invalid special opcode",
  "Invalid special opcode",
  "Invalid special opcode",
  "ADD",
  "ADDU",
  "SUB",
  "SUBU",
  "AND",
  "OR",
  "XOR",
  "NOR",
  "Invalid special opcode",
  "Invalid special opcode",
  "SLT",
  "SLTU",
  "Invalid special opcode",
  "Invalid special opcode",
  "Invalid special opcode",
  "Invalid special opcode",
  "TGE",
  "TGEU",
  "TLT",
  "TLTU",
  "TEQ",
  "Invalid special opcode",
  "TNE",
  "Invalid special opcode",
  "Invalid special opcode",
  "Invalid special opcode",
  "Invalid special opcode",
  "Invalid special opcode",
  "Invalid special opcode",
  "Invalid special opcode",
  "Invalid special opcode",
  "Invalid special opcode",
  NULL
};
*/

uint32_t cpu_register_to_number(char *name){
  int i;

  if(name[0] == 'r') {
  return atoi(name+1);
  }

  for(i=0; i<NumCpuRegs; i++) {
  if(!strcasecmp(name, cpu_register_names[i])) {
    return i;
  }
  }

  return 0xffffffff;
}

cpu_t * cpu_init(uint32_t id){
  /* create cpu, cp0 and tlb */
  int i;

  cpu_t * cpu = smalloc(sizeof(cpu_t));
  cpu->cp0 = cp0_create(id);

  cpu->cpu_id = id;

  for(i = 0; i < NumCpuRegs; i++)
      cpu->registers[i] = 0;

  cpu->registers[zero] = 0;
  cpu->registers[PC] = STARTUP_PC;

  cpu->next_pc = cpu->registers[PC] + 4;
  cpu->exception_pending = NoException;

  return cpu;
}

void cpu_destroy(cpu_t *cpu){
}

char *exception_names[] = {
  "NoException",
  "TLBModification",
  "TLBLoad",
  "TLBStore",
  "AddressLoad",
  "AddressStore",
  "BusErrorInstr",
  "BusErrorData",
  "Syscall",
  "Breakpoint",
  "ReservedInstruction",
  "CoprocessorUnusable",
  "ArithmeticOverflow",
  "Trap",
  NULL
};

static void raise_exception(cpu_t * cpu, exception_t which) {
  cpu->exception_pending = which;
}

static void raise_address_exception(cpu_t * cpu, exception_t which,uint32_t addr)
{

  if (which == TLBModification ||
  which == TLBLoad || which == TLBStore ||
  which == TLBLoadInvalid || which == TLBStoreInvalid) {
    /* TLB exceptions write to Context and EntryHi registers */
    /* EntryHi ASID field already contains the right value */
    SET_CP0_CONTEXT_BADVPN2(cpu, addr);
    SET_CP0_ENTRYHI_BADVPN2(cpu, addr);
  }

  cpu->exception_pending = which;
  cpu->cp0->registers[BadVAddr] = addr;
}

static void raise_cp_exception(cpu_t * cpu, exception_t which, uint32_t cp_nro) {
  cpu->exception_pending = which;
  SET_CP0_CAUSE_CE(cpu, cp_nro);
}

/* Select a CPU in round-robin manner */
int select_cpu_for_irq() {
  static int current = -1;

  current = (current + 1) % hardware->num_cpus;
  return current;
}

static void instr_decode(instr_t * instr) {
  instr->opcode = instr->instr >> 26;
  instr->rs = (instr->instr & 0x03e00000) >> 21;
  instr->rt = (instr->instr & 0x001f0000) >> 16;
  instr->rd = (instr->instr & 0x0000f800) >> 11;
  instr->sa = (instr->instr & 0x000007c0) >> 6;
  instr->instr_index = (instr->instr & 0x03ffffff);
  instr->immediate = (instr->instr & 0x0000ffff);
  instr->function = instr->instr & 0x0000003f;
}


void disasm_instruction(uint32_t addr,
      uint32_t instr_word,
      char *buf,
      int len) {
  instr_t instr;

  instr.instr = instr_word;
  instr_decode(&instr);

  disasm(addr, &instr, buf, len);
}

static void invalidate_rmw_sequences(uint32_t vAddr) {
  int i;

  for(i = 0 ; i < hardware->num_cpus ; i++) {
  if(hardware->cpus[i]->cp0->registers[LLAddr] == vAddr) {
    hardware->cpus[i]->cp0->registers[LLAddr]=0xffffffff; /* invalid */
  }
  }
}


int cpu_exception_condition(cpu_t *cpu, uint32_t *exceptionCode) {
  uint32_t ex;

  *exceptionCode = NoException;
  ex       = CP0_CAUSE_IP(cpu) & CP0_STATUS_IM(cpu);

  if(cpu->exception_pending != NoException) {
  *exceptionCode = cpu->exception_pending;
  cpu->exception_pending = NoException;
  return 1;
  }

  if(ex != 0 &&
     CP0_STATUS_IE(cpu) == 1 &&
     CP0_STATUS_EXL(cpu) == 0 &&
     CP0_STATUS_ERL(cpu) == 0) {
  *exceptionCode = NoException; /* interrupt */
  return 1;
  }

  return 0;
}

static void cpu_timer_interrupt(cpu_t *cpu) {

  /* Clear interrupts */
  SET_CP0_CAUSE_IP_HW(cpu, 0);

  /* Count register increment + timer interrupt */
  cpu->cp0->registers[Count]++;

  if (cpu->cp0->registers[Count] == cpu->cp0->registers[Compare]) {
    cpu->cp0->req_timer_interrupt = 1;
  }

  if (cpu->cp0->req_timer_interrupt == 1) {
    SET_CP0_CAUSE_IP7(cpu, 1);
  }
}

static void cpu_next_cycle(cpu_t *cpu){
  instr_t instr;
  uint32_t next_next_pc = cpu->next_pc + 4;
  exception_t exc;
  uint32_t exceptionCode;

  uint8_t temp8;
  uint16_t temp16;
  uint32_t temp32;
  uint64_t temp64;

  if(cpu_exception_condition(cpu, &exceptionCode)) {
  uint32_t vectorOffset, vectorBase;

  if(CP0_STATUS_EXL(cpu) == 0) {
    if(cpu->registers[PC] != cpu->next_pc-4) {
    if(CP0_STATUS_RP(cpu) == 1) {
      /* Stalled by wait. After interrupt continue from
         PC + 4. */
      cpu->cp0->registers[EPC] = cpu->registers[PC] + 4;
      SET_CP0_STATUS_RP(cpu, 0);
    }
    else {
      /* in delay slot */
      cpu->cp0->registers[EPC] = cpu->registers[PC] - 4;
    }
    SET_CP0_CAUSE_BD(cpu, 1);
    } else {
    cpu->cp0->registers[EPC] = cpu->registers[PC];
    SET_CP0_CAUSE_BD(cpu, 0);
    }

    if(exceptionCode == TLBLoad ||
       exceptionCode == TLBStore) {
        vectorOffset = 0;
    } else if(exceptionCode == TLBModification) {
        vectorOffset = 0x180;
    } else if(exceptionCode == TLBLoadInvalid) {
        vectorOffset = 0x180;
    exceptionCode = TLBLoad;
    } else if(exceptionCode == TLBStoreInvalid) {
        vectorOffset = 0x180;
    exceptionCode = TLBStore;
      } else if(exceptionCode == Breakpoint ||
        exceptionCode == Syscall) {
      vectorOffset = 0x180;
    } else if(CP0_CAUSE_EXCCODE(cpu) == NoException /* interrupt */ &&
        CP0_CAUSE_IV(cpu) == 1) {
    vectorOffset = 0x200;
    } else {
    vectorOffset = 0x180;
    }
  } else {
      if (exceptionCode == TLBLoadInvalid)
        exceptionCode = TLBLoad;
      if (exceptionCode == TLBStoreInvalid)
        exceptionCode = TLBStore;
    vectorOffset = 0x180;
  }

  SET_CP0_CAUSE_EXCODE(cpu, exceptionCode);
  SET_CP0_STATUS_EXL(cpu, 1);

  if(CP0_STATUS_BEV(cpu) == 1) {
    vectorBase = 0xbfc00200;
  } else {
    vectorBase = 0x80000000;
  }

  cpu->registers[PC] = vectorBase + vectorOffset;
  cpu->next_pc = cpu->registers[PC] + 4;
  next_next_pc = cpu->next_pc + 4;

  if (cpu->registers[PC] == hardware->breakpoint) {
    hardware->running = 0;
    printf("CPU %" PRId32 " hit breakpoint at #%.8" PRIx32
       " (exception handler entry)\n",
       cpu->cpu_id, hardware->breakpoint);
      /* Return, so that the first instruction of exception handler
         can be stepped normally in hw-console. Note that this causes
         excepting instruction execution take two clock cycles. */
    return;
  }
  }

  exc = mem_read(hardware->memory, cpu->registers[PC], &instr.instr, 4,
           cpu);

  if (exc != NoException) {
    if (exc == BusErrorData)
      exc = BusErrorInstr;
    raise_address_exception(cpu, exc, cpu->registers[PC]);

    return;
  }

  instr_decode(&instr);

  /* This might help in debugging the hand-written machine code... */
  /*
  printf("Instruction decode:\n");
  printf("opcode: %x, rs %x, rt %x, rd %x\n",
       instr.opcode, instr.rs, instr.rt, instr.rd);
  printf("sa %x, instr_index %x, immediate %x function %x\n",
       instr.sa, instr.instr_index, instr.immediate, instr.function);
  */
  /* execute next instruction */
  switch(instr.opcode) {
  case OP_SPECIAL:   /* 0x00 */
    switch(instr.function) {
    case SPEC_SLL: /* 0x00 */
      WRITE_REG(cpu, instr.rd, READ_REG(cpu, instr.rt) << instr.sa);
      break;
    case SPEC_MOVC1: /* 0x01 */
      raise_cp_exception(cpu, CoprocessorUnusable, 1);
    return;
      break;
    case SPEC_SRL: /* 0x02 */
      WRITE_REG(cpu, instr.rd, READ_REG(cpu, instr.rt) >> instr.sa);
      break;
    case SPEC_SRA: /* 0x03 */
      WRITE_REG(cpu, instr.rd,
        (uint32_t)(((int32_t)READ_REG(cpu, instr.rt)) >> instr.sa));
      break;
    case SPEC_SLLV: /* 0x04 */
      WRITE_REG(cpu, instr.rd,
        READ_REG(cpu, instr.rt) <<
        (READ_REG(cpu, instr.rs) & 0x1f));
      break;
    case SPEC_SRLV: /* 0x06 */
      WRITE_REG(cpu, instr.rd,
        READ_REG(cpu, instr.rt) >>
        (READ_REG(cpu, instr.rs) & 0x1f));
      break;
    case SPEC_SRAV: /* 0x07 */
      WRITE_REG(cpu, instr.rd,
        (uint32_t)(((int32_t)READ_REG(cpu, instr.rt)) >>
        (READ_REG(cpu, instr.rs) & 0x1f)));
      break;
    case SPEC_JR: /* 0x08 */
      next_next_pc = READ_REG(cpu, instr.rs);
      break;
    case SPEC_JALR: /* 0x09 */
    WRITE_REG(cpu, instr.rd, READ_REG(cpu, PC) + 8);
    next_next_pc = READ_REG(cpu, instr.rs); /* jump to reg[rs] */
      break;
    case SPEC_MOVZ: /* 0x0a */
    if ( READ_REG(cpu, instr.rt) == 0 ) {
    WRITE_REG(cpu, instr.rd,
        READ_REG(cpu, instr.rs));
    }
      break;
    case SPEC_MOVN: /* 0x0b */
    if ( READ_REG(cpu, instr.rt) != 0 ) {
    WRITE_REG(cpu, instr.rd,
        READ_REG(cpu, instr.rs));
    }
      break;
    case SPEC_SYSCALL: /* 0x0c */
    raise_exception(cpu, Syscall);
    return;
      break;
    case SPEC_BREAK: /* 0x0d */
    raise_exception(cpu, Breakpoint);
    return;
      break;
    case SPEC_SYNC: /* 0x0f */
      /* nop */
      break;
    case SPEC_MFHI: /* 0x10 */
      WRITE_REG(cpu, instr.rd, READ_REG(cpu, HI));
      break;
    case SPEC_MTHI: /* 0x11 */
      WRITE_REG(cpu, HI, READ_REG(cpu, instr.rs));
      break;
    case SPEC_MFLO: /* 0x12 */
      WRITE_REG(cpu, instr.rd, READ_REG(cpu, LO));
      break;
    case SPEC_MTLO: /* 0x13 */
      WRITE_REG(cpu, LO, READ_REG(cpu, instr.rs));
      break;
    case SPEC_MULT: /* 0x18 */
    {
    int64_t itemp64;

    itemp64 = (int64_t)(int32_t)READ_REG(cpu, instr.rs) *
      (int64_t)(int32_t)READ_REG(cpu, instr.rt);
    WRITE_REG(cpu, LO, (uint32_t)itemp64);
    WRITE_REG(cpu, HI, (uint32_t)(itemp64 >> 32));
    }
      break;
    case SPEC_MULTU: /* 0x19 */
    temp64 = ((uint64_t) READ_REG(cpu, instr.rs)) *
    ((uint64_t) READ_REG(cpu, instr.rt));
    WRITE_REG(cpu, LO, (uint32_t) temp64);
    WRITE_REG(cpu, HI, (uint32_t) (temp64 >> 32));
      break;
    case SPEC_DIV: /* 0x1a */
      {
        uint32_t temp2 = READ_REG(cpu, instr.rs);
        temp32 = READ_REG(cpu, instr.rt);
        if(temp32 == 0)
          break; /* Result can be unpredictable by spec */
        /* NaN / -1 might crash on Pentium CPU*/
        if (temp2 == 0x80000000 && temp32 == 0xffffffff) {
          WRITE_REG(cpu, LO, 0x80000000);
          WRITE_REG(cpu, HI, 0);
          break;
        }
        WRITE_REG(cpu, LO, ((int32_t) temp2) / ((int32_t) temp32));
        WRITE_REG(cpu, HI, ((int32_t) temp2) % ((int32_t) temp32));
        break;
      }
    case SPEC_DIVU: /* 0x1b */
    temp32 = READ_REG(cpu, instr.rt);
    if(temp32 == 0)
    break; /* Result can be unpredictable by spec */
    WRITE_REG(cpu, LO, READ_REG(cpu, instr.rs) / temp32);
    WRITE_REG(cpu, HI, READ_REG(cpu, instr.rs) % temp32);
      break;
    case SPEC_ADD: /* 0x20 */
    {
    uint32_t left, right, samesign;

    left = READ_REG(cpu, instr.rs);
    right = READ_REG(cpu, instr.rt);

    samesign = (~(left ^ right)) >> 31;
    temp32 = left + right;

    /* Check overflow (left & right of same sign, temp the other)*/
    if (samesign && ((left ^ temp32) >> 31)) {
      raise_exception(cpu, ArithmeticOverflow);
      return;
    } else {
      WRITE_REG(cpu, instr.rd, temp32);
    }
    }
    break;
    case SPEC_ADDU: /* 0x21 */
      WRITE_REG(cpu, instr.rd,
            READ_REG(cpu,instr.rs) + READ_REG(cpu, instr.rt));
      break;
    case SPEC_SUB: /* 0x22 */
    {
    uint32_t left, right, samesign;

    /* same as SPEC_ADD, only negate rt first */
    left = READ_REG(cpu, instr.rs);
    right = (~READ_REG(cpu, instr.rt)) + 1; /* NEG */

    /* special case for 0x80000000, as it has
     * no valid negation (the addition works, however) */
    if (right == 0x80000000)
      samesign = (~(left ^ 0)) >> 31;
    else
      samesign = (~(left ^ right)) >> 31;
    temp32 = left + right;

    /* Check overflow (left & right of same sign, temp the other)*/
    if (samesign && ((left ^ temp32) >> 31)) {
      raise_exception(cpu, ArithmeticOverflow);
      return;
    } else {
      WRITE_REG(cpu, instr.rd, temp32);
    }
    }
      break;
    case SPEC_SUBU: /* 0x23 */
      WRITE_REG(cpu, instr.rd,
            READ_REG(cpu,instr.rs) - READ_REG(cpu, instr.rt));
      break;
    case SPEC_AND: /* 0x24 */
    WRITE_REG(cpu, instr.rd,
        READ_REG(cpu, instr.rs) & READ_REG(cpu, instr.rt));
      break;
    case SPEC_OR: /* 0x25 */
    WRITE_REG(cpu, instr.rd,
        READ_REG(cpu, instr.rs) | READ_REG(cpu, instr.rt));
      break;
    case SPEC_XOR: /* 0x26 */
    WRITE_REG(cpu, instr.rd,
        READ_REG(cpu, instr.rs) ^ READ_REG(cpu, instr.rt));
      break;
    case SPEC_NOR: /* 0x27 */
    WRITE_REG(cpu, instr.rd,
        ~(READ_REG(cpu, instr.rs) | READ_REG(cpu, instr.rt)));
      break;
    case SPEC_SLT: /* 0x2a */
    if (((int32_t) READ_REG(cpu, instr.rs)) <
    ((int32_t) READ_REG(cpu, instr.rt)))
    WRITE_REG(cpu, instr.rd, 1);
    else
    WRITE_REG(cpu, instr.rd, 0);
      break;
    case SPEC_SLTU: /* 0x2b */
    if (READ_REG(cpu, instr.rs) < READ_REG(cpu, instr.rt))
    WRITE_REG(cpu, instr.rd, 1);
    else
    WRITE_REG(cpu, instr.rd, 0);
      break;
    case SPEC_TGE: /* 0x30 */
    if( ((int32_t) READ_REG(cpu, instr.rs))
    >= ((int32_t) READ_REG(cpu, instr.rt)) ) {
    raise_exception(cpu, Trap);
    return;
    }
      break;
    case SPEC_TGEU: /* 0x31 */
    if( READ_REG(cpu, instr.rs)
    >= READ_REG(cpu, instr.rt) ) {
    raise_exception(cpu, Trap);
    return;
    }
      break;
    case SPEC_TLT: /* 0x32 */
    if( ((int32_t) READ_REG(cpu, instr.rs))
    < ((int32_t) READ_REG(cpu, instr.rt)) ) {
    raise_exception(cpu, Trap);
    return;
    }
      break;
    case SPEC_TLTU: /* 0x33 */
    if( READ_REG(cpu, instr.rs)
    < READ_REG(cpu, instr.rt) ) {
    raise_exception(cpu, Trap);
    return;
    }
      break;
    case SPEC_TEQ: /* 0x34 */
    if( READ_REG(cpu, instr.rs) ==
    READ_REG(cpu, instr.rt) ) {
    raise_exception(cpu, Trap);
    return;
    }
      break;
    case SPEC_TNE: /* 0x36 */
    if( READ_REG(cpu, instr.rs) !=
    READ_REG(cpu, instr.rt) ) {
    raise_exception(cpu, Trap);
    return ;
    }
      break;
    default:
      raise_exception(cpu, ReservedInstruction);
      return;
    }
    break;
  case OP_REGIMM: /* 0x01 */
    switch(instr.rt) {
    case REGIMM_BLTZ: /* 0x00 */
      temp32 = (uint32_t)((I_SIGN_EXTEND_16(instr.immediate) << 2) +
                cpu->next_pc);
      if (((int32_t)READ_REG(cpu, instr.rs)) < 0) {
        next_next_pc = temp32;
      }
      break;
    case REGIMM_BGEZ: /* 0x01 */
      temp32 = (uint32_t)((I_SIGN_EXTEND_16(instr.immediate) << 2) +
                cpu->next_pc);
      if(((int32_t)READ_REG(cpu, instr.rs) >= 0)) {
        next_next_pc = temp32;
      }
      break;
    case REGIMM_BLTZL: /* 0x02 */
      temp32 = (uint32_t)((I_SIGN_EXTEND_16(instr.immediate) << 2) +
                cpu->next_pc);
      if (((int32_t)READ_REG(cpu, instr.rs)) < 0) {
        next_next_pc = temp32;
      } else {
        cpu->next_pc = next_next_pc;
        next_next_pc = cpu->next_pc + 4;
      }
      break;
    case REGIMM_BGEZL: /* 0x03 */
      temp32 = (uint32_t)((I_SIGN_EXTEND_16(instr.immediate) << 2) +
                cpu->next_pc);
      if (((int32_t)READ_REG(cpu, instr.rs)) >= 0) {
        next_next_pc = temp32;
      } else {
        cpu->next_pc = next_next_pc;
        next_next_pc = cpu->next_pc + 4;
      }
      break;
    case REGIMM_TGEI: /* 0x08 */
    if ( ((int32_t) READ_REG(cpu, instr.rs)) >=
     I_SIGN_EXTEND_16(instr.immediate) ) {
    raise_exception(cpu, Trap);
    return ;
    }
      break;
    case REGIMM_TGEIU: /* 0x09 */
    if ( READ_REG(cpu, instr.rs) >=
     U_SIGN_EXTEND_16(instr.immediate) ) {
    raise_exception(cpu, Trap);
    return ;
    }
      break;
    case REGIMM_TLTI: /* 0x0a */
    if ( ((int32_t) READ_REG(cpu, instr.rs)) <
     I_SIGN_EXTEND_16(instr.immediate) ) {
    raise_exception(cpu, Trap);
    return ;
    }
      break;
    case REGIMM_TLTIU: /* 0x0b */
    if ( READ_REG(cpu, instr.rs) <
     U_SIGN_EXTEND_16(instr.immediate) ) {
    raise_exception(cpu, Trap);
    return ;
    }
      break;
    case REGIMM_TEQI: /* 0x0c */
    if ( ((int32_t) READ_REG(cpu, instr.rs)) ==
     I_SIGN_EXTEND_16(instr.immediate) ) {
    raise_exception(cpu, Trap);
    return ;
    }
      break;
    case REGIMM_TNEI: /* 0x0e */
    if ( ((int32_t) READ_REG(cpu, instr.rs)) !=
     I_SIGN_EXTEND_16(instr.immediate) ) {
    raise_exception(cpu, Trap);
    return ;
    }
      break;
    case REGIMM_BLTZAL: /* 0x10 */
      temp32 = (uint32_t)((I_SIGN_EXTEND_16(instr.immediate) << 2) +
                cpu->next_pc);
      WRITE_REG(cpu, R31, cpu->registers[PC] + 8);
      if (((int32_t)READ_REG(cpu, instr.rs)) < 0) {
        next_next_pc = temp32;
      }
      break;
    case REGIMM_BGEZAL: /* 0x11 */
      temp32 = (uint32_t)((I_SIGN_EXTEND_16(instr.immediate) << 2) +
                cpu->next_pc);
      WRITE_REG(cpu, R31, cpu->registers[PC] + 8);
      if (((int32_t)READ_REG(cpu, instr.rs)) >= 0) {
        next_next_pc = temp32;
      }
      break;
    case REGIMM_BLTZALL: /* 0x12 */
      temp32 = (uint32_t)((I_SIGN_EXTEND_16(instr.immediate) << 2) +
                cpu->next_pc);
      WRITE_REG(cpu, R31, cpu->registers[PC] + 8);
      if (((int32_t)READ_REG(cpu, instr.rs)) < 0) {
        next_next_pc = temp32;
      } else {
        cpu->next_pc = next_next_pc;
        next_next_pc = cpu->next_pc + 4;
      }
      break;
    case REGIMM_BGEZALL: /* 0x13 */
      temp32 = (uint32_t)((I_SIGN_EXTEND_16(instr.immediate) << 2) +
                cpu->next_pc);
      WRITE_REG(cpu, R31, cpu->registers[PC] + 8);
      if (((int32_t)READ_REG(cpu, instr.rs)) >= 0) {
        next_next_pc = temp32;
      } else {
        cpu->next_pc = next_next_pc;
        next_next_pc = cpu->next_pc + 4;
      }
      break;
    default:
      raise_exception(cpu, ReservedInstruction);
      return;
    }
    break;
  case OP_J:   /* 0x02 */
    next_next_pc = ((cpu->registers[PC] + 4) & 0xf0000000) |
      (instr.instr_index << 2);
    break;
  case OP_JAL: /* 0x03 */
  WRITE_REG(cpu, R31, READ_REG(cpu, PC) + 8); /* ra = retaddr */
    next_next_pc = ((READ_REG(cpu, PC) + 4) & 0xf0000000) |
      (instr.instr_index << 2);  /* word in current 256MB region */
  break;
  case OP_BEQ: /* 0x04 */
    temp32 = (uint32_t)((I_SIGN_EXTEND_16(instr.immediate) << 2) +
              cpu->next_pc);
    if (READ_REG(cpu, instr.rs) == READ_REG(cpu, instr.rt)) {
      next_next_pc = temp32;
    }
    break;
  case OP_BNE: /* 0x05 */
    temp32 = (uint32_t)((I_SIGN_EXTEND_16(instr.immediate) << 2) +
              cpu->next_pc);
    if (READ_REG(cpu, instr.rs) != READ_REG(cpu, instr.rt)) {
      next_next_pc = temp32;
    }
    break;
  case OP_BLEZ: /* 0x06 */
    temp32 = (uint32_t)((I_SIGN_EXTEND_16(instr.immediate) << 2) +
              cpu->next_pc);
    if (((int32_t)READ_REG(cpu, instr.rs)) <= 0) {
      next_next_pc = temp32;
    }
    break;
  case OP_BGTZ: /* 0x07 */
    temp32 = (uint32_t)((I_SIGN_EXTEND_16(instr.immediate) << 2) +
              cpu->next_pc);
    if (((int32_t)READ_REG(cpu, instr.rs)) > 0) {
      next_next_pc = temp32;
    }
    break;
  case OP_ADDI: /* 0x08 */
  {
    uint32_t left, right, samesign;

    /* same as SPEC_ADD, only use sign-extended immediate */
    left = READ_REG(cpu, instr.rs);
    right = U_SIGN_EXTEND_16(instr.immediate);

    samesign = (~(left ^ right)) >> 31;
    temp32 = left + right;

    /* Check overflow (left & right of same sign, temp the other)*/
    if (samesign && ((left ^ temp32) >> 31)) {
    raise_exception(cpu, ArithmeticOverflow);
    return ;
    } else {
    WRITE_REG(cpu, instr.rt, temp32);
    }
  }
    break;
  case OP_ADDIU: /* 0x09 */
  temp32 = READ_REG(cpu, instr.rs) +
    U_SIGN_EXTEND_16(instr.immediate);
  WRITE_REG(cpu, instr.rt, temp32);
    break;
  case OP_SLTI: /* 0x0a */
  if (((int32_t) READ_REG(cpu, instr.rs)) < I_SIGN_EXTEND_16(instr.immediate))
    WRITE_REG(cpu, instr.rt, 1);
  else
    WRITE_REG(cpu, instr.rt, 0);
    break;
  case OP_SLTIU: /* 0x0b */
  if (READ_REG(cpu, instr.rs) < U_SIGN_EXTEND_16(instr.immediate))
    WRITE_REG(cpu, instr.rt, 1);
  else
    WRITE_REG(cpu, instr.rt, 0);
    break;
  case OP_ANDI: /* 0x0c */
  WRITE_REG(cpu, instr.rt,
      READ_REG(cpu, instr.rs) & (uint32_t)instr.immediate);
    break;
  case OP_ORI: /* 0x0d */
  WRITE_REG(cpu, instr.rt,
      READ_REG(cpu, instr.rs) | (uint32_t)instr.immediate);
    break;
  case OP_XORI: /* 0x0e */
  WRITE_REG(cpu, instr.rt,
      READ_REG(cpu, instr.rs) ^ (uint32_t)instr.immediate);
    break;
  case OP_LUI: /* 0x0f */
  WRITE_REG(cpu, instr.rt, (((uint32_t) instr.immediate)<<16));
    break;
  case OP_COP0: /* 0x10 */
  if(CP0_KERNEL_MODE(cpu)) {
    if ((instr.rs & 0x10) != 0) {
    switch(instr.function) {
    case COP0_TLBR: /* 0x01 */
      {
      cp0_register_t i = READ_CP0_REG(cpu->cp0, Index) &
              0x7fffffff;
      if (i < (cp0_register_t)NUM_TLB_ENTRIES) {
        temp32 = (TLB_VPN2(cpu->cp0, i) << 13) |
        TLB_ASID(cpu->cp0, i);
        WRITE_CP0_REG(cpu->cp0, EntryHi, temp32);

        temp32 = (TLB_PFN1(cpu->cp0, i) << 6) |
        (TLB_C1(cpu->cp0, i) << 3) |
        (TLB_D1(cpu->cp0, i) << 2) |
        (TLB_V1(cpu->cp0, i) << 1) |
        TLB_G(cpu->cp0, i);
        WRITE_CP0_REG(cpu->cp0, EntryLo1, temp32);

        temp32 = (TLB_PFN0(cpu->cp0, i) << 6) |
        (TLB_C0(cpu->cp0, i) << 3) |
        (TLB_D0(cpu->cp0, i) << 2) |
        (TLB_V0(cpu->cp0, i) << 1) |
        TLB_G(cpu->cp0, i);
        WRITE_CP0_REG(cpu->cp0, EntryLo0, temp32);
      }
      }
      break;
    case COP0_TLBWI: /* 0x02 */
      {
      cp0_register_t i = READ_CP0_REG(cpu->cp0, Index) &
              0x7fffffff;

      /* operation UNDEFINED if i > number of tlb entries */
      if (i < (cp0_register_t)NUM_TLB_ENTRIES) {
        SET_TLB_VPN2(cpu->cp0, i, CP0_ENTRYHI_VPN2(cpu));
        SET_TLB_ASID(cpu->cp0, i, CP0_ENTRYHI_ASID(cpu));
        SET_TLB_G(cpu->cp0, i, CP0_ENTRYLO1_G(cpu) &
            CP0_ENTRYLO0_G(cpu));
        SET_TLB_PFN1(cpu->cp0, i, CP0_ENTRYLO1_PFN(cpu));
        SET_TLB_C1(cpu->cp0, i, CP0_ENTRYLO1_C(cpu));
        SET_TLB_D1(cpu->cp0, i, CP0_ENTRYLO1_D(cpu));
        SET_TLB_V1(cpu->cp0, i, CP0_ENTRYLO1_V(cpu));
        SET_TLB_PFN0(cpu->cp0, i, CP0_ENTRYLO0_PFN(cpu));
        SET_TLB_C0(cpu->cp0, i, CP0_ENTRYLO0_C(cpu));
        SET_TLB_D0(cpu->cp0, i, CP0_ENTRYLO0_D(cpu));
        SET_TLB_V0(cpu->cp0, i, CP0_ENTRYLO0_V(cpu));
      }
      }
      break;
    case COP0_TLBWR: /* 0x06 */
      {
      cp0_register_t i = READ_CP0_REG(cpu->cp0, Random);

      SET_TLB_VPN2(cpu->cp0, i, CP0_ENTRYHI_VPN2(cpu));
      SET_TLB_ASID(cpu->cp0, i, CP0_ENTRYHI_ASID(cpu));
      SET_TLB_G(cpu->cp0, i,
          CP0_ENTRYLO1_G(cpu) & CP0_ENTRYLO0_G(cpu));
      SET_TLB_PFN1(cpu->cp0, i, CP0_ENTRYLO1_PFN(cpu));
      SET_TLB_C1(cpu->cp0, i, CP0_ENTRYLO1_C(cpu));
      SET_TLB_D1(cpu->cp0, i, CP0_ENTRYLO1_D(cpu));
      SET_TLB_V1(cpu->cp0, i, CP0_ENTRYLO1_V(cpu));
      SET_TLB_PFN0(cpu->cp0, i, CP0_ENTRYLO0_PFN(cpu));
      SET_TLB_C0(cpu->cp0, i, CP0_ENTRYLO0_C(cpu));
      SET_TLB_D0(cpu->cp0, i, CP0_ENTRYLO0_D(cpu));
      SET_TLB_V0(cpu->cp0, i, CP0_ENTRYLO0_V(cpu));

            /* Update random register */
            if (i - 1 >= READ_CP0_REG(cpu->cp0, Wired) && i != 0)
              cpu->cp0->registers[Random] = i - 1;
            else
              cpu->cp0->registers[Random] = NUM_TLB_ENTRIES - 1;

      }
      break;
    case COP0_TLBP: /* 0x08 */
      {
      int i;

      /* set highest bit in Index register */
      WRITE_CP0_REG(cpu->cp0, Index,
            0x80000000 |
            READ_CP0_REG(cpu->cp0, Index));

      for(i=0; i < NUM_TLB_ENTRIES; i++) {
        if ((TLB_VPN2(cpu->cp0, i) ==
         CP0_ENTRYHI_VPN2(cpu)) &&
        ((TLB_G(cpu->cp0,i)  == 1) ||
         (TLB_ASID(cpu->cp0,i) ==
          CP0_ENTRYHI_ASID(cpu)))) {
        WRITE_CP0_REG(cpu->cp0, Index, i);
        }

      }
      }
      break;
    case COP0_ERET: /* 0x18 */
      if(CP0_STATUS_ERL(cpu) == 1) {
      temp32 = cpu->cp0->registers[ErrorEPC];
      SET_CP0_STATUS_ERL(cpu, 0);
      } else {
      temp32 = cpu->cp0->registers[EPC];
      SET_CP0_STATUS_EXL(cpu, 0);
      }

      cpu->next_pc     = temp32;
      next_next_pc     = cpu->next_pc + 4;

      cpu->cp0->registers[LLAddr] = 0xffffffff;
      break;
    case COP0_WAIT: /* 0x20 */
      /* Simply prevent PC forwarding. Set bit in CP0:Status
         register to inform that we are in power save mode.
      */
      cpu->next_pc = cpu->registers[PC];
      next_next_pc = cpu->registers[PC];

      SET_CP0_STATUS_RP(cpu, 1);

      break;
    default:
      raise_exception(cpu, ReservedInstruction);
      return;
    }
    } else {
    switch(instr.rs) {
    case COP0_MFC0: /* 0x00 */
      {
      /* Check for config register */
      uint8_t sel = instr.instr & 0x00000007;
      if ((instr.rd == Config) && (sel == 1)) {
        WRITE_REG(cpu, instr.rt,
            READ_CP0_REG(cpu->cp0, Config1));
      } else {
        WRITE_REG(cpu, instr.rt,
            READ_CP0_REG(cpu->cp0, instr.rd));
      }
            break;
      }
    case COP0_MTC0: /* 0x08 */
      {
      switch(instr.rd) {
      case Index:
        cpu->cp0->registers[instr.rd] =
        (cpu->cp0->registers[instr.rd] & 0x80000000) |
        (READ_REG(cpu, instr.rt)&(NUM_TLB_ENTRIES-1));
        break;
      case EntryLo0:
      case EntryLo1:
        cpu->cp0->registers[instr.rd] =
        (READ_REG(cpu, instr.rt) & 0x03ffffff);
        break;
      case Context:
        cpu->cp0->registers[instr.rd] =
        (cpu->cp0->registers[instr.rd] & 0x007ffff0) |
        (READ_REG(cpu, instr.rt) & 0xff800000);
        break;
      case Wired:
        cpu->cp0->registers[instr.rd] =
        (READ_REG(cpu, instr.rt)&(NUM_TLB_ENTRIES-1));
        /* Side effect */
        cpu->cp0->registers[Random] = NUM_TLB_ENTRIES-1;
        break;
      case EntryHi:
        /* Could be R/W */
        cpu->cp0->registers[instr.rd] =
        (READ_REG(cpu, instr.rt) & 0xffffe0ff);
        break;
      case Status:
        /* b 0001 0000 0100 0000 1111 1111 0001 0111 */
        cpu->cp0->registers[instr.rd] =
        (READ_REG(cpu, instr.rt) & 0x1040ff17);
        break;
      case Cause:
        /* b 1011  0000 0000 0000 1111 1100 0111 1100 */
        /* b 0000  0000 1000 0000 0000 0011 0000 0000 */
        cpu->cp0->registers[instr.rd] =
        (cpu->cp0->registers[instr.rd] & 0xb000fc7c) |
        (READ_REG(cpu, instr.rt) & 0x00800300);
        break;
      case Compare:
              /* Read/Write register + clear interrupt */
              cpu->cp0->req_timer_interrupt = 0;
              /* fall through */
      case EPC:
      case Count:
      case ErrorEPC:
        /* Read/Write registers*/
        cpu->cp0->registers[instr.rd] =
        READ_REG(cpu, instr.rt);
        break;

        /* By default do nothing i.e. read-only registers: */
        /* - Random */
        /* - PageMask (only 4 kB pages supported) */
        /* - CP07 reserved*/
        /* - BadVAddr */
        /* - PRId */
        /* - Config */
        /* - Config1 */
        /* - LLAddr */
        /* - WatchLo (not implemented) */
        /* - WatchHi (not implemented) */
        /* - CP020 (reserved) */
        /* - CP021 (reserved) */
        /* - CP022  = Config1 */
        /* - Debug (EJTAG) */
        /* - DEPC (EJTAG) */
        /* - PerfCnt (not implemented) */
        /* - ErrCtl (not imeplemented) */
        /* - CacheErr (no cache) */
        /* - CP0Lo (no cache) */
        /* - CP0Hi (no cahce) */
        /* - DESAVE (EJTAG) */
      }

      break;
      }
      break;
    default:
      raise_exception(cpu, ReservedInstruction);
      return;
    }
    }
  } else {
    raise_cp_exception(cpu, CoprocessorUnusable, 0);
    return;
  }
    break;
  case OP_COP1: /* 0x11 */
    raise_cp_exception(cpu, CoprocessorUnusable, 1);
  return ;
    break;
  case OP_COP2: /* 0x12 */
    raise_cp_exception(cpu, CoprocessorUnusable, 2);
  return ;
    break;
  case OP_COP3: /* 0x13 */
    raise_cp_exception(cpu, CoprocessorUnusable, 3);
  return ;
    break;
  case OP_BEQL: /* 0x14 */
    temp32 = (uint32_t)((I_SIGN_EXTEND_16(instr.immediate) << 2) +
              cpu->next_pc);
    if (READ_REG(cpu, instr.rs) == READ_REG(cpu, instr.rt)) {
      next_next_pc = temp32;
    } else {
      cpu->next_pc = next_next_pc;
      next_next_pc = cpu->next_pc + 4;
    }
    break;
  case OP_BNEL: /* 0x15 */
    temp32 = (uint32_t)((I_SIGN_EXTEND_16(instr.immediate) << 2) +
              cpu->next_pc);
    if (READ_REG(cpu, instr.rs) != READ_REG(cpu, instr.rt)) {
      next_next_pc = temp32;
    } else {
      cpu->next_pc = next_next_pc;
      next_next_pc = cpu->next_pc + 4;
    }
    break;
  case OP_BLEZL: /* 0x16 */
    temp32 = (uint32_t)((I_SIGN_EXTEND_16(instr.immediate) << 2) +
              cpu->next_pc);
    if (((int32_t)READ_REG(cpu, instr.rs)) <= 0 ) {
      next_next_pc = temp32;
    } else {
      cpu->next_pc = next_next_pc;
      next_next_pc = cpu->next_pc + 4;
    }
    break;
  case OP_BGTZL: /* 0x17 */
    temp32 = (uint32_t)((I_SIGN_EXTEND_16(instr.immediate) << 2) +
              cpu->next_pc);
    if (((int32_t)READ_REG(cpu, instr.rs)) > 0 ) {
      next_next_pc = temp32;
    } else {
      cpu->next_pc = next_next_pc;
      next_next_pc = cpu->next_pc + 4;
    }
    break;
  case OP_SPECIAL2: /* 0x1c */
    switch(instr.function) {
    case SPECIAL2_MADD: /* 0x00 */
    {
    int64_t hilo,result;

    result = (int64_t)(int32_t)READ_REG(cpu, instr.rs) *
      (int64_t)(int32_t)READ_REG(cpu, instr.rt);

    hilo = (int64_t)((((uint64_t)READ_REG(cpu, HI)) << 32) +
      (uint64_t)READ_REG(cpu, LO));

    result = hilo + result;

    WRITE_REG(cpu, LO, (uint32_t)result);
    WRITE_REG(cpu, HI, (uint32_t)(result >> 32));
    }
      break;
    case SPECIAL2_MADDU: /* 0x01 */
    {
    uint64_t hilo,result;

    result = (uint64_t)READ_REG(cpu, instr.rs) *
      (uint64_t)READ_REG(cpu, instr.rt);
    hilo = (((uint64_t)READ_REG(cpu, HI)) << 32) +
      ((uint64_t)READ_REG(cpu, LO));
    result = hilo + result;

    WRITE_REG(cpu, LO, (uint32_t)result);
    WRITE_REG(cpu, HI, (uint32_t)(result >> 32));
    }
      break;
    case SPECIAL2_MUL: /* 0x02 */
      {
    int64_t temp;

    temp = (int64_t)((int32_t)READ_REG(cpu,instr.rs)) *
      (int64_t)((int32_t)READ_REG(cpu,instr.rt));

    WRITE_REG(cpu,instr.rd,(uint32_t)temp);
    }
      break;
    case SPECIAL2_MSUB: /* 0x04 */
    {
    int64_t hilo, result;

    result = (int64_t)(int32_t)READ_REG(cpu, instr.rs) *
      (int64_t)(int32_t)READ_REG(cpu, instr.rt);
    hilo = (int64_t)((((uint64_t)READ_REG(cpu, HI)) << 32) +
      (uint64_t)READ_REG(cpu, LO));

    result = hilo - result;

    WRITE_REG(cpu, LO, (uint32_t)result);
    WRITE_REG(cpu, HI, (uint32_t)(result >> 32));
    }
      break;
    case SPECIAL2_MSUBU: /* 0x05 */
      {
    uint64_t hilo,result;

    result = (uint64_t)READ_REG(cpu, instr.rs) *
      (uint64_t)READ_REG(cpu, instr.rt);
    hilo = (((uint64_t)READ_REG(cpu, HI)) << 32) +
      ((uint64_t)READ_REG(cpu, LO));
    result = hilo - result;

    WRITE_REG(cpu, LO, (uint32_t)result);
    WRITE_REG(cpu, HI, (uint32_t)(result >> 32));
    }
      break;
    case SPECIAL2_CLZ: /* 0x20 */
    {
    int i;
    uint32_t reg, count = 0, place = 0x80000000;

    reg = READ_REG(cpu, instr.rs);
    for (i=0; i<32; i++) {
      if (reg & place) break; /* stop if 1 encountered */
      count++;
      place >>= 1;
    }
    WRITE_REG(cpu, instr.rd, count);
    }
      break;
    case SPECIAL2_CLO: /* 0x21 */
    {
    int i;
    uint32_t reg, count = 0, place = 0x80000000;

    /* same as CLZ with inverted register */
    reg = ~READ_REG(cpu, instr.rs);
    for (i=0; i<32; i++) {
      if (reg & place) break; /* stop if 1 encountered */
      count++;
      place >>= 1;
    }
    WRITE_REG(cpu, instr.rd, count);
    }
      break;
    default:
      raise_exception(cpu, ReservedInstruction);
      return;
    }
    break;
  case OP_LB: /* 0x20 */
    temp32 = U_SIGN_EXTEND_16(instr.immediate) +
      READ_REG(cpu, instr.rs);
    exc = mem_read(hardware->memory,
             temp32,
             &temp8, 1, cpu);
    if (exc != NoException) {
      raise_address_exception(cpu, exc,temp32);
      return;
    } else {
      WRITE_REG(cpu, instr.rt, U_SIGN_EXTEND_8(temp8));
    }
    break;
  case OP_LH: /* 0x21 */
    temp32 = U_SIGN_EXTEND_16(instr.immediate) +
      READ_REG(cpu, instr.rs);
    exc = mem_read(hardware->memory,
             temp32,
             &temp16, 2, cpu);
    if (exc != NoException) {
      raise_address_exception(cpu, exc, temp32);
      return;
    } else {
      WRITE_REG(cpu, instr.rt, U_SIGN_EXTEND_16(temp16));
    }
    break;
  case OP_LWL: /* 0x22 */
  {
    uint32_t vAddr = U_SIGN_EXTEND_16(instr.immediate) +
    READ_REG(cpu, instr.rs);

    exc = mem_read(hardware->memory,
         (vAddr & 0xfffffffc),
         &temp32, 4, cpu);
    if(exc != NoException) {
    raise_address_exception(cpu, exc, vAddr);
    return;
    } else {
    uint32_t mask;
    uint32_t sft;

    temp32 = temp32 << ((vAddr & 0x00000003)*8);

    sft = (32-(vAddr & 3)*8);
    mask = ((uint32_t)0xffffffff) >> sft;
    if(sft >= 32)
      mask = 0x00000000;

    WRITE_REG(cpu, instr.rt,
        (READ_REG(cpu, instr.rt) & mask) | temp32);
    }
  }
    break;
  case OP_LW:    /*0x23*/
    {
      uint32_t vAddr = U_SIGN_EXTEND_16(instr.immediate) +
        READ_REG(cpu, instr.rs);
      exc = mem_read(hardware->memory,
               vAddr,
               &temp32, 4, cpu);
      if (exc != NoException) {
        raise_address_exception(cpu, exc, vAddr);
        return;
      } else {
        WRITE_REG(cpu, instr.rt, temp32);
      }
      break;
    }
  case OP_LBU: /* 0x24 */
    temp32 = U_SIGN_EXTEND_16(instr.immediate) +
      READ_REG(cpu, instr.rs);
    exc = mem_read(hardware->memory,
             temp32,
             &temp8, 1, cpu);
    if (exc != NoException) {
      raise_address_exception(cpu, exc, temp32);
      return;
    } else {
      WRITE_REG(cpu, instr.rt, (uint32_t) temp8);
    }
    break;
  case OP_LHU: /* 0x25 */
    temp32 = U_SIGN_EXTEND_16(instr.immediate) +
      READ_REG(cpu, instr.rs);
    exc = mem_read(hardware->memory,
             temp32,
             &temp16, 2, cpu);
    if (exc != NoException) {
      raise_address_exception(cpu, exc, temp32);
      return;
    } else {
      WRITE_REG(cpu, instr.rt, (uint32_t) temp16);
    }
    break;
  case OP_LWR: /* 0x26 */
  {
    uint32_t vAddr = U_SIGN_EXTEND_16(instr.immediate) +
    READ_REG(cpu, instr.rs);

    exc = mem_read(hardware->memory,
         (vAddr & 0xfffffffc),
         &temp32, 4, cpu);
    if(exc != NoException) {
    raise_address_exception(cpu, exc, vAddr);
    return;
    } else {
    uint32_t mask, sft;

    temp32 = temp32 >> (24-(vAddr & 0x00000003)*8);

    sft = 8+(vAddr & 3)*8;
    mask = ((uint32_t)0xffffffff) << sft;
    if(sft >= 32)
      mask = 0x00000000;

    WRITE_REG(cpu, instr.rt,
        (READ_REG(cpu, instr.rt) & mask) | temp32);
    }
  }
    break;
  case OP_SB: /* 0x28 */
  {
    uint32_t vAddr = U_SIGN_EXTEND_16(instr.immediate) +
    READ_REG(cpu, instr.rs);
    temp8 = (uint8_t) READ_REG(cpu, instr.rt);
    exc = mem_write(hardware->memory,
        vAddr,
        &temp8, 1, cpu);
    if (exc != NoException) {
    raise_address_exception(cpu, exc, vAddr);
    return;
    }
    invalidate_rmw_sequences(phys_addr(vAddr & 0xfffffffc, cpu));
  }
    break;
  case OP_SH: /* 0x29 */
  {
    uint32_t vAddr = U_SIGN_EXTEND_16(instr.immediate) +
    READ_REG(cpu, instr.rs);

    temp16 = (uint16_t) READ_REG(cpu, instr.rt);
    exc = mem_write(hardware->memory,
        vAddr,
        &temp16, 2, cpu);
    if (exc != NoException) {
    raise_address_exception(cpu, exc, vAddr);
    return;
    }
    invalidate_rmw_sequences(phys_addr(vAddr & 0xfffffffc, cpu));
  }
    break;
  case OP_SWL: /* 0x2a */
  {
    uint32_t vAddr = U_SIGN_EXTEND_16(instr.immediate) +
    READ_REG(cpu, instr.rs);


    exc = mem_read(hardware->memory,
         (vAddr & 0xfffffffc),
         &temp32, 4, cpu);
    if(exc != NoException) {
    raise_address_exception(cpu, exc, vAddr);
    return;
    } else {
    uint32_t mask, sft;

    sft = 32-(vAddr & 3)*8;
    mask = ((uint32_t)0xffffffff) << sft;
    if(sft >= 32)
      mask = 0x00000000;

    temp32 = temp32 & mask;
    temp32 = temp32 | (READ_REG(cpu, instr.rt) >>
           ((vAddr & 3)*8));

    exc = mem_write(hardware->memory,
        (vAddr & 0xfffffffc),
        &temp32, 4, cpu);
    if(exc != NoException) {
      raise_address_exception(cpu, exc, vAddr);
      return;
    }
    invalidate_rmw_sequences(phys_addr(vAddr & 0xfffffffc, cpu));
    }
  }
    break;
  case OP_SW:    /* 0x2b */
  {
    uint32_t vAddr = U_SIGN_EXTEND_16(instr.immediate) +
    READ_REG(cpu, instr.rs);

    temp32 = READ_REG(cpu, instr.rt);
    exc = mem_write(hardware->memory,
        vAddr,
        &temp32, 4, cpu);
    if (exc != NoException) {
    raise_address_exception(cpu, exc, vAddr);
    return;
    }
    invalidate_rmw_sequences(phys_addr(vAddr, cpu));
  }
    break;
  case OP_SWR: /* 0x2e */
  {
    uint32_t vAddr = U_SIGN_EXTEND_16(instr.immediate) +
    READ_REG(cpu, instr.rs);


    exc = mem_read(hardware->memory,
         (vAddr & 0xfffffffc),
         &temp32, 4, cpu);
    if(exc != NoException) {
    raise_address_exception(cpu, exc, vAddr);
    return;
    } else {
    uint32_t mask,sft;

    sft  = 8+(vAddr & 3)*8;
    mask = ((uint32_t)0xffffffff) >> sft;
    if(sft >= 32)
      mask = 0x00000000;

    temp32 = temp32 & mask;
    temp32 = temp32 | (READ_REG(cpu, instr.rt) <<
           (24-(vAddr & 3)*8));

    exc = mem_write(hardware->memory,
        (vAddr & 0xfffffffc),
        &temp32, 4, cpu);
    if(exc != NoException) {
      raise_address_exception(cpu, exc, vAddr);
      return;
    }
    invalidate_rmw_sequences(phys_addr(vAddr & 0xfffffffc, cpu));
    }
  }
    break;
  case OP_CACHE: /* 0x2f */
    /* nop */
    break;
  case OP_LL: /* 0x30 */
  {
    uint32_t vAddr = U_SIGN_EXTEND_16(instr.immediate) +
    READ_REG(cpu, instr.rs);
    exc = mem_read(hardware->memory,
         vAddr,
         &temp32, 4, cpu);
    if (exc != NoException) {
    raise_address_exception(cpu, exc, vAddr);
    return ;
    } else {
    WRITE_REG(cpu, instr.rt, temp32);
    /* start atomic RMW sequence */
    cpu->cp0->registers[LLAddr] = phys_addr(vAddr, cpu);
    }
  }
    break;
  case OP_LWC1: /* 0x31 */
    raise_cp_exception(cpu, CoprocessorUnusable, 1);
  return ;
    break;
  case OP_LWC2: /* 0x32 */
    raise_cp_exception(cpu, CoprocessorUnusable, 2);
  return ;
    break;
  case OP_PREF: /* 0x33 */
  /* no operation in our hardware (no cache loads)*/
    break;
  case OP_LDC1: /* 0x35 */
    raise_cp_exception(cpu, CoprocessorUnusable, 1);
  return ;
    break;
  case OP_LDC2: /* 0x36 */
    raise_cp_exception(cpu, CoprocessorUnusable, 2);
  return ;
    break;
  case OP_SC: /* 0x38 */
  {
    int atomic;
    uint32_t pAddr, kludge;
    uint32_t vAddr = U_SIGN_EXTEND_16(instr.immediate) +
    READ_REG(cpu, instr.rs);

    /* BEGIN phys_addr kludge */
    /* check for exceptions (no write done) */
    exc = mem_write(hardware->memory, vAddr, &kludge, 0, cpu);
    if (exc != NoException) {
    raise_address_exception(cpu, exc, vAddr);
    return;
    }
    pAddr = phys_addr(vAddr, cpu);
    /* END phys_addr kludge */

    atomic = (cpu->cp0->registers[LLAddr] == pAddr);

    if(atomic) {
    /* LL/SC combination was atomic */

    temp32 = READ_REG(cpu, instr.rt);
    exc = mem_write(hardware->memory,
        vAddr,
        &temp32, 4, cpu);
    if (exc != NoException) {
      /* should not happen (due to the phys_addr kludge) */
      raise_address_exception(cpu, exc, vAddr);
      return;
    }
    /* make all other LL/SC combinations fail */
    invalidate_rmw_sequences(pAddr);

    WRITE_REG(cpu, instr.rt, 1); /* signal success */
    } else {
    /* LL/SC combination was not atomic */
    WRITE_REG(cpu, instr.rt, 0); /* signal failure */
    }
  }
    break;
  case OP_SWC1: /* 0x39 */
    raise_cp_exception(cpu, CoprocessorUnusable, 1);
  return ;
    break;
  case OP_SWC2: /* 0x3a */
    raise_cp_exception(cpu, CoprocessorUnusable, 2);
  return ;
    break;
  case OP_SDC1: /* 0x3d */
    raise_cp_exception(cpu, CoprocessorUnusable, 1);
  return ;
    break;
  case OP_SDC2: /* 0x3e */
    raise_cp_exception(cpu, CoprocessorUnusable, 2);
  return ;
    break;
  default:
    raise_exception(cpu, ReservedInstruction);
    return;
  }

  cpu->registers[PC] = cpu->next_pc;
  cpu->next_pc = next_next_pc;

  if (cpu->registers[PC] == hardware->breakpoint) {
  hardware->running = 0;
  printf("CPU %" PRId32 " hit breakpoint at #%.8" PRIx32 "\n",
       cpu->cpu_id, hardware->breakpoint);
  }
}

void cpu_update(cpu_t *cpu) {
  cpu_next_cycle(cpu);
  cpu_timer_interrupt(cpu);
}
