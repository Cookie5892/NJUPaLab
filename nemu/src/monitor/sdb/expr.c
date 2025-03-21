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

/* We use the POSIX regex functions to process regular expressions.
 * Type 'man regex' for more information about POSIX regex functions.
 */
#include <regex.h>
static int find_main_operator();
static bool check_parentheses();
word_t isa_reg_str2val();
word_t vaddr_read();

enum
{
  TK_NOTYPE = 256,
  TK_EQ = 1,
  TK_PL,
  TK_MI,
  TK_MU,
  TK_DI,
  TK_NUM,
  TK_PAL,
  TK_PAR,
  TK_UEQ,
  TK_HEX,
  TK_REG,
  DEREF,

  /* TODO: Add more token types */

};

static struct rule
{
  const char *regex; // 不能通过指针regex修改它所指向的对象，但指针本身可以指向其他地址
  int token_type;
} rules[] = {

    /* TODO: Add more rules.
     * Pay attention to the precedence level of different rules.
     */

    {" +", TK_NOTYPE},  // spaces
    {"\\+", '+'},       // plus
    {"==", TK_EQ},      // equal
    {"\\-", '-'},       // minus
    {"\\*", '*'},       // multiple
    {"\\/", '/'},       // division
    {"[0-9]+", TK_NUM}, // NUMber
    {"\\(", '('},       // 左括号
    {"\\)", ')'},       // 右括号
    //{"!=", TK_UEQ},     // 不等于
    {"0x[0-9a-fA-F]{8}",TK_HEX}, //十六进制数
    {"\\$[a-z][0-9]",TK_REG}, //寄存器变量
    };

#define NR_REGEX ARRLEN(rules)

static regex_t re[NR_REGEX] = {};

/* Rules are used for many times.
 * Therefore we compile them only once before any usage.
 */
void init_regex()
{
  int i;
  char error_msg[128];
  int ret;

  for (i = 0; i < NR_REGEX; i++)
  {
    ret = regcomp(&re[i], rules[i].regex, REG_EXTENDED);
    if (ret != 0)
    {
      regerror(ret, &re[i], error_msg, 128);
      panic("regex compilation failed: %s\n%s", error_msg, rules[i].regex);
    }
  }
}

typedef struct token
{
  int type;
  char str[32];
} Token;

static Token tokens[32] __attribute__((used)) = {};
static int nr_token __attribute__((used)) = 0; // 这个 GCC 属性用于告知编译器，即使该变量在代码中没有被显式引用，也不要将其优化掉

static bool make_token(char *e)
{
  int position = 0;
  int i;
  regmatch_t pmatch; // regmatch_t是一个结构体，里面来有两个int型变量

  nr_token = 0; // 用来记录当前产生的 token 数量

  while (e[position] != '\0')
  { // 传近来的字符串
    /* Try all rules one by one. */
    for (i = 0; i < NR_REGEX; i++)
    { // rules数组的个数
      if (regexec(&re[i], e + position, 1, &pmatch, 0) == 0 && pmatch.rm_so == 0)
      {
        char *substr_start = e + position;
        int substr_len = pmatch.rm_eo;

        Log("match rules[%d] = \"%s\" at position %d with len %d: %.*s",
            i, rules[i].regex, position, substr_len, substr_len, substr_start);

        position += substr_len; // 更新下一个传进去的字串

        /* TODO: Now a new token is recognized with rules[i]. Add codes
         * to record the token in the array `tokens'. For certain types
         * of tokens, some extra actions should be performed.
         */

        switch (rules[i].token_type)
        {
        case TK_NOTYPE:
          break;
        case TK_NUM:
          tokens[nr_token].type = TK_NUM;
          strncpy(tokens[nr_token].str, substr_start, substr_len);
          tokens[nr_token].str[substr_len] = '\0';
          nr_token++;
          break;
        default:
          tokens[nr_token].type = rules[i].token_type;
          tokens[nr_token].str[0] = rules[i].token_type;
          tokens[nr_token].str[1] = '\0';
          nr_token++;
          break;
        }

        break;
      }
    }

    if (i == NR_REGEX)
    {
      printf("no match at position %d\n%s\n%*.s^\n", position, e, position, "");
      return false;
    }
  }

  return true;
}

static word_t eval(int p, int q)
{
  if (p > q){
    return 0;
  }else if (p == q){
    if(tokens[p].type == TK_REG){
      bool success = false;
      word_t reg_vale = isa_reg_str2val(&tokens[p].str[1],&success);
      if(success){
        return reg_vale;
      }
      printf("获取寄存器的值失败\n");
      return 0;
    }
    return strtol(tokens[p].str, NULL, 0);
  }else if (check_parentheses(p, q) == true){
    return eval(p + 1, q - 1);
  }else if(tokens[p].type == DEREF){
  word_t addr = eval(p + 1, q);
  return vaddr_read(addr, 4);
  } else{
    int op = find_main_operator(p, q);
    char op_type = tokens[op].type;
    word_t val1 = eval(p, op - 1);
    word_t val2 = eval(op + 1, q);

    switch (op_type)
    {
    case '+':
      return val1 + val2;
    case '-':
      return val1 - val2;
    case '*':
      return val1 * val2;
    case '/':
      return val1 / val2;
    case TK_EQ:
        return (val1 == val2 ? 1 :0);
    default:
      assert(0);
    }
  }
}

word_t expr(char *e, bool *success){
  if (!make_token(e))
  {
    *success = false;
    return 0;
  }
  /* TODO: Insert codes to evaluate the expression. */

  for(int i = 0; i < nr_token; i++){
    if((tokens[i].type == '*') && (i == 0 ||tokens[i-1].type == '+' ||tokens[i-1].type == '-' 
    || tokens[i-1].type == '*' || tokens[i-1].type == '/' || tokens[i-1].type == TK_EQ ))
    {
      tokens[i].type = DEREF;
    }
  }
  return eval(0, nr_token - 1);
}

// token表达式中寻找主运算符了
static int find_main_operator(int p, int q)
{
  int op_c = -1;
  bool op = true;
  int j = 0;
  for (int i = p; i <= q; i++)
  {
    if (tokens[i].type == '(' || tokens[i].type == ')')
    {
      j = tokens[i].type == '(' ? j + 1 : j - 1;
    }

    if ((tokens[i].type == '+' || tokens[i].type == '-' || tokens[i].type == '*' || tokens[i].type == '/') &&
        j == 0)
    {
      switch (tokens[i].type)
      {
      case '+':
        op_c = i;
        op = false;
        break;
      case '-':
        op_c = i;
        op = false;
        break;
      case '*':
        if (op)
        {
          op_c = i;
          break;
        }
      case '/':
        if (op)
        {
          op_c = i;
          break;
        }
      default:
        break;
      }
    }
  }
  return op_c;
}

// 判断表达式是否被一对匹配的括号包围着, 同时检查表达式的左右括号是否匹配
static bool check_parentheses(int p, int q)
{
  if (tokens[p].type != '(' || tokens[q].type != ')')
  {
    return false;
  }

  int count = 0;
  for (p = p + 1; p < q; p++)
  {
    if (tokens[p].type == '(')
    {
      count++;
    }

    if (tokens[p].type == ')')
    {
      count--;
    }
    if (count < 0)
      return false;
  }
  return (count == 0);
}