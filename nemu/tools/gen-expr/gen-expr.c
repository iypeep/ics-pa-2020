//pa1-5表达式求值-如何测试你的代码-要求的功能全部完成
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <assert.h>
#include <string.h>
#include <regex.h>
#include <unistd.h>

// this should be enough
static char buf[65536] = {};
static char code_buf[65536 + 128] = {}; // a little larger than `buf`
static char* code_format =
"#include <stdio.h>\n"
"int main() { \n"
"  int result = %s; \n"
"  printf(\"%%d\\n\", result); \n"
"  return 0; \n"
"}";

//generate a number less than n
int choose(int n) {
   return rand() % n;
}

void gen_num() {//直接copy的
   // 确保 buf 中有足够的空间
   if (strlen(buf) + 100 < sizeof(buf)) {
      char num_str[12];  // 用于存储生成的数字字符串

      // 生成第一个非零数字
      int first_digit = choose(9) + 1;  // 这将生成 1 到 9 之间的数字
      sprintf(num_str, "%d", first_digit);

      // 生成随机长度的数字（例如，可以生成 1 到 2 位的数字）
      int num_length = choose(1) + 1;
      for (int i = 1; i < num_length; ++i) {
         int digit = choose(10);  // 生成 0 到 9 之间的数字
         sprintf(num_str + strlen(num_str), "%d", digit);
      }

      // 将生成的数字字符串添加到 buf
      strcat(buf, num_str);
   }
}


void gen_rand_op() {
   char ops[] = "+-*/";
   if (strlen(buf) + 100 < sizeof(buf)) {
      buf[strlen(buf)] = ops[choose(4)];
   }
}

//下方使用正则表达式过滤"[0-9]+\\("情况
static regex_t regex;
static int ret;
static regmatch_t matchs[2];//保存匹配结果 

void init_regex_add_op() {
   ret = regcomp(&regex, "[0-9]+\\(|\\)[0-9]+|\\)\\(", REG_EXTENDED);
   assert(ret == 0);  // 确保正则表达式编译成功
}
//使用正则表达式判别"[0-9]+\\(|\\)[0-9]+"情况并插入运算符
void adding_rand_op() {
   int offset = 0;
   char ops[] = "+-*";
   init_regex_add_op();
   while (!regexec(&regex, buf + offset, 1, matchs, 0)) {
      memmove(buf + offset + matchs[0].rm_eo, buf + offset + matchs[0].rm_eo, sizeof(buf) - offset - matchs[0].rm_eo);
      buf[offset + matchs[0].rm_eo] = ops[choose(4)];
      offset += matchs[0].rm_eo + 1;
   }
}

//这是一个对buf的调整,随机插入空格,满足对exp.c的调试需求,空格的插入放在code_format运行之后,不然对code_format中result =的运行有影响
void insert_rand_space(char* temp_buf) {
   memset(temp_buf, 0, sizeof(buf)); // 使用 sizeof(buf) 来清空整个缓冲区

   int j = 0; // 临时缓冲区的索引
   for (int i = 0; buf[i] != '\0'; ++i) {
      temp_buf[j++] = buf[i]; // 将原始字符复制到临时缓冲区

      // 以一定概率插入空格
      if (choose(2)) {
         temp_buf[j++] = ' '; // 在字符后插入一个空格
      }
   }

   temp_buf[j] = '\0'; // 确保临时缓冲区以 null 字符结尾
   strcpy(buf, temp_buf); // 将修改后的表达式复制回原始缓冲区
}


//下方是限制最小最大递归深度相关,注意是限制的递归深度,没有对式子长度做直接的限制
static int depth = 0;//如果定义放到函数内部,这个一定要静态
#define MAX_DEPTH 6
#define MIN_DEPTH 5

int parenthesis;

void gen_rand_expr() {
   if (depth >= MAX_DEPTH) {
      gen_num(); // 如果达到最大深度，只生成一个数字
      return;
   }

   int choice;
   if (depth < MIN_DEPTH) {
      // 在没有达到最小深度之前，避免选择只生成数字的选项
      choice = choose(2) + 1;  // 只选择 1 或 2
   }
   else {
      // 当达到或超过最小深度时，可以自由选择任何选项
      static int last_choice;
      do {
         choice = choose(3);
      } while (choice == last_choice);
      last_choice = choice;
   }

   switch (choice) {
   case 0:
      gen_num();
      break;
   case 1:
      strcat(buf, "(");
      parenthesis++;
      depth++;
      gen_rand_expr();
      depth--;
      parenthesis--;
      strcat(buf, ")");
      break;
   default:
      depth++;
      gen_rand_expr();
      depth--;
      gen_rand_op();
      // if we are generating a division operation, make sure the divisor is not zero.
      if (buf[strlen(buf) - 1] == '/') {
         int divisor;
         do {
            divisor = choose(10);
         } while (divisor == 0);
         sprintf(buf + strlen(buf), "%d", divisor);
      }
      else {
         // Only generate a new expression if the last character is not an operator
         depth++;
         gen_rand_expr();
         depth--;
      }
      break;
   }
}

int main(int argc, char* argv[]) {
   int seed = time(0);
   srand(seed);
   int loop = 100;
   if (argc > 1) {
      sscanf(argv[1], "%d", &loop);
   }
   // 重定向 stdout 到一个文件
   freopen("input", "w", stdout);

   int i;
   for (i = 0; i < loop; i++) {
      memset(buf, 0, sizeof(buf));

      gen_rand_expr();

      adding_rand_op();

      sprintf(code_buf, code_format, buf);//Operational Objectives;格式化字符串;符合格式化的内容

      FILE* fp = fopen("/tmp/.code.c", "w");
      assert(fp != NULL);
      fputs(code_buf, fp);
      fclose(fp);

      int ret = system("gcc /tmp/.code.c -o /tmp/.expr");
      if (ret != 0) continue;

      fp = popen("/tmp/.expr", "r");
      assert(fp != NULL);

      int result;
      fscanf(fp, "%d", &result);
      pclose(fp);

      char temp_buf[sizeof(buf)]; // 创建一个临时缓冲区
      insert_rand_space(temp_buf);//为什么放在这里:该函数注释
      printf("%d %s\n", result, buf);
   }
   regfree(&regex);
   // 关闭文件，恢复标准输出
   fclose(stdout);
   return 0;
}
