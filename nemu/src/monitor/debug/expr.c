//通过gen-expr.c测试
#include <isa.h>
#include <stdio.h>
/* We use the POSIX regex functions to process regular expressions.
 * Type 'man regex' for more information about POSIX regex functions.
 */
#include <regex.h>
#include <stdlib.h>
#include <memory/paddr.h>
#include <memory/vaddr.h>

enum {
   TK_NOTYPE = 256,// 空格 256
   TK_PLUS = '+',  // 加号 43
   TK_MINUS = '-', // 减号 45
   TK_STAR = '*',  // 乘号 42
   TK_SLASH = '/', // 除号 47
   TK_NEG = 257,   // 负数 与TK_MINUS减法区别 257
   TK_EQ,          // 等号 258
   TK_LPAR,        // 左括号 259
   TK_RPAR,        // 右括号 260
   TK_NUM,         // 数字 261
   TK_DEREF,       //指针解引用262 

};


static struct rule
{
   char* regex;
   int token_type;
} rules[] = {
    {" +", TK_NOTYPE}, // spaces
    {"\\+", TK_PLUS},  // plus
    {"-", TK_MINUS},   // minus
    {"-", TK_NEG},     // negtive sign
    {"\\*", TK_STAR},  // multiplication
    {"\\*",TK_DEREF},  //pointer dereference未实现
    {"/", TK_SLASH},   // division
    //{"==", TK_EQ},     //equal
    //{"!=",TK_NEQ},     //not equal未实现
    //{"&&",TK_AND},     //and未实现
    {"\\(", TK_LPAR},  // left parenthesis
    {"\\)", TK_RPAR},  // right parenthesis
    {"[0-9]+", TK_NUM} // numbers (at least one digit)
};

#define NR_REGEX (sizeof(rules) / sizeof(rules[0]))

static regex_t re[NR_REGEX] = {}; // regex.h 这是一个计算机可以更高效匹配的内部格式，不能在regex之外用这个，应该用rules作为正则模式

void init_regex_add_op() {//辅助函数:插入随机
   int i;
   char error_msg[128];
   int reti;//return value of regex compile

   for (i = 0; i < NR_REGEX; i++) {
      reti = regcomp(&re[i], rules[i].regex, REG_EXTENDED);
      if (reti != 0) {
         regerror(reti, &re[i], error_msg, 128);
         panic("regex compilation failed: %s\n%s", error_msg, rules[i].regex);
      }
   }
}

typedef struct token
{
   int type;
   char str[32];
} Token;

static Token tokens[1000] __attribute__((used)) = {}; // 用于存放识别过了的字符串
static int nr_token __attribute__((used)) = 0;      // 识别过了的字符串的数量


// Converts an expression string into tokens based on the defined regex rules.
// @param e: String expression to tokenize
// @return: Returns true if tokenization is successful, false otherwise
/* Rules are used for many times.
 * Therefore we compile them only once before any usage.
 */
static bool make_token(char* e) {
   int position = 0;//字符串当前处理位置
   int i;
   regmatch_t pmatch;//匹配结果

   nr_token = 0;
   //e是输入的被判定字符串
   while (e[position] != '\0') {
      /* Try all rules one by one. */
      for (i = 0; i < NR_REGEX; i++) {
         if (regexec(&re[i], e + position, 1, &pmatch, 0) == 0 && pmatch.rm_so == 0) {
            char* substr_start = e + position; // 当前循环中被判定字符的指针
            int substr_len = pmatch.rm_eo;     // 当前循环中被判定字符的长度

            int if_true = 0;//检查if是否执行了内部代码

            Log("match rules[%d] = \"%s\" at position %d with len %d: %.*s",
               i, rules[i].regex, position, substr_len, substr_len, substr_start);

            // 挪动e中指针，针对最外层while循环做改变
            position += substr_len;

            // 检查数组tokens是否已满
            if (nr_token >= sizeof(tokens) / sizeof(Token)) {
               printf("token array is full, cannot insert more\n");
               return false;
            }

            //判断是减号还是负号
            if (rules[i].token_type == TK_MINUS) {//对:  文件开头-,(-,=-  三种情况做判定
               if (nr_token == 0 || tokens[nr_token - 1].type == TK_LPAR || tokens[nr_token - 1].type == TK_EQ) {
                  tokens[nr_token].type = TK_NEG;
                  if_true++;
               }
            }

            //判断DEREFerence引用
            if (rules[i].token_type == TK_STAR) {//对:  tokens开头*,+*,-*,**,/*,(*做判定
               if (i == 0 || tokens[nr_token - 1].type == TK_PLUS || tokens[nr_token - 1].type == TK_MINUS || tokens[nr_token - 1].type == TK_STAR || tokens[nr_token - 1].type == TK_SLASH || tokens[nr_token - 1].type == TK_LPAR) {
                  tokens[nr_token].type = TK_DEREF;
                  if_true++;
               }
            }
            //写入token type到tokens
            if (rules[i].token_type != TK_NOTYPE) {// 抛掉空格
               if (!if_true) { //token类型只设置一次
                  tokens[nr_token].type = rules[i].token_type; // 设置token类型
               }
               // 下方三行  将匹配的子字符串复制到tokens的str字段中

               int length_to_copy = substr_len < sizeof(tokens[nr_token].str) ? substr_len : sizeof(tokens[nr_token].str) - 1;//超出sizeof(tokens[nr_token].str)截断
               strncpy(tokens[nr_token].str, substr_start, length_to_copy);
               tokens[nr_token].str[length_to_copy] = '\0';
               if_true++;
            }
            if (if_true) {
               nr_token++;//如果识别到了token type,则nr_token++
            }
            break;
         }
      }

      if (i == NR_REGEX) {
         printf("no match at position %d\n%s\n%*.s^\n", position, e, position, "");
         return false;
      }
   }
   return true;
}

