#ifndef __WATCHPOINT_H__
#define __WATCHPOINT_H__

#include <common.h>

typedef struct watchpoint {//watchpoint
   int NO;                          // 监视点的序号
   struct watchpoint* next;         // 下一个监视点的指针
   /* TODO: Add more members if necessary */
   char expr[256];                  // 监视的表达式
   uint32_t old_value;              // 表达式的旧值
   uint32_t new_value;              // 表达式的新值
   bool enabled;                    // 监视点是否启用
} WP;


#endif
