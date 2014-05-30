// Contributed by Jia Xu
//                Te-Kang Chao
//
// The ESESC/BSD License
//
// Copyright (c) 2005-2013, Regents of the University of California and 
// the ESESC Project.
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
//   - Redistributions of source code must retain the above copyright notice,
//   this list of conditions and the following disclaimer.
//
//   - Redistributions in binary form must reproduce the above copyright
//   notice, this list of conditions and the following disclaimer in the
//   documentation and/or other materials provided with the distribution.
//
//   - Neither the name of the University of California, Santa Cruz nor the
//   names of its contributors may be used to endorse or promote products
//   derived from this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.

#include "MIPSCrack.h"
#include "crack_scqemu.h"
#include <iostream>

#ifdef DEBUG
#define OOPS(x) do { static int conta=0; if (conta<1) { printf(x); conta++; }; }while(0)
#else
#define OOPS(x) 
#endif


//Cracks MIPS instructions into SCOORE uOps.
void MIPSCrack::expand(RAWDInst *rinst)
{

  settransType(MIPS);

  // MIPS instructions are not in ITBlock
  rinst->setInITBlock(false);

  rinst->clearInst();

  // decodes 32 bit instructions to each field
  uint32_t insn             = rinst->getInsn();
  uint8_t opcode            = (insn >> 26) & 0x3F;
  uint8_t funct             = insn & 0x3F;
  uint8_t rs                = (insn >> 21) & 0x1F;
  uint8_t rt                = (insn >> 16) & 0x1F;
  uint8_t rd                = (insn >> 11) & 0x1F;             
  uint8_t shift             = (insn >> 6)  & 0x1F;
  uint16_t shift_j          = (insn >> 11) & 0x3FF;

  // immediate or address
  uint32_t IMM16             = insn & 0xFF;
  uint32_t TARGET28          = (insn & 0x3FFFFFF) << 2;

  // some codes for funct field
  const uint8_t JR    = 0x08;
  const uint8_t MULT  = 0x18;
  const uint8_t DIV   = 0x1a;
  const uint8_t ADD   = 0x20;
  const uint8_t ADDU  = 0x21;
  const uint8_t SUB   = 0x22;
  const uint8_t SUBU  = 0x23;
  const uint8_t AND   = 0x24;
  const uint8_t OR    = 0x25;
  const uint8_t XOR   = 0x26;
  const uint8_t NOR   = 0x27;
  


  const uint8_t RA = 31;   // register for return address 
  const uint64_t PC = rinst->getPC();
  
  //printf("insn is %x, opcode is %x, funct is %x, rs is %x, rt is %x, rd is %x, shift is %x\n", insn, opcode, funct, rs, rt, rd, shift);

  // list of common opcode for MIPS
  const  uint8_t MIPS_OP_LB  = 0x20; 
  const  uint8_t MIPS_OP_LW  = 0x23;
  const  uint8_t MIPS_OP_LBU = 0x24;
  const  uint8_t MIPS_OP_SB  = 0x28;
  const  uint8_t MIPS_OP_SW  = 0x2b;
 
  switch(opcode){
    case MIPS_OP_SPECIAL:
      if( !shift_j){
	switch(funct){
	case JR:
	  CrackInst::setup(rinst, iBALU_RJUMP, OP_U64_JMP_REG, rs, 0, 0, 0, 0, 0, 0);
	  break;
	}
      }
      else if( !shift ){
	switch(funct){
	case ADD:
	  CrackInst::setup(rinst, iAALU, OP_S32_ADD, rs, rt, 0, rd, 0, 0, 0);
	  break;
	case ADDU:
	  CrackInst::setup(rinst, iAALU, OP_U32_ADD, rs, rt, 0, rd, 0, 0, 0);
	  break;
        case SUB:
	  CrackInst::setup(rinst, iAALU, OP_S32_SUB, rs, rt, 0, rd, 0, 0, 0);
	  break;
	case SUBU:
	  CrackInst::setup(rinst, iAALU, OP_U32_SUB, rs, rt, 0, rd, 0, 0, 0);
	  break;
	case AND:
	  CrackInst::setup(rinst, iAALU, OP_S64_AND, rs, rt, 0, rd, 0, 0, 0);
	  break;
	case OR:
	  CrackInst::setup(rinst, iAALU, OP_S64_OR, rs, rt, 0, rd, 0, 0, 0);
	  break;
	case XOR:
	  CrackInst::setup(rinst, iAALU, OP_S64_XOR, rs, rt, 0,rd, 0, 0, 0);
	  break;

	}
      }
      break;
    case MIPS_OP_REGIMM:
      if(!rt){  // BLTZ
	CrackInst::setup(rinst, iAALU, OP_U32_SUB, rs, rt, 0, LREG_TMP1, 0, 0, 0);
        CrackInst::setup(rinst, iBALU_LBRANCH, OP_U64_LBL, LREG_TMP1, 0, PC+IMM16<<2, LREG_TMP1, 0, 0, 0);
      }else if(rt == 16){  // BLTZAL
       CrackInst::setup(rinst, iAALU, OP_U32_SUB, rs, 0, 0, LREG_TMP1, 0, 0, 0);
       CrackInst::setup(rinst, iAALU, OP_S64_COPYICC, RA, LREG_TMP1, 0, RA, 0, 0, 0);
       CrackInst::setup(rinst, iBALU_LBRANCH, OP_U64_LBL, LREG_TMP1, 0, PC+IMM16<<2, LREG_TMP1, 0, 0, 0);
       CrackInst::setup(rinst, iAALU, OP_U64_CMOV_G, RA, PC+8, 0, RA, 0, 0, 0);
      }
      break;
    case MIPS_OP_J:
      CrackInst::setup(rinst, iBALU_LJUMP, OP_U64_JMP_IMM, 0, 0, TARGET28, 0, 0, 0, 0);
      break;
    case MIPS_OP_JAL:
      CrackInst::setup(rinst, iAALU, OP_U32_ADD, PC, 0, 8 , RA, 0,0,0);
      CrackInst::setup(rinst, iBALU_LJUMP, OP_U64_JMP_IMM, 0, 0 , TARGET28, 0, 0, 0, 0);
      break;
    case MIPS_OP_BEQ:
      CrackInst::setup(rinst, iAALU, OP_U32_SUB, rs, rt, 0, LREG_TMP1, 0, 0, 0);
      CrackInst::setup(rinst, iBALU_LBRANCH, OP_U64_LBE, LREG_TMP1, 0, PC+IMM16<<2, LREG_TMP1, 0, 0, 0);
      break;
    case MIPS_OP_BNE:
      CrackInst::setup(rinst, iAALU, OP_U32_SUB, rs, rt, 0, LREG_TMP1, 0, 0, 0);
      CrackInst::setup(rinst, iBALU_LBRANCH, OP_U64_LBNE, LREG_TMP1, 0, PC + IMM16 << 2, LREG_TMP1, 0, 0, 0);
      break;
    case MIPS_OP_BLEZ:
      CrackInst::setup(rinst, iAALU, OP_U32_SUB, rs, 0, 0, LREG_TMP1, 0, 0, 0);
      CrackInst::setup(rinst, iBALU_LBRANCH, OP_U64_LBLE, LREG_TMP1, 0, PC + IMM16 << 2, LREG_TMP1, 0, 0, 0);
      break;
    case MIPS_OP_BGTZ:
      CrackInst::setup(rinst, iAALU, OP_U32_SUB, rs, 0, 0, LREG_TMP1, 0, 0, 0);
      CrackInst::setup(rinst, iBALU_LBRANCH, OP_U64_LBG, LREG_TMP1, 0, PC + IMM16 << 2, LREG_TMP1, 0, 0, 0);
      break;
    case MIPS_OP_ADDI:
      CrackInst::setup(rinst, iAALU, OP_S32_ADD, rs, 0, IMM16, rt, 0, 0, 0); 
      break;
    case MIPS_OP_ADDIU:
      CrackInst::setup(rinst, iAALU, OP_U32_ADD, rs, 0, IMM16, rt, 0, 0, 0); 
      break;
    case MIPS_OP_SLTI:
      
      break;
    case MIPS_OP_SLTIU:
      break;
    case MIPS_OP_LW:
      CrackInst::setup(rinst, iAALU, OP_U32_ADD, rs, 0, IMM16, rs, 0, 0, 0);
      CrackInst::setup(rinst, iLALU_LD, OP_U32_LD_L, rs, 0, 0, rt, 0, 0, 0); 
      break; 
    case MIPS_OP_LB:
      CrackInst::setup(rinst, iAALU, OP_U32_ADD, rs, 0, IMM16, rs, 0, 0, 0);
      CrackInst::setup(rinst, iLALU_LD, OP_S08_LD, rs, 0, 0, rt, 0, 0, 0);
    break;
   case MIPS_OP_LBU:
      CrackInst::setup(rinst, iAALU, OP_U32_ADD, rs, 0, IMM16, rs, 0, 0, 0);
      CrackInst::setup(rinst, iLALU_LD, OP_U08_LD, rs, 0, 0, rt, 0, 0, 0);
      break;
  case MIPS_OP_ANDI:
    CrackInst::setup(rinst, iAALU, OP_S64_AND, rs, 0, IMM16, rt, 0, 0, 0);
    break;
  case MIPS_OP_ORI:
    CrackInst::setup(rinst, iAALU, OP_S64_OR, rs, 0, IMM16, rt, 0, 0, 0);
    break;
  case MIPS_OP_XORI:
    CrackInst::setup(rinst, iAALU, OP_S64_XOR, rs, 0, IMM16, rt, 0, 0, 0);
    break;
  case MIPS_OP_SB:
    CrackInst::setup(rinst, iAALU, OP_U32_ADD, rs, 0, IMM16, rs, 0, 0, 0);
    CrackInst::setup(rinst, iLALU_ST, OP_U08_ST, rs, 0, 0, rt, 0, 0, 0);
    break;
  case MIPS_OP_SW:
    CrackInst::setup(rinst, iAALU, OP_U32_ADD, rs, 0, IMM16, rs, 0, 0, 0);
    CrackInst::setup(rinst, iLALU_ST, OP_U32_ST, rs, 0, 0, rt, 0, 0, 0);
  }
}

void MIPSCrack::advPC(){
   pc += 4;
}