//检查括号是否有且正确
int check_parentheses(int p, int q) {
   int count = 0;
   int exist = 0;
   for (int i = p; i <= q; i++) {
      if (tokens[i].type == TK_LPAR) {
         exist++;
         count++;
      }
      else if (tokens[i].type == TK_RPAR) {
         exist++;
         count--;
      }
      if (count == 0 && i != q && i != p && exist) {//情况:最外层不由括号包裹,返回主op
         //(4 + 3)* (2 - 1)such
         return i + 1;
      }
      if (count < 0) {//情况:错误,返回-1
         return -1;
      }
   }
   if (!(count == 0 && exist)) {//情况:没有括号,返回0
      return 0;
   }
   return count == 0 && exist;//检查括号是否正确且有
}

//find main operator
int find_main_op(int p, int q) {
   int count = 0;
   int main_op = -1;
   int last_found_mul_div = -1;//用来将*或/限制在右端第一个
   for (int i = q; i != p; i--) {
      if (tokens[i].type == TK_NOTYPE) {
         continue;
      }

      if (tokens[i].type == TK_LPAR) {
         count++;
      }
      if (tokens[i].type == TK_RPAR) {
         count--;
      }

      if (count == 0) {
         if (tokens[i].type == TK_PLUS || tokens[i].type == TK_MINUS) {
            return i;
         }
         if ((tokens[i].type == TK_STAR || tokens[i].type == TK_SLASH) && last_found_mul_div == -1) {
            last_found_mul_div = i;
            main_op = i;
         }
      }
   }
   if (last_found_mul_div != -1) {
      return last_found_mul_div;
   }
   return main_op;
}

int find_next_expr_end(int start, int end) {//辅助函数:判断*引用结束位置
   int level = 0;
   for (int i = start; i <= end; i++) {
      if (tokens[i].type == TK_LPAR) level++;
      if (tokens[i].type == TK_RPAR) level--;
      // 如果我们在顶层找到了一个操作符，就返回它的位置
      if (level == 0 && (tokens[i].type == TK_PLUS || tokens[i].type == TK_MINUS ||
         tokens[i].type == TK_STAR || tokens[i].type == TK_SLASH)) {
         return i - 1;
      }
   }
   return end; // 如果没有找到其他操作符，整个范围都是表达式的一部分
}


//evaluate
//这个函数是通过教案指导的分治法也就是那嵌套的几行exp<>写出来的
int eval(int p, int q) {
   if (p > q) {
      /* Bad expression */
      printf("Bad expression from %d to %d.\n", p, q);
      return -1;
   }
   else if (p == q) {
      /* Single token.
       * For now this token should be a number.
       * Return the value of the number.
       */
      if (tokens[p].type == TK_NUM) {
         return (atoi(tokens[p].str));
      }
      printf("Unkown type %d.\n", tokens[p].type);
      return -1;
   }
   else if (check_parentheses(p, q) == 1) {
      /* The expression is surrounded by a matched pair of parentheses.
       * If that is the case, just throw away the parentheses.
       */
      int result = eval(p + 1, q - 1);
      return result;
   }
   else if (tokens[p].type == TK_NEG) {//判断负号
      return -eval(p + 1, q);
   }
   else if (tokens[p].type == TK_DEREF) {//判断解引用
      // 确保后续有一个表达式
      if (p + 1 <= q) {
         // 需要找到解引用操作符后的表达式的范围
         int next_expr_end = find_next_expr_end(p + 1, q);
         uint32_t addr = eval(p + 1, next_expr_end); // 计算地址
         // ...[地址有效性检查和读取内存的代码]...
         if (PMEM_BASE < addr && addr <= PMEM_BASE + PMEM_SIZE - 1) {
            printf("Invalid memory access at address 0x%08x.\n", addr);
            return -1;
         }
         return vaddr_read(addr, 4);
      }
      else {
         printf("Dereference error at %d.\n", p);
         return -1;
      }
   }
   else {
      int op = find_main_op(p, q);//principal operator
      if (op == -1) {
         int check_par_result = check_parentheses(p, q);
         if (check_par_result == -1) {
            printf("No main operator found from %d to %d.\n", p, q);
            return -1;
         }
         else if (check_par_result > 1) {
            op = check_par_result;
         }
      }
      int val1 = 0, val2 = 0;
      if (op != p) { // 确保左侧表达式存在
         val1 = eval(p, op - 1);
      }
      if (op != q) { // 确保右侧表达式存在
         val2 = eval(op + 1, q);
      }

      switch (tokens[op].type) {
      case TK_PLUS:  return val1 + val2;
      case TK_MINUS: return val1 - val2;
      case TK_STAR:  return val1 * val2;
      case TK_SLASH:
         if (val2 != 0) { return val1 / val2; }
         else {
            printf("Division by zero.\n");
            return -1;
         }
      default: assert(0);
      }
   }
}

//express
word_t expr(char* e, bool* success) {
   if (!make_token(e)) {
      *success = false;
      return 0;
   }

   *success = true; // Set success to true as make_token succeeded
   return eval(0, nr_token - 1);
}

