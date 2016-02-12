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

   $Id: cpu.h,v 1.35 2003/01/29 22:54:52 javirta2 Exp $
*/

#ifndef CPU_H
#define CPU_H

#include "cpuregs.h"
#include "cp0.h"
#include "cpu_defs.h"

typedef struct {
  uint32_t instr;
  uint8_t opcode : 6;
  uint8_t rs : 5;
  uint8_t rt : 5;
  uint8_t rd : 5 ;
  uint8_t sa : 5;
  uint32_t instr_index : 26;
  uint16_t immediate;
  uint8_t function : 6;
} instr_t;

/* Initialize the cpu_t struct */
cpu_t *cpu_init(uint32_t id);

/* Deallocate a cpu_t struct */
void cpu_destroy(cpu_t *cpu);

/* Execute next instruction */
void cpu_update(cpu_t *cpu);

/* Disassemble instruction word into a string */
void disasm_instruction(uint32_t addr, uint32_t instr, char *buf, int len);

/* Exception types*/

/* Select a CPU in round-robin manner */
int select_cpu_for_irq();

/*
#define WRITE_REG(cpuptr, reg, value) \
  {if ((reg) != zero) \
     (cpuptr)->registers[(reg)] = (value); \
  }
*/
#define WRITE_REG(cpuptr, reg, value) \
(((reg) != (unsigned int)zero) ? ((cpuptr)->registers[(reg)] = (value)) : 0)

#define READ_REG(cpuptr, reg) \
  ((cpuptr)->registers[(reg)])

/* Unsigned sign extension */
#define U_SIGN_EXTEND_8(value8bit) \
  ((uint32_t) ((int32_t) ((int8_t) (value8bit))))

#define U_SIGN_EXTEND_16(value16bit) \
  ((uint32_t) ((int32_t) ((int16_t) (value16bit))))

/* Signed sign extension */
#define I_SIGN_EXTEND_8(value8bit) \
  ((int32_t) ((int8_t) (value8bit)))

#define I_SIGN_EXTEND_16(value16bit) \
  ((int32_t) ((int16_t) (value16bit)))

#define RAISE_INTERRUPT(cpu, int_nro) {\
    hardware->cpus[cpu]->cp0->registers[Cause] = \
  hardware->cpus[cpu]->cp0->registers[Cause] | \
  (((uint32_t) 1) << ((int_nro)+8));}

#define RAISE_HW_INTERRUPT(cpu, int_nro) {\
    hardware->cpus[cpu]->cp0->registers[Cause] = \
  hardware->cpus[cpu]->cp0->registers[Cause] | \
  (((uint32_t) 1) << ((int_nro)+8+2));}


/* Memory access types for TLB translation */

#define MEM_LOAD  0
#define MEM_STORE 1

#endif /* CPU_H */
