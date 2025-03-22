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
#include <cpu/cpu.h>
#include <readline/readline.h>
#include <readline/history.h>
#include "sdb.h"

static int is_batch_mode = false;

void init_regex();
void init_wp_pool();
void isa_reg_display();
word_t vaddr_read();
word_t expr();
void set_wp();

/* We use the `readline' library to provide more flexibility to read from stdin. */
// 函数rl_gets本身被声明为static，而不是它的返回值。文件作用域
static char *rl_gets()
{
  static char *line_read = NULL;

  if (line_read)
  {
    free(line_read);
    line_read = NULL;
  }
  // 调用 readline 库函数实现交互式输入
  line_read = readline("(nemu) ");

  if (line_read && *line_read)
  {
    add_history(line_read);
  }

  return line_read;
}

static int cmd_si(char *args)
{
  if (args == NULL)
  {
    cpu_exec(1);
    return 0;
  }

  if (atoi(args) <= 0)
  {
    printf("Please input >= 0 number\n");
    return 0;
  }
  cpu_exec(atoi(args));

  return 0;
}

static int cmd_info(char *args)
{
  if (args == NULL || *args != 'r' || *args != 'w'){
    printf("请输入正确的命令\n");
    return 0;
  }

  if ( *args == 'r'){
    isa_reg_display();
  }else {
    cpu_exec(-1);
  }
  return 0;
}

static int cmd_x(char *args)
{
  char *args_end = args + strlen(args);
  char *step = strtok(args, " ");
  char *n_args = step + 3;
  if (n_args >= args_end)
  {
    n_args = NULL;
  }

  if (step != NULL && n_args != NULL)
  {
    int len = atoi(step);
    word_t addr = 0;
    if (sscanf(n_args, "%x", &addr) > 0)
    {
      for (int i = 0; i < len; i++)
      {
        printf("0x%x:%08x\n", addr, vaddr_read(addr, 4));
        addr = addr + 4;
      }
    }
    return 0;
  }
  printf("请输入正确的命令\n");
  return 0;
}

static int cmd_p(char *args)
{

  if (args == NULL){
    printf("请输入正确的命令\n");
    return 0;
  }
  bool success = true;
  word_t result = expr(args, &success);
  if (success){
    printf("表达式%s的值为:%08x\n", args, result);
    return 0;
  }
  printf("词法解析失败\n");
  return 0;
}



static int cmd_w(char *args){
  if (args == NULL){
    printf("请输入正确的命令\n");
    return 0;
  }
  set_wp(args);
  return 0;
}

static int cmd_c(char *args)
{
  cpu_exec(-1);
  return 0;
}

static int cmd_q(char *args)
{
  return -1;
}

static int cmd_help(char *args);

static struct
{
  const char *name;
  const char *description;
  // 函数指针，返回值为int，参数为char *
  int (*handler)(char *);
} cmd_table[] = {
    {"help", "Display information about all supported commands", cmd_help},
    {"c", "Continue the execution of the program", cmd_c},
    {"q", "Exit NEMU", cmd_q},

    /* TODO: Add more commands */
    {"si", "Let the program pause execution after stepping through N instructions", cmd_si},
    {"info", "打印寄存器状态", cmd_info},
    {"x", "打印内存", cmd_x},
    {"p","表达式求值",cmd_p},
    {"w","设置断点",cmd_w},

};

#define NR_CMD ARRLEN(cmd_table)

static int cmd_help(char *args)
{
  /* extract the first argument */
  char *arg = strtok(NULL, " ");
  int i;

  if (arg == NULL)
  {
    /* no argument given */
    for (i = 0; i < NR_CMD; i++)
    {
      printf("%s - %s\n", cmd_table[i].name, cmd_table[i].description);
    }
  }
  else
  {
    for (i = 0; i < NR_CMD; i++)
    {
      if (strcmp(arg, cmd_table[i].name) == 0)
      {
        printf("%s - %s\n", cmd_table[i].name, cmd_table[i].description);
        return 0;
      }
    }
    printf("Unknown command '%s'\n", arg);
  }
  return 0;
}

void sdb_set_batch_mode()
{
  is_batch_mode = true;
}

void sdb_mainloop()
{
  // 检查是否是批处理模式，如果是则直接调用cmd_c()函数执行程序
  if (is_batch_mode)
  {
    cmd_c(NULL);
    return;
  }

  // strlen库函数函数用于计算字符串的长度，不包括字符串的结束符
  for (char *str; (str = rl_gets()) != NULL;)
  {
    char *str_end = str + strlen(str);
    // str_end指向字符串的末尾，str指向字符串的开头

    /* extract the first token as the command */
    // strtok库函数函数用于将字符串分割成多个部分，并以第一个参数作为分隔符，当str中没有分隔符时，strtok 返回整个字符串，即 cmd 指向 "si"
    char *cmd = strtok(str, " ");
    if (cmd == NULL)
    {
      continue;
    }

    /* treat the remaining string as the arguments,
     * which may need further parsing
     */
    char *args = cmd + strlen(cmd) + 1;
    if (args >= str_end)
    {
      args = NULL;
    }

#ifdef CONFIG_DEVICE
    extern void sdl_clear_event_queue();
    sdl_clear_event_queue();
#endif
    // NR_CMD是一个宏，表示cmd_table数组的长度
    // strcmp库函数函数用于比较两个字符串是否相等，如果相等则返回0
    int i;
    for (i = 0; i < NR_CMD; i++)
    {
      if (strcmp(cmd, cmd_table[i].name) == 0)
      {
        if (cmd_table[i].handler(args) < 0)
        {
          return;
        }
        break;
      }
    }

    if (i == NR_CMD)
    {
      printf("Unknown command '%s'\n", cmd);
    }
  }
}

void init_sdb()
{
  /* Compile the regular expressions. */
  init_regex();

  /* Initialize the watchpoint pool. */
  init_wp_pool();
}
/*这段代码定义了一个初始化 SDB（调试器）的函数 init_sdb()，其作用是为调试器的后续工作做好准备。具体步骤如下：
编译正则表达式
调用 init_regex() 函数，主要作用是将调试器中可能用于表达式求值的正则表达式编译处理好，以便后续在解析用户输入或者断点/watchpoint表达式时使用。
初始化断点池（Watchpoint Pool）
调用 init_wp_pool() 函数，用来初始化一个断点池。这个池子保存了所有设置的 watchpoint（监视点），用来跟踪变量或内存值的变化，帮助用户调试程序。
总的来说，init_sdb() 函数完成调试器的基本配置工作，为之后的交互式调试做好了环境配置。*/