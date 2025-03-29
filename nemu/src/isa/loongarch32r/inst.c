/***************************************************************************************
* Copyright (c) 2014-2024 Zihao Yu, Nanjing University
*
* NEMU is licensed under Mulan PSL v2.
* You can use this software according to the terms and conditions of the Mulan PSL v2.
* You may obtain a copy of Mulan PSL v2 at:
*          http://license.coscl.org.cn/MulanPSL2
*
* THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
* EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
* MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
*
* See the Mulan PSL v2 for more details.
***************************************************************************************/

#include "local-include/reg.h"
#include <cpu/cpu.h>
#include <cpu/ifetch.h>
#include <cpu/decode.h>

#define R(i) gpr(i)
#define Mr vaddr_read
#define Mw vaddr_write

enum {
  TYPE_2RI12, TYPE_1RI20,
  TYPE_N, // none
  TYPE_riscvRI12,
};

#define src1R()  do { *src1 = R(rj); } while (0)
#define simm12() do { *imm = SEXT(BITS(i, 21, 10), 12); } while (0)
#define simm20() do { *imm = SEXT(BITS(i, 24, 5), 20) << 12; } while (0)

//riscv32
//SEXT当处理有限位宽的立即数时（例如 12 位、20 位），如果最高位（符号位）为1，
//则需要将其扩展为一个完整的机器字长（例如 32 位或 64 位）的负数表示
#define riscvsrc1R()  do { *src1 = R(rj); } while (0)
#define riscvsimm12() do  { *imm = SEXT(BITS(i, 31, 20), 12); } while (0)

static void decode_operand(Decode *s, int *rd_, word_t *src1, word_t *src2, word_t *imm, int type) {
  uint32_t i = s->isa.inst;
  int rj = BITS(i, 9, 5);
  *rd_ = BITS(i, 4, 0);
  switch (type) {
    case TYPE_1RI20: simm20(); src1R(); break;
    case TYPE_2RI12: simm12(); src1R(); break;
    case TYPE_N: break;
    default: panic("Unsupport type = %d", type);
  }
}

static void decode_operand_isa32(Decode *s, int *rd_, word_t *src1, word_t *src2, word_t *imm, int type){
  uint32_t i = s->isa.inst;
  int rj = BITS(i, 19, 15);
  *rd_ = BITS(i, 11, 7);
  switch (type) {
    case TYPE_riscvRI12: riscvsimm12(); riscvsrc1R(); break;
    case TYPE_2RI12: simm12(); src1R(); break;
    case TYPE_N: break;
    default: panic("Unsupport type = %d", type);
  }
}



static int decode_exec(Decode *s) {
  s->dnpc = s->snpc;

#define INSTPAT_INST(s) ((s)->isa.inst)
#define INSTPAT_MATCH(s, name, type, ... /* execute body */ ) { \
  int rd = 0; \
  word_t src1 = 0, src2 = 0, imm = 0; \
  decode_operand_isa32(s, &rd, &src1, &src2, &imm, concat(TYPE_, type)); \
  __VA_ARGS__ ; \
}

  INSTPAT_START();  //用于初始化指令模式匹配的环境
  INSTPAT("0001110 ????? ????? ????? ????? ?????" , pcaddu12i, 1RI20 , R(rd) = s->pc + imm);
  INSTPAT("0010100010 ???????????? ????? ?????"   , ld.w     , 2RI12 , R(rd) = Mr(src1 + imm, 4));
  INSTPAT("0010100110 ???????????? ????? ?????"   , st.w     , 2RI12 , Mw(src1 + imm, 4, R(rd)));
  INSTPAT("???????????? ????? ??? ????? 0010011"  , addi     , riscvRI12  , R(rd) = src1 + imm);
  

  INSTPAT("0000 0000 0010 10100 ????? ????? ?????", break    , N     , NEMUTRAP(s->pc, R(4))); // R(4) is $a0
  INSTPAT("????????????????? ????? ????? ?????"   , inv      , N     , INV(s->pc));
  INSTPAT_END();

  R(0) = 0; // reset $zero to 0

  return 0;
}

int isa_exec_once(Decode *s) {
  s->isa.inst = inst_fetch(&s->snpc, 4); //根据物理地址获取指定长度内存中的指令，返回指令值
  return decode_exec(s);
}
