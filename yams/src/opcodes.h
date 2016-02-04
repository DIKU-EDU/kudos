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

   $Id: opcodes.h,v 1.2 2002/11/20 12:44:32 ttakanen Exp $
*/

#ifndef YAMS_OPCODES_H
#define YAMS_OPCODES_H

/* OpCode field encodings */

#define OP_SPECIAL 0x00 /* b000000 */
#define OP_REGIMM  0x01 /* b000001 */
#define OP_J       0x02 /* b000010 */
#define OP_JAL     0x03 /* b000011 */
#define OP_BEQ     0x04 /* b000100 */
#define OP_BNE     0x05 /* b000101 */
#define OP_BLEZ    0x06 /* b000110 */
#define OP_BGTZ    0x07 /* b000111 */
#define OP_ADDI    0x08 /* b001000 */
#define OP_ADDIU   0x09 /* b001001 */
#define OP_SLTI    0x0a /* b001010 */
#define OP_SLTIU   0x0b /* b001011 */
#define OP_ANDI    0x0c /* b001100 */
#define OP_ORI     0x0d /* b001101 */
#define OP_XORI    0x0e /* b001110 */
#define OP_LUI     0x0f /* b001111 */
#define OP_COP0    0x10 /* b010000 */
#define OP_COP1    0x11 /* b010001 */
#define OP_COP2    0x12 /* b010010 */
#define OP_COP3    0x13 /* b010011 */
#define OP_BEQL    0x14 /* b010100 */
#define OP_BNEL    0x15 /* b010101 */
#define OP_BLEZL   0x16 /* b010110 */
#define OP_BGTZL   0x17 /* b010111 */

#define OP_SPECIAL2 0x1c /* b011100 */

#define OP_LB      0x20 /* b100000 */
#define OP_LH      0x21 /* b100001 */
#define OP_LWL     0x22 /* b100010 */
#define OP_LW      0x23 /* b100011 */
#define OP_LBU     0x24 /* b100100 */
#define OP_LHU     0x25 /* b100101 */
#define OP_LWR     0x26 /* b100110 */

#define OP_SB      0x28 /* b101000 */
#define OP_SH      0x29 /* b101001 */
#define OP_SWL     0x2a /* b101010 */
#define OP_SW      0x2b /* b101011 */

#define OP_SWR     0x2e /* b101110 */
#define OP_CACHE   0x2f /* b101111 */
#define OP_LL      0x30 /* b110000 */
#define OP_LWC1    0x31 /* b110001 */
#define OP_LWC2    0x32 /* b110010 */
#define OP_PREF    0x33 /* b110011 */

#define OP_LDC1    0x35 /* b110101 */
#define OP_LDC2    0x36 /* b110110 */

#define OP_SC      0x38 /* b111000 */
#define OP_SWC1    0x39 /* b111001 */
#define OP_SWC2    0x3a /* b111010 */

#define OP_SDC1    0x3d /* b111101 */
#define OP_SDC2    0x3e /* b111110 */

/* Special opcode, function field encodings */

#define SPEC_SLL   0x00 /* b000000 */
#define SPEC_MOVC1 0x01 /* b000001 */
#define SPEC_SRL   0x02 /* b000010 */
#define SPEC_SRA   0x03 /* b000011 */
#define SPEC_SLLV  0x04 /* b000100 */

#define SPEC_SRLV    0x06 /* b000110 */
#define SPEC_SRAV    0x07 /* b000111 */
#define SPEC_JR      0x08 /* b001000 */
#define SPEC_JALR    0x09 /* b001001 */
#define SPEC_MOVZ    0x0a /* b001010 */
#define SPEC_MOVN    0x0b /* b001011 */
#define SPEC_SYSCALL 0x0c /* b001100 */
#define SPEC_BREAK   0x0d /* b001101 */

#define SPEC_SYNC    0x0f /* b001111 */
#define SPEC_MFHI    0x10 /* b010000 */
#define SPEC_MTHI    0x11 /* b010001 */
#define SPEC_MFLO    0x12 /* b010010 */
#define SPEC_MTLO    0x13 /* b010011 */

#define SPEC_MULT    0x18 /* b011000 */
#define SPEC_MULTU   0x19 /* b011001 */
#define SPEC_DIV     0x1a /* b011010 */
#define SPEC_DIVU    0x1b /* b011011 */

#define SPEC_ADD     0x20 /* b100000 */
#define SPEC_ADDU    0x21 /* b100001 */
#define SPEC_SUB     0x22 /* b100010 */
#define SPEC_SUBU    0x23 /* b100011 */
#define SPEC_AND     0x24 /* b100100 */
#define SPEC_OR      0x25 /* b100101 */
#define SPEC_XOR     0x26 /* b100110 */
#define SPEC_NOR     0x27 /* b100111 */

#define SPEC_SLT     0x2a /* b101010 */
#define SPEC_SLTU    0x2b /* b101011 */

#define SPEC_TGE     0x30 /* b110000 */
#define SPEC_TGEU    0x31 /* b110001 */
#define SPEC_TLT     0x32 /* b110010 */
#define SPEC_TLTU    0x33 /* b110011 */
#define SPEC_TEQ     0x34 /* b110100 */

#define SPEC_TNE     0x36 /* b110110 */

/* Regimm opcode, rt field encodings  */

#define REGIMM_BLTZ    0x00 /* b00000 */
#define REGIMM_BGEZ    0x01 /* b00001 */
#define REGIMM_BLTZL   0x02 /* b00010 */
#define REGIMM_BGEZL   0x03 /* b00011 */
 
#define REGIMM_TGEI    0x08 /* b01000 */
#define REGIMM_TGEIU   0x09 /* b01001 */
#define REGIMM_TLTI    0x0a /* b01010 */
#define REGIMM_TLTIU   0x0b /* b01011 */
#define REGIMM_TEQI    0x0c /* b01100 */

#define REGIMM_TNEI    0x0e /* b01110 */

#define REGIMM_BLTZAL  0x10 /* b10000 */
#define REGIMM_BGEZAL  0x11 /* b10001 */
#define REGIMM_BLTZALL 0x12 /* b10010 */
#define REGIMM_BGEZALL 0x13 /* b10011 */

/* Special2 opcode, function field encodings */

#define SPECIAL2_MADD  0x00 /* b000000 */
#define SPECIAL2_MADDU 0x01 /* b000001 */
#define SPECIAL2_MUL   0x02 /* b000010 */

#define SPECIAL2_MSUB  0x04 /* b000011 */
#define SPECIAL2_MSUBU 0x05 /* b000100 */

#define SPECIAL2_CLZ   0x20 /* b100000 */
#define SPECIAL2_CLO   0x21 /* b100001 */

/* COP0 opcode, rs field encodings */

#define COP0_MFC0 0x00 /* b00000 */

#define COP0_MTC0 0x04 /* b00100 */

/* COP0 opcode rs field b1xxxxx, function field encodings */

#define COP0_TLBR  0x01 /* b000001 */
#define COP0_TLBWI 0x02 /* b000010 */

#define COP0_TLBWR 0x06 /* b000110 */

#define COP0_TLBP  0x08 /* b001000*/

#define COP0_ERET  0x18 /* b011000 */

#define COP0_WAIT  0x20 /* b100000 */

#endif
