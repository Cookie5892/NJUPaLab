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
void disassemble(char *str, int size, uint64_t pc, uint8_t *code, int nbyte);
static void iringbuf(Decode *s);
char *ftrace(vaddr_t pc);
static void print_ftrace(Decode *s, bool c_or_r);

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
#define immU() do { *imm = BITS(i, 31, 12) << 12; } while(0)
#define immS() do { word_t temp = (BITS(i, 31, 25) << 5) | (BITS(i, 11, 7)); \
                    *imm = SEXT(temp, 12);  \
                  } while(0)
#define immUJ() do { \
  word_t temp = (BITS(i, 31, 31) << 20) | (BITS(i, 19, 12) << 12)\
  | (BITS(i, 20, 20) << 11) | (BITS(i, 30, 21) << 1) ;\
        *imm = SEXT(temp, 21); \
      } while (0)
#define immB() do { \
  word_t temp = (BITS(i, 31, 31) << 12) | (BITS(i, 7, 7) << 11)  \
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
    case TYPE_RE: src1R(); src2R();        break;
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
  INSTPAT("???????????????????? ????? 1101111"    , jal    , UJ,R(rd) = s->pc + 4; s->dnpc = s->pc + imm; print_ftrace(s, true));
  INSTPAT("???????????? ????? 000 ????? 1100111"  , jalr   , I ,s->dnpc = (src1 + imm) & ~ 1; if(rd != 0) R(rd) = s->pc + 4; if (rd == 0) print_ftrace(s, false); if (rd == 1) print_ftrace(s, true));
  INSTPAT("??????? ????? ????? 010 ????? 0100011" , sw     , S ,Mw(src1 + imm, 4, src2));
  INSTPAT("??????? ????? ????? 000 ????? 1100011" , beq    , B ,if(src1 == src2 ) s->dnpc = s->pc + imm);
  INSTPAT("??????? ????? ????? 010 ????? 0000011" , lw     , I ,R(rd) = Mr(src1 + imm, 4));
  INSTPAT("0000000 ????? ????? 000 ????? 0110011" , add    , RE,R(rd) = src1 + src2);
  INSTPAT("0100000 ????? ????? 000 ????? 0110011" , sub    , RE,R(rd) = src1 - src2);
  INSTPAT("???????????? ????? 011 ????? 0010011"  , sltiu  , I ,R(rd) = (src1 < imm) ? 1 : 0);
  INSTPAT("??????? ????? ????? 001 ????? 1100011" , bne    , B ,if(src1 != src2) s->dnpc = s->pc + imm);
  INSTPAT("??????? ????? ????? 101 ????? 1100011" , bge    , B ,if(src1 >= src2) s->dnpc = s->pc + imm);
  INSTPAT("0000001 ????? ????? 000 ????? 0110011" , mul    , RE,R(rd) = src1 *src2);
  INSTPAT("0000001 ????? ????? 100 ????? 0110011" , divi   , RE,R(rd) = src1 / src2);
  INSTPAT("???????????????????? ????? 0110111"    , lui    , U ,R(rd) = imm);
  INSTPAT("??????? ????? ????? 100 ????? 1100011" , blt    , B ,if(src1 < src2) s->dnpc = s->pc + imm);
  INSTPAT("0000000 ????? ????? 010 ????? 0110011" , slt    , RE,R(rd) = (int32_t)src1 < (int32_t)src2 ? 1 : 0);
  INSTPAT("???????????? ????? 111 ????? 0010011"  , andi   , I ,R(rd) = src1 & imm);
  INSTPAT("0000000 ????? ????? 111 ????? 0110011" , and    , RE,R(rd) = src1 & src2);
  INSTPAT("0000001 ????? ????? 110 ????? 0110011" , rem    , RE,R(rd) = (int32_t)src1 % (int32_t)src2);
  INSTPAT("???????????? ????? 001 ????? 0000011"  , lh     , I ,R(rd) = (int16_t)Mr(src1 + imm, 2));
  INSTPAT("???????????? ????? 101 ????? 0000011"  , lhu    , I ,R(rd) = Mr(src1 + imm, 2));
  INSTPAT("0000000 ????? ????? 011 ????? 0110011" , sltu   , RE,R(rd) = src1 < src2 ? 1 : 0);
  INSTPAT("0000000 ????? ????? 100 ????? 0110011" , xor    , RE,R(rd) = src1 ^ src2);
  INSTPAT("0000000 ????? ????? 110 ????? 0110011" , or     , RE,R(rd) = src1 | src2);
  INSTPAT("??????? ????? ????? 001 ????? 01000 11", sb     , S ,Mw(src1 + imm, 2, src2));
  INSTPAT("0100000 ????? ????? 101 ????? 0010011" , sral   , I ,R(rd) = (int32_t)src1 >> (imm & 0x1F));
  INSTPAT("0000000 ????? ????? 001 ????? 0110011" , sll    , RE,R(rd) = src1 << (src2 & 0x1F));
  INSTPAT("???????????? ????? 100 ????? 0010011"  , xort   , I ,R(rd) = src1 ^ imm);
  INSTPAT("0000000 ????? ????? 101 ????? 0010011" , srli   , I ,R(rd) = src1 >> (imm & 0x1F));
  INSTPAT("??????? ????? ????? 111 ????? 1100011" , bgeq   , B ,if(src1 >= src2) s->dnpc= s->pc + imm);
  INSTPAT("0000000 ????? ????? 001 ????? 0010011" , slli   , I ,R(rd) = src1 << (imm & 0x1F));

  INSTPAT("0000000 00001 00000 000 00000 11100 11", ebreak , N, NEMUTRAP(s->pc, R(10))); // R(10) is $a0
  INSTPAT("??????? ????? ????? ??? ????? ????? ??", inv    , N, INV(s->pc));
  INSTPAT_END();

  R(0) = 0; // reset $zero to 0

  return 0;
}

