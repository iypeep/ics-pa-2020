#include <cpu/exec.h>
#include "rtl.h"

// decode operand helper
#define def_DopHelper(name) \
  void concat(decode_op_, name) (DecodeExecState *s, Operand *op, word_t val, bool load_val)

static inline def_DopHelper(i) {
   op->type = OP_TYPE_IMM;//这里是设置Oprand的种类,这将使得指令对应到其OP_TYPE_ 上 
   op->imm = val;//将指令放入 这里有隐式类型转换 不用考虑op->imm是uint32_t可能和val不匹配的问题

   print_Dop(op->str, OP_STR_SIZE, "%d", op->imm);
}

static inline def_DopHelper(r) {
   op->type = OP_TYPE_REG;//同上 //这里是设置Oprand的种类,这将使得指令对应到其OP_TYPE_ 上 
   op->reg = val;//这里存储寄存器的编号。
   op->preg = &reg_l(val);//preg直接指向word_t val()  &reg_l(val)对相应寄存器取值 

   print_Dop(op->str, OP_STR_SIZE, "%s", reg_name(op->reg));
}

static inline def_DHelper(I) {
   decode_op_r(s, id_src1, s->isa.instr.i.rs1, true);//这里的bool表示是否加载这个值 (但是这里没有用到)
   decode_op_i(s, id_src2, s->isa.instr.i.simm11_0, true);
   decode_op_r(s, id_dest, s->isa.instr.i.rd, false);
}

static inline def_DHelper(U) {//Upper 上移
   decode_op_i(s, id_src1, s->isa.instr.u.imm31_12 << 12, true);
   decode_op_r(s, id_dest, s->isa.instr.u.rd, false);

   print_Dop(id_src1->str, OP_STR_SIZE, "0x%x", s->isa.instr.u.imm31_12);
}

static inline def_DHelper(S) {//Store 存储
   decode_op_r(s, id_src1, s->isa.instr.s.rs1, true);
   sword_t simm = (s->isa.instr.s.simm11_5 << 5) | s->isa.instr.s.imm4_0;
   decode_op_i(s, id_src2, simm, true);
   decode_op_r(s, id_dest, s->isa.instr.s.rs2, true);
}

//下方均为后来添加
static inline def_DHelper(R) {//Register
   // 解码源寄存器1
   decode_op_r(s, id_src1, s->isa.instr.r.rs1, true);
   // 解码源寄存器2
   decode_op_r(s, id_src2, s->isa.instr.r.rs2, true);
   // 解码目标寄存器
   decode_op_r(s, id_dest, s->isa.instr.r.rd, false);
}

static inline def_DHelper(B) {//Branch
   // 解码源寄存器1
   decode_op_r(s, id_src1, s->isa.instr.b.rs1, true);

   // 解码源寄存器2
   decode_op_r(s, id_src2, s->isa.instr.b.rs2, true);

   // B-type 指令立即数字段的位组合
   *s0 = (
      ((s->isa.instr.b.imm11) << 11) |
      ((s->isa.instr.b.imm10_5) << 5) |
      ((s->isa.instr.b.imm4_1) << 1) |
      ((s->isa.instr.b.imm12) << 12)
      );

   // 立即数字段进行符号扩展
   //*s0 = (*s0 << 19) >> 19;//没有考虑负imm的情况:不能使用
   // 对立即数进行符号扩展
   if (*s0 & (1 << 12)) { // 检查最高位，如果为1，说明是负数
      *s0 |= 0xFFFFE000; // 通过或操作将高19位设置为1，进行符号扩展
   }
   else {
      *s0 &= 0x00001FFF; // 如果不是负数，确保只有13位有效，高位清零
   }
   //存放处理好了的立即数  "按照特定的位模式组合并进行符号扩展"了的立即数
   id_dest->imm = *s0;
   id_src1->type = OP_TYPE_IMM;
   print_Dop(id_dest->str, OP_STR_SIZE, "%d", id_dest->imm);
}

static inline def_DHelper(J) {//Jump
   // J-type 指令立即数字段的位组合
   *s0 = (
      (s->isa.instr.j.imm20 << 20) |
      (s->isa.instr.j.imm19_12 << 12) |
      (s->isa.instr.j.imm11 << 11) |
      (s->isa.instr.j.imm10_1 << 1)
      );

   // 立即数字段进行符号扩展
   //*s0 = (*s0 << 11) >> 11;//当前环境不符合JAL要求:不能处理负数 imm20=0 
   // 对立即数进行符号扩展
   if (*s0 & (1 << 20)) { // 检查最高位（即imm20），如果为1，说明是负数
      *s0 |= 0xFFF00000; // 通过或操作将高12位设置为1，进行符号扩展
   }
   else {
      *s0 &= 0x001FFFFF; // 如果不是负数，确保只有21位有效，高位清零
   }

   //存放处理好了的立即数  "按照特定的位模式组合并进行符号扩展"了的立即数
   id_src1->imm = *s0;
   id_src1->type = OP_TYPE_IMM;
   print_Dop(id_src1->str, OP_STR_SIZE, "%d", id_src1->imm);
   // 解码目标寄存器 
   decode_op_r(s, id_dest, s->isa.instr.j.rd, true);
}

static inline def_DHelper(SYST) {//从CSR改为sysytem,因为需要实现ecall
   // 解码源寄存器（rs1）
   decode_op_r(s, id_src1, s->isa.instr.csr.rs1, true); // 从寄存器加载值

   // 设置立即数值为 CSR 地址
   id_src2->type = OP_TYPE_IMM; // 设置操作数类型为立即数
   id_src2->imm = s->isa.instr.csr.csr; // 将立即数值设置为指令中的 CSR 字段
   // 打印出 CSR 地址以便调试
   print_Dop(id_src2->str, OP_STR_SIZE, "0x%x", id_src2->imm);

   // 解码目标寄存器（rd）
   decode_op_r(s, id_dest, s->isa.instr.csr.rd, false); // 不需要加载值
}

static inline def_DHelper(SYSTEM) {
   //孔姐ma
}
