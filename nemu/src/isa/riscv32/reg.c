#include <isa.h>
#include "local-include/reg.h"

word_t get_reg_value(int i);

const char* regs[] = {
  "$0", "ra", "sp", "gp", "tp", "t0", "t1", "t2",
  "s0", "s1", "a0", "a1", "a2", "a3", "a4", "a5",
  "a6", "a7", "s2", "s3", "s4", "s5", "s6", "s7",
  "s8", "s9", "s10", "s11", "t3", "t4", "t5", "t6"
};

void isa_reg_display() {
   extern riscv32_CPU_state cpu;  // 假设这是你的CPU状态
   for (int i = 0; i < 32; i++) { // 遍历32个通用寄存器
      printf("%s reg[%d] = 0x%x\n", regs[i], i, cpu.gpr[i]._32); // 打印寄存器名和值
   }
   printf("pc = 0x%x\n", cpu.pc); // 打印程序计数器pc
}


word_t isa_reg_str2val(const char* s, bool* success) {
   if (s == NULL || success == NULL) {
      printf("isa_reg_str2val() failed,Illegal input parameters\n");
      return -1;
   }

   for (int i = 0; i < 32; i++) {
      if (strcmp(s, regs[i]) == 0) {
         *success = true;
         return get_reg_value(i);
      }
   }
   if (strcmp(s, "pc") == 0) {
      *success = true;
      return get_reg_value(32); // 对应于pc的特殊索引
   }

   *success = false;
   return -1;
}

word_t get_reg_value(int i) {
   extern riscv32_CPU_state cpu; // 假设这是你的CPU状态
   if (i >= 0 && i < 32) {
      return cpu.gpr[i]._32; // 返回第i个通用寄存器的值
   }
   else if (i == 32) {
      return cpu.pc; // 如果i为32，返回pc的值
   }
   else {
      printf("Illegal Register Index.\n");
      return -1; // 非法寄存器索引
   }
}