int isa_exec_once(Decode *s) {
  s->isa.inst = inst_fetch(&s->snpc, 4);    //根据物理地址获取指定长度内存中的指令，返回指令值
  iringbuf(s);
  return decode_exec(s);
}

static void print_ftrace(Decode *s, bool c_or_r){
  char * fun_name = ftrace(s->dnpc);
  static int space_num = 1;
  printf("0x%08x:", s->pc);
  for( int i = 0; i < space_num; i ++){
    putchar(' ');
  } 

  if (c_or_r){
    printf("call [%s@0x%08x]\n", fun_name, s->dnpc);
    space_num += 2;
  }else {
    printf("ret [%s]\n", fun_name);
    space_num = (space_num >= 2) ? space_num -2 : 0 ;
  }
}


#define MAX_iringbuf_log 16
static char iringbuf_log[MAX_iringbuf_log][128] = {0};
static int i = 0;
static void iringbuf(Decode *s) {
  char *p = s->logbuf;
  p += snprintf(p, sizeof(s->logbuf), FMT_WORD ":", s->pc); 
  int ilen = s->snpc - s->pc;
  int j;
  uint8_t *inst = (uint8_t *)&s->isa.inst;
  for (j = ilen - 1; j >= 0; j--) {
    p += snprintf(p, 4, " %02x", inst[j]);
  }
  int ilen_max = MUXDEF(CONFIG_ISA_x86, 8, 4);
  int space_len = ilen_max - ilen;
  if (space_len < 0) space_len = 0;
  space_len = space_len * 3 + 1;
  memset(p, ' ', space_len);
  p += space_len;
  disassemble(p, s->logbuf + sizeof(s->logbuf) - p,
      MUXDEF(CONFIG_ISA_x86, s->snpc, s->pc), (uint8_t *)&s->isa.inst, ilen);

  // 将当前指令信息写入 iringbuf 中
  strncpy(iringbuf_log[i % MAX_iringbuf_log], s->logbuf, sizeof(iringbuf_log[0]) - 1);
  iringbuf_log[i % MAX_iringbuf_log][sizeof(iringbuf_log[0]) - 1] = '\0'; // 确保字符串终止
  i++;
}

//打印iringbuf
void iringbuf_prin(){
  int num_log = i >= MAX_iringbuf_log ? MAX_iringbuf_log : i ;
    for(int j = 0; j < num_log; j++){
      if( j == (i-1) % MAX_iringbuf_log){
        printf("------->%s\n", iringbuf_log[j]);
      }else {
        printf("        %s\n", iringbuf_log[j]);
      }
    }
}