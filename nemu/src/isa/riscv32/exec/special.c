// special.c 可能包含特殊或少见的指令，可能是为了特定的操作而设计的。例如：
// 系统调用和特权指令
// 自定义扩展指令
// 设备控制指令
#include <cpu/exec.h>
#include <monitor/monitor.h>
#include <monitor/difftest.h>
#include "../local-include/reg.h"

def_EHelper(inv) {
   /* invalid opcode */

   uint32_t instr[2];
   s->seq_pc = cpu.pc;
   instr[0] = instr_fetch(&s->seq_pc, 4);
   instr[1] = instr_fetch(&s->seq_pc, 4);

   printf("invalid opcode(PC = " FMT_WORD ": %08x %08x ...\n\n",
      cpu.pc, instr[0], instr[1]);

   display_inv_msg(cpu.pc);

   rtl_exit(NEMU_ABORT, cpu.pc, -1);

   print_asm("invalid opcode");
}

def_EHelper(nemu_trap) {
   difftest_skip_ref();

   rtl_exit(NEMU_END, cpu.pc, reg_l(10)); // gpr[10] is $a0

   print_asm("nemu trap");
   return;
}
