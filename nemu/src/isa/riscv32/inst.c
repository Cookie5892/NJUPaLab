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
  TYPE_I, TYPE_U, TYPE_S,
  TYPE_N, // none
  TYPE_UJ,
  TYPE_B,
  TYPE_RE,
};

//SEXT当处理有限位宽的立即数时（例如 12 位、20 位），如果最高位（符号位）为1，
//则需要将其扩展为一个完整的机器字长（例如 32 位或 64 位）的负数表示
#define src1R() do { *src1 = R(rs1); } while (0)
#define src2R() do { *src2 = R(rs2); } while (0)
#define immI() do { *imm = SEXT(BITS(i, 31, 20), 12); } while(0)
#define immU() do { word_t temp = BITS(i, 31, 12) << 12; \
                        *imm = SEXT(temp, 32) ;} while(0)
#define immS() do { word_t temp = (BITS(i, 31, 25) << 5) | (BITS(i, 11, 7)); \
                    *imm = (int32_t)SEXT(temp, 12);  \
                  } while(0)
#define immUJ() do { \
  word_t temp = (BITS(i, 31, 31) << 20) | (BITS(i, 19, 12) << 12)\
  | (BITS(i, 20, 20) << 11) | (BITS(i, 30, 21) << 1) ;\
        *imm = SEXT(temp, 21); \
      } while (0)
#define immB() do { \
  word_t temp = (BITS(i, 31, 31) << 12) | (BITS(i, 7, 7) << 1)  \
                | (BITS(i, 30, 25) << 5) | (BITS(i, 11, 8) << 1); \
                *imm = SEXT(temp, 13); \
} while (0)




static void decode_operand(Decode *s, int *rd, word_t *src1, word_t *src2, word_t *imm, int type) {
  uint32_t i = s->isa.inst;
  int rs1 = BITS(i, 19, 15);
  int rs2 = BITS(i, 24, 20);
  *rd     = BITS(i, 11, 7);
  switch (type) {
    case TYPE_I: src1R();          immI(); break;
    case TYPE_U:                   immU(); break;
    case TYPE_S: src1R(); src2R(); immS(); break;
    case TYPE_UJ:                  immUJ();break;
    case TYPE_B:                   immB(); break;
    case TYPE_RE: src1R(); src2R();         break;
    case TYPE_N: break;
    default: panic("unsupported type = %d", type);
  }
}

static int decode_exec(Decode *s) {
  s->dnpc = s->snpc;

#define INSTPAT_INST(s) ((s)->isa.inst)
#define INSTPAT_MATCH(s, name, type, ... /* execute body */ ) { \
  int rd = 0; \
  word_t src1 = 0, src2 = 0, imm = 0; \
  decode_operand(s, &rd, &src1, &src2, &imm, concat(TYPE_, type)); \
  __VA_ARGS__ ; \
}
  INSTPAT_START();
  INSTPAT("??????? ????? ????? ??? ????? 00101 11", auipc  , U, R(rd) = s->pc + imm);
  INSTPAT("??????? ????? ????? 100 ????? 00000 11", lbu    , I, R(rd) = Mr(src1 + imm, 1));
  INSTPAT("??????? ????? ????? 000 ????? 01000 11", sb     , S, Mw(src1 + imm, 1, src2));

  INSTPAT("???????????? ????? 000 ????? 0010011"  , addi   , I, R(rd) = src1 + imm);
  INSTPAT("???????????????????? ????? 1101111"    , jal    , UJ,R(rd) = s->pc + 4; s->dnpc = s->pc + imm);
  INSTPAT("???????????? ????? 000 ????? 1100111"  , jalr   , I ,s->dnpc = (src1 + imm) &~ 1; if(rd != 0) R(rd) = s->pc + 4);
  INSTPAT("??????? ????? ????? 010 ????? 0100011" , sw     , S ,Mw(src1 + imm, 4, src2));
  INSTPAT("??????? ????? ????? 000 ????? 1100011" , beq    , B ,if(src1 == src2 ) s->dnpc = s->pc + imm);
  INSTPAT("???????????? ????? 010 ????? 0000011"  , lw     , I ,R(rd) = Mr(src1 + imm, 4));
  INSTPAT("0000000 ????? ????? 000 ????? 0110011" , add    , RE,R(rd) = src1 + src2);
  INSTPAT("0100000 ????? ????? 000 ????? 0110011" , sub    , RE,R(rd) = src1 - src2);
  INSTPAT("???????????? ????? 011 ????? 0010011"  , sltiu  , I ,R(rd) = (src1 < imm) ? 1 : 0);
  INSTPAT("??????? ????? ????? 001 ????? 1100011" , bne    , B ,if(src1 != src2) s->dnpc = s->pc + imm);
  INSTPAT("??????? ????? ????? 101 ????? 1100011" , bge    , B ,if(src1 >= src2) s->dnpc = s->pc + imm);
  INSTPAT("0000001 ????? ????? 000 ????? 0110011" , mul    , RE,R(rd) = src1 *src2);
  INSTPAT("0000001 ????? ????? 100 ????? 0110011" , divi   , RE,R(rd) = src1 / src2);
  INSTPAT("???????????????????? ????? 0110111"    , lui    , U ,R(rd) = imm);



  INSTPAT("0000000 00001 00000 000 00000 11100 11", ebreak , N, NEMUTRAP(s->pc, R(10))); // R(10) is $a0
  INSTPAT("??????? ????? ????? ??? ????? ????? ??", inv    , N, INV(s->pc));
  INSTPAT_END();

  R(0) = 0; // reset $zero to 0

  return 0;
}

int isa_exec_once(Decode *s) {
  s->isa.inst = inst_fetch(&s->snpc, 4);    //根据物理地址获取指定长度内存中的指令，返回指令值
  return decode_exec(s);
}
