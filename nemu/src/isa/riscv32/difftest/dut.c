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
#include <cpu/difftest.h>
#include "../local-include/reg.h"
const char *gain_name(int indx_reg);

bool isa_difftest_checkregs(CPU_state *ref_r, vaddr_t pc) {
  bool success_dut = true;

  for (int i = 0; i < 32; i++){
    if (cpu.gpr[i] != ref_r->gpr[i]){
      const char *reg_name = gain_name(i);
      if (reg_name != NULL){
        printf("reg %s real: 0x%08x---expectancy: 0x%08x\n", reg_name, cpu.gpr[i], ref_r->gpr[i]);
      }
      success_dut = false;
    }
  }
  if (cpu.pc != ref_r->pc){
    printf("pc real: 0x%08x---expectancy: 0x%08x\n", cpu.pc, ref_r->pc);
    success_dut = false;
  }
  
  return success_dut ? true : false;
}

void isa_difftest_attach() {
}
