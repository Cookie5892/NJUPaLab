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

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <assert.h>
#include <string.h>
#define MAX_DEPTH 10

// this should be enough
static char buf[65536] = {};
static char code_buf[65536 + 128] = {}; // a little larger than `buf`
static char *code_format =
"#include <stdio.h>\n"
"int main() { "
"  unsigned result = (unsigned)(%s); "
"  printf(\"%%u\", result); "
"  return 0; "
"}";


//生成随机数，将随机数映射到0-n-1的范围
uint32_t choose(uint32_t n){
  uint32_t random_num = random();
  uint32_t result = random_num % n;
  return result;
}


//生成随机数字token
static void gen_num(){
  uint32_t num = labs(choose(100000));    //生成0～4294967296
  static char num_str[32];
  sprintf(num_str, "%d", num);
  strcat(buf,num_str);
}

//随机生成运算符
static void gen_rand_op(){
  switch (choose(4)){
    case 0: 
      strcat(buf,"+");
      break;
    case 1: 
      strcat(buf,"-");
      break;
    case 2: 
      strcat(buf,"*");
      break;
    default: 
      strcat(buf,"/");
      break;
  }
}

//生成'('或')'
static void gen(char paren){
    if( paren == '('){
      strcat(buf,"(");
    } else{
      strcat(buf,")");
    }
}

//随机插入空格
static void blsp(){
  uint32_t num_blsp = choose(5);
  char char_blsp[6] = {0};
  for(int i = 0 ; i < num_blsp ; i++){
    char_blsp[i] = ' ';
  }
  char_blsp[num_blsp] = '\0';
  strcat(buf,char_blsp);
}

//递归表达式的辅助函数
static void gen_rand_expr_helper(int depth){
  if(depth > MAX_DEPTH){
    gen_num();
    return;
  }
switch (choose(3)){
  case 0: 
    blsp();
    gen_num();
    blsp();
    break;
  case 1:
    blsp();
    gen('(');
    blsp();
    gen_rand_expr_helper(depth + 1);
    blsp();
    gen(')');
    blsp();
    break;
  default:
    gen_rand_expr_helper(depth + 1);
    gen_rand_op();
    gen_rand_expr_helper(depth + 1);
    break;
  } 
}


static void gen_rand_expr() {
  buf[0] = '\0';
  gen_rand_expr_helper(0);
}

int main(int argc, char *argv[]) {
  int seed = time(0);
  srand(seed);
  int loop = 1;
  if (argc > 1) {
    sscanf(argv[1], "%d", &loop);
  }
  int i;
  for (i = 0; i < loop; i ++) {
    gen_rand_expr();

    sprintf(code_buf, code_format, buf);

    FILE *fp = fopen("/tmp/.code.c", "w");
    assert(fp != NULL);
    fputs(code_buf, fp);
    fclose(fp);

    int ret = system("gcc /tmp/.code.c -o /tmp/.expr");
    if (ret != 0) continue;           //如果编译失败，则重新生成表达式

    fp = popen("/tmp/.expr", "r");
    assert(fp != NULL);

    int result;
    ret = fscanf(fp, "%d", &result);
    pclose(fp);

    printf("%u %s\n", result, buf);
  }
  return 0;
}
