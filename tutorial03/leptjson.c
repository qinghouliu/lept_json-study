//leptjson.h：自定义的 JSON 解析器头文件。
//<assert.h>：用于定义断言（assert）宏，用于调试时的条件检查。
//<errno.h>：包含了错误码 errno 和错误常量 ERANGE。
//<math.h>：提供了数学运算函数，如 HUGE_VAL。
//<stdlib.h>：包含了内存分配函数 malloc(), realloc(), free()，以及字符串转换函数 strtod()。
//<string.h>：提供了字符串操作函数，如 memcpy()。
#ifdef _WINDOWS
#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#endif
#include "leptjson.h"
#include <assert.h>  /* assert() */
#include <errno.h>   /* errno, ERANGE */
#include <math.h>    /* HUGE_VAL */
#include <stdlib.h>  /* NULL, malloc(), realloc(), free(), strtod() */
#include <string.h>  /* memcpy() */
//定义解析器使用的堆栈初始大小，默认为 256
#ifndef LEPT_PARSE_STACK_INIT_SIZE
#define LEPT_PARSE_STACK_INIT_SIZE 256
#endif

#define EXPECT(c, ch)       do { assert(*c->json == (ch)); c->json++; } while(0)
#define ISDIGIT(ch)         ((ch) >= '0' && (ch) <= '9')
#define ISDIGIT1TO9(ch)     ((ch) >= '1' && (ch) <= '9')
//新增加的
#define PUTC(c, ch)         do { *(char*)lept_context_push(c, sizeof(char)) = (ch); } while(0)
//*(char*)lept_context_push(c, sizeof(char)) = (ch);
//lept_context_push(c, sizeof(char))：调用 lept_context_push 函数，在堆栈中分配足够的空间以存储一个字符，并返回这个空间的指针。
//(char*)：将返回的指针转换为 char* 类型。
//* (char*)... = (ch); ：通过解引用，将字符 ch 存储到新分配的堆栈空间位置中。
//*(char*)：将返回的指针转换为 char 类型指针，并解引用以访问或存储字符。
//(ch)：将 ch 值存入预留的空间中。



//对lept_contex进行扩展，加入了栈
typedef struct {
    const char* json;
    char* stack;
    size_t size, top;
}lept_context;


//lept_context_push 和 lept_context_pop：解析器堆栈的压入。
//static：限制该函数的作用域，仅在当前文件可见。
//void* ：返回值类型，指向在堆栈中分配的空间。
//lept_context* c：指向堆栈管理结构 lept_context 的指针。
//size_t size：指定要分配的字节数


//定义一个返回 void* 指针的静态函数 lept_context_push，用于在 lept_context 堆栈结构中分配空间。
//这段代码通过 lept_context_push 函数动态调整堆栈容量，确保可以始终为新的数据分配足够的空间，并通过 PUTC 宏将字符直接存储到堆栈中。
static void* lept_context_push(lept_context* c, size_t size) {
    void* ret;//定义一个通用指针 ret，用于返回新分配的堆栈空间地址。
    assert(size > 0);//确保传入的 size 大于0，否则触发调试断言错误
    if (c->top + size >= c->size) {//检查当前栈顶位置与所需分配的大小加在一起是否超出当前堆栈的容量
        if (c->size == 0)//如果堆栈当前容量为零，初始化为默认的堆栈容量
            c->size = LEPT_PARSE_STACK_INIT_SIZE;
        while (c->top + size >= c->size)//不断调整堆栈容量直到满足当前分配所需的空间。
            c->size += c->size >> 1;  /* c->size * 1.5 */
        c->stack = (char*)realloc(c->stack, c->size);
        //使用 realloc 重新分配内存，将新的指针赋值给 c->stack。如果堆栈还没有分配过，realloc 会自动分配。
    }
    ret = c->stack + c->top;//获取新的数据块的起始地址，并存储到 ret 变量。
    c->top += size;//更新 c->top 指针位置，指向新的栈顶
    return ret;//返回分配空间的起始地址
}


