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

#include <isa.h>
#include "local-include/reg.h"

const char *regs[] = {
    "$0", "ra", "sp", "gp", "tp", "t0", "t1", "t2",
    "s0", "s1", "a0", "a1", "a2", "a3", "a4", "a5",
    "a6", "a7", "s2", "s3", "s4", "s5", "s6", "s7",
    "s8", "s9", "s10", "s11", "t3", "t4", "t5", "t6"};

  void isa_reg_display() {
    int nr_regs = sizeof(regs) / sizeof(regs[0]);
    // 输出标题行，注意各字段的宽度可以根据需要调整
    printf("%-4s   %-12s   %-12s\n", "Reg", "Decimal", "Hexadecimal");
    for (int i = 0; i < nr_regs; i++) {
      // "%-4s" 左对齐寄存器名称（宽度为4），"%12d" 右对齐的10进制数，"0x%08x" 输出8位16进制数
      printf("%-4s : %12d   0x%08x\n", regs[i], cpu.gpr[i], cpu.gpr[i]);
    }
  }

//获取寄存器的值
word_t isa_reg_str2val(const char *s, bool *success)
{
  int nr_regs = sizeof(regs) / sizeof(regs[0]);
  for (int i = 0; i < nr_regs; i++){
    if (strcmp(s, regs[i] ) == 0)
    {
      *success = true;
      return cpu.gpr[i];
    }
  }
  *success = false;
  return 0;
}


//根据状态，获取寄存器名
const char *gain_name(int indx_reg){
  if (indx_reg < 0 || indx_reg >= 32) {
    return NULL; // 返回 NULL 表示无效索引
  }
  return regs[indx_reg];
}
