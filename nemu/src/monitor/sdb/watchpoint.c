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

#include "sdb.h"

#define NR_WP 32
#define MAX_BUF 32

typedef struct watchpoint {
  int NO;
  struct watchpoint *next;
  char exp[MAX_BUF];
  word_t old_value;
  /* TODO: Add more members if necessary */

} WP;

static WP wp_pool[NR_WP] = {};
WP *head = NULL, *free_ = NULL;

void init_wp_pool() {
  int i;
  for (i = 0; i < NR_WP; i ++) {
    wp_pool[i].NO = i;
    wp_pool[i].next = (i == NR_WP - 1 ? NULL : &wp_pool[i + 1]);
  }

  head = NULL;
  free_ = wp_pool;
}

WP* new_wp(){
  while (free_){
    WP *p = free_;
    free_ = free_->next;
    return p;
  }
  assert(0);
}


void free_wp(WP *wp){
  wp->next = free_;
  free_ = wp;
}

void set_wp(char *exp){
  WP *wp = new_wp(), *p = head;
  bool success = true;
  strcpy(wp->exp, exp);
  wp->old_value = expr(exp, &success);
  if (!success)
  {
    printf("error in expression\n");
  }
  head = wp;
  wp->next = p;
  printf("set watchpoint %d: %s-----%d\n", wp->NO, exp, wp->old_value);
}


bool check_watchpoint(){
  while ( head != NULL){
    word_t old_value = head->old_value;
    word_t new_value = expr(head->exp, NULL);
    if (old_value != new_value){
      head->old_value = new_value;
      printf("watchpoint %d : %s = %d---%08x\n", head->NO, head->exp,new_value,new_value);
      return true;
    }
  }
  return false;
  }


  bool delete_wp(int NO){
    WP *before = head, *after = head->next;
    if (head->NO == NO){
      head = head->next;
      free_wp(before);
      return true;
    }
    while ( after != NULL){
      if(after->NO == NO){
        before->next = after->next;
        after->next =NULL;
        free_wp(after);
        return true;
      }
      after = after->next;
      before = before->next;
    }
    return false;
  }
/* TODO: Implement the functionality of watchpoint */