//弹出操作
static void* lept_context_pop(lept_context* c, size_t size) {
    assert(c->top >= size);//c->top >= size 确保当前栈顶位置 c->top 至少有 size 个字节，以防止栈溢出错误。
    return c->stack + (c->top -= size);//：从栈顶指针 c->top 中减去 size，更新栈顶位置以实现弹出效果。
    //c->stack + ...：计算弹出后栈顶的地址。
}
//解析空白字符
static void lept_parse_whitespace(lept_context* c) {
    const char *p = c->json;
    while (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r')
        p++;
    c->json = p;
}
//进行true 、false、NULL的测试
static int lept_parse_literal(lept_context* c, lept_value* v, const char* literal, lept_type type) {
    size_t i;
    EXPECT(c, literal[0]);
    for (i = 0; literal[i + 1]; i++)
        if (c->json[i] != literal[i + 1])
            return LEPT_PARSE_INVALID_VALUE;
    c->json += i;
    v->type = type;
    return LEPT_PARSE_OK;
}
//对数字进行测试
static int lept_parse_number(lept_context* c, lept_value* v) {
    const char* p = c->json;
    if (*p == '-') p++;
    if (*p == '0') p++;
    else {
        if (!ISDIGIT1TO9(*p)) return LEPT_PARSE_INVALID_VALUE;
        for (p++; ISDIGIT(*p); p++);
    }
    if (*p == '.') {
        p++;
        if (!ISDIGIT(*p)) return LEPT_PARSE_INVALID_VALUE;
        for (p++; ISDIGIT(*p); p++);
    }
    if (*p == 'e' || *p == 'E') {
        p++;
        if (*p == '+' || *p == '-') p++;
        if (!ISDIGIT(*p)) return LEPT_PARSE_INVALID_VALUE;
        for (p++; ISDIGIT(*p); p++);
    }
    errno = 0;
    v->u.n = strtod(c->json, NULL);
    if (errno == ERANGE && (v->u.n == HUGE_VAL || v->u.n == -HUGE_VAL))
        return LEPT_PARSE_NUMBER_TOO_BIG;
    v->type = LEPT_NUMBER;
    c->json = p;
    return LEPT_PARSE_OK;
}
//解析 JSON 字符串类型。
//将解析出的字符串存储到 lept_value 结构中。
//head：记录当前解析堆栈的起始位置。
//len：用于存储解析出的字符串的长度。
//EXPECT(c, '\"')：检查当前位置是否为双引号开头。
//for (;;) { ... }：无限循环，用于遍历字符串中的每个字符。
//char ch = *p++：读取当前字符并将指针 p 向前移动。
//switch (ch) { ... }：根据当前字符进行分支判断。
//case '\"':：遇到双引号表示字符串结束，计算字符串长度并将字符串存储到 lept_value 结构中，然后更新解析器位置并返回解析成功。
//case '\0':：遇到字符串结尾但未找到双引号，恢复解析堆栈并返回缺少双引号的错误。
//default:：遇到普通字符，将字符压入解析堆栈中。
static int lept_parse_string(lept_context* c, lept_value* v) {
    size_t head = c->top, len;
    const char* p;
    EXPECT(c, '\"');//宏（或函数）确保第一个字符是双引号"，表示字符串的开头。验证后移动指针位置。
    p = c->json;//指针p初始化为JSON字符串中的当前位置
    for (;;) {//无限循环，持续解析，直到找到闭合引号或发生错误
        char ch = *p++;//获取当前字符并将指针p移动到下一个位置
        switch (ch) {
        case '\"'://识别到字符串的结尾。
            len = c->top - head;//获取字符的长度
            lept_set_string(v, (const char*)lept_context_pop(c, len), len);
            //原来字符串在c中，经过lept_set_string将c中的放置到v中
            c->json = p;
            return LEPT_PARSE_OK;
        case '\\':
            switch (*p++) {
            case '\"': PUTC(c, '\"'); break;//处理转义的双引号
            case '\\': PUTC(c, '\\'); break;//处理转义的反斜杠
            case '/':  PUTC(c, '/'); break;//处理转义的正斜杠
            case 'b':  PUTC(c, '\b'); break;//处理退格符（\b）
            case 'f':  PUTC(c, '\f'); break;//处理换页符（\f）
            case 'n':  PUTC(c, '\n'); break;//处理换行符（\n）
            case 'r':  PUTC(c, '\r'); break;//处理回车符（\r）
            case 't':  PUTC(c, '\t'); break;//处理制表符（\t）
            default://如果转义序列无效，将堆栈重置（c->top = head），并返回LEPT_PARSE_INVALID_STRING_ESCAPE。
                c->top = head;
                return LEPT_PARSE_INVALID_STRING_ESCAPE;
            }
            break;
        case '\0'://遇到空字符意味着输入已到达末尾但没有找到闭合引号。
            c->top = head;
            return LEPT_PARSE_MISS_QUOTATION_MARK;
        default://检查该字符是否为控制字符（ASCII码小于0x20）
            //如果是，将堆栈重置（c->top = head），并返回LEPT_PARSE_INVALID_STRING_CHAR。
            //否则，使用PUTC将该字符存储到解析堆栈中
            if ((unsigned char)ch < 0x20) {
             //控制字符是指 ASCII 编码值小于 32（0x20）的字符，通常是不可打印字符，包括回车、换行等。它们在 JSON 字符串中被认为是无效的。
                c->top = head;//在堆栈中，重置 c->top 指针，将其指向 head 位置。
                //head 是当前解析堆栈开始解析字符串时的堆栈位置。如果发现无效字符，需要重置到解析开始时的状态。
                return LEPT_PARSE_INVALID_STRING_CHAR;//表示解析过程中遇到了无效的字符串字符（控制字符）。
            }
            PUTC(c, ch);//一个宏（或函数）用于将字符ch推入由c管理的解析堆栈
        }
    }
}

static int lept_parse_value(lept_context* c, lept_value* v) {
    switch (*c->json) {
        case 't':  return lept_parse_literal(c, v, "true", LEPT_TRUE);
        case 'f':  return lept_parse_literal(c, v, "false", LEPT_FALSE);
        case 'n':  return lept_parse_literal(c, v, "null", LEPT_NULL);
        case '"':  return lept_parse_string(c, v);
        case '\0': return LEPT_PARSE_EXPECT_VALUE;
        default:   return lept_parse_number(c, v);
    }
}
//最主要的函数 lept_parse
int lept_parse(lept_value* v, const char* json) {
    lept_context c;
    int ret;
    assert(v != NULL);
    //初始化对lept_context中的数据内容
    c.json = json;//在这里将字符串的值放到C中
    c.stack = NULL;
    c.size = c.top = 0;
    lept_init(v);
    lept_parse_whitespace(&c);
    if ((ret = lept_parse_value(&c, v)) == LEPT_PARSE_OK) {
        lept_parse_whitespace(&c);
        if (*c.json != '\0') {
            v->type = LEPT_NULL;
            ret = LEPT_PARSE_ROOT_NOT_SINGULAR;
        }
    }
    assert(c.top == 0);//在解析结束时堆栈中没有剩余的元素，即 c.top 应该为 0。
    //如果堆栈中还有元素，意味着解析过程中出现了错误，堆栈没有被正确清理。
    // 这个断言的作用是在调试阶段捕获这种不符合预期的情况，帮助排查错误。
    free(c.stack);
   // 释放解析过程中分配的堆栈空间，防止内存泄漏。这里假设解析函数中通过动态分配内存来维护堆栈结构，所以在解析结束后需要手动释放分配的内存。
    return ret;
}
//释放 lept_value 结构中的资源。
void lept_free(lept_value* v) {
    assert(v != NULL);
    if (v->type == LEPT_STRING)
        free(v->u.s.s);
    v->type = LEPT_NULL;
}
//获取 lept_value 结构中的类型
lept_type lept_get_type(const lept_value* v) {
    assert(v != NULL);
    return v->type;
}

//定义一个返回类型为 int 的函数 lept_get_boolean，参数是一个指向 lept_value 类型的指针 v。
int lept_get_boolean(const lept_value* v) {
    /* \TODO */
    assert(v != NULL && (v->type == LEPT_TRUE || v->type == LEPT_FALSE));
    //通过断言确保 v 不为空，并且 v 的类型是布尔值 LEPT_TRUE 或 LEPT_FALSE。
    return v->type == LEPT_TRUE;
    return 0;
}

void lept_set_boolean(lept_value* v, int b) {//定义一个无返回值的函数 lept_set_boolean，参数是指向 lept_value 类型的指针 v 和一个整型值 b
    /* \TODO */
    lept_free(v);
    v->type = b ? LEPT_TRUE : LEPT_FALSE;//b 为非零，表达式结果为 LEPT_TRUE
    //b 为 0，表达式结果为 LEPT_FALSE
}

double lept_get_number(const lept_value* v) {//定义一个返回类型为 double 的函数 lept_get_number，参数是一个指向 lept_value 类型的指针 v。
    assert(v != NULL && v->type == LEPT_NUMBER);
    return v->u.n;
}

void lept_set_number(lept_value* v, double n) {
    //定义一个无返回值的函数 lept_set_number，参数是指向 lept_value 类型的指针 v 和一个 double 类型的数值 n
    /* \TODO */
    lept_free(v);
    v->u.n = n;
    v->type = LEPT_NUMBER;
}

const char* lept_get_string(const lept_value* v) {
    //定义一个返回类型为指向 char 类型的常量指针的函数 lept_get_string，参数是一个指向 lept_value 类型的指针 v。
    assert(v != NULL && v->type == LEPT_STRING);
    return v->u.s.s;
}

size_t lept_get_string_length(const lept_value* v) {
    //定义一个返回类型为 size_t 的函数 lept_get_string_length，参数是一个指向 lept_value 类型的指针 v。
    assert(v != NULL && v->type == LEPT_STRING);
    return v->u.s.len;
}

void lept_set_string(lept_value* v, const char* s, size_t len) {
    //定义一个无返回值的函数 lept_set_string，参数是指向 lept_value 类型的指针 v，以及一个指向 char 类型的常量指针 s 和一个字符串长度 len。
    assert(v != NULL && (s != NULL || len == 0));
    //通过断言确保 v 不为空，并且 s 为非空，或者 len 为 0。
    lept_free(v);//调用 lept_free 函数释放 v 所占用的资源。
    v->u.s.s = (char*)malloc(len + 1);//为 v 的字符串字段 u.s.s 分配内存，长度为 len + 1 字节。
    memcpy(v->u.s.s, s, len);//将输入字符串 s 的内容复制到 v 的字符串字段 u.s.s 中，复制的长度为 len。
    v->u.s.s[len] = '\0';//在 u.s.s 的最后一位添加字符串终止符 \0
    v->u.s.len = len;//设置 v 的字符串长度字段 u.s.len 为 len
    v->type = LEPT_STRING;//将 v 的类型设置为字符串 LEPT_STRING。
}
