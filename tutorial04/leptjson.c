#ifdef _WINDOWS
#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#endif
//如果编译环境为 Windows，则定义 _CRTDBG_MAP_ALLOC，并包含 <crtdbg.h> 头文件，用于内存泄漏检测。
#include "leptjson.h"//包含该 JSON 库的声明头文件
#include <assert.h>  /* assert() */
#include <errno.h>   /* errno, ERANGE */
#include <math.h>    /* HUGE_VAL */
#include <stdlib.h>  /* NULL, malloc(), realloc(), free(), strtod() */
#include <string.h>  /* memcpy() */
#ifndef LEPT_PARSE_STACK_INIT_SIZE
#define LEPT_PARSE_STACK_INIT_SIZE 256
#endif
//定义初始堆栈大小。如果未事先定义，堆栈大小默认为 256。

#define EXPECT(c, ch)       do { assert(*c->json == (ch)); c->json++; } while(0)//EXPECT: 确认当前字符是否为预期字符，若是，则向前移动。
#define ISDIGIT(ch)         ((ch) >= '0' && (ch) <= '9')//判断字符是否为数字或1到9之间的数字。
#define ISDIGIT1TO9(ch)     ((ch) >= '1' && (ch) <= '9')
#define PUTC(c, ch)         do { *(char*)lept_context_push(c, sizeof(char)) = (ch); } while(0)
//确保宏的使用像函数调用一样，即使在不需要分号的上下文中也能安全使用。
//被调用以确保有足够的空间在堆栈上存储字符，并返回一个指向新空间的指针。
//将字符 ch 存储到通过 lept_context_push 获取的内存地址上。
//
typedef struct {
    const char* json;
    char* stack;
    size_t size, top;
}lept_context;
//一个解析上下文结构体，包含 JSON 字符串、动态堆栈、堆栈大小和当前堆栈顶部的索引。
static void* lept_context_push(lept_context* c, size_t size) {//在解析堆栈中分配指定大小的空间。如果空间不足，将重新分配更大的堆栈。
    void* ret;
    assert(size > 0);
    if (c->top + size >= c->size) {
        if (c->size == 0)
            c->size = LEPT_PARSE_STACK_INIT_SIZE;
        while (c->top + size >= c->size)
            c->size += c->size >> 1;  /* c->size * 1.5 */
        c->stack = (char*)realloc(c->stack, c->size);
    }
    ret = c->stack + c->top;
    //地址加上一个数量（偏移量）通常用于指针运算。这种运算的结果是一个新的地址，它是原地址偏移一定数量之后的位置。
    c->top += size;
    return ret;
}

static void* lept_context_pop(lept_context* c, size_t size) {//从堆栈中弹出指定大小的数据。
    assert(c->top >= size);
    return c->stack + (c->top -= size);
}
static void lept_parse_whitespace(lept_context* c) {//跳过 JSON 字符串中的空白字符。
    const char *p = c->json;
    while (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r')
        p++;
    c->json = p;
}
static int lept_parse_literal(lept_context* c, lept_value* v, const char* literal, lept_type type) {//解析 JSON 中的字面量（如 true、false、null）。
    size_t i;
    EXPECT(c, literal[0]);
    for (i = 0; literal[i + 1]; i++)
        if (c->json[i] != literal[i + 1])
            return LEPT_PARSE_INVALID_VALUE;
    c->json += i;
    v->type = type;
    return LEPT_PARSE_OK;
}

static int lept_parse_number(lept_context* c, lept_value* v) {//解析 JSON 中的数字，并处理可能的数字范围错误。
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
    if (errno == ERANGE && (v->u.n == HUGE_VAL || v->u.n == -HUGE_VAL))//数字过大时会报错
        return LEPT_PARSE_NUMBER_TOO_BIG;
    v->type = LEPT_NUMBER;
    c->json = p;
    return LEPT_PARSE_OK;
}

static const char* lept_parse_hex4(const char* p, unsigned* u) {
    //定义一个静态函数 lept_parse_hex4，接受一个指向字符的指针 p 和一个指向无符号整数的指针 u。
    //lept_parse_hex4 函数解析 4 位十六进制数，将结果存储在 u 指向的变量中，并返回处理过的字符串指针
    //static 关键字表示该函数只在当前源文件内可见。
    //const char* 表示函数返回一个指向常量字符的指针
    //通过每一位的移位和按位或操作，我们可以将每个十六进制字符逐步累加到最终的结果中。
    // 这样，在处理完 \u3A7F 之后，*u 的值就是 0x3A7F，表示这个 Unicode 转义序列对应的字符。
    int i;
    *u = 0;//将 u 指向的变量初始化为 0，准备累加解析出的十六进制值。
    for (i = 0; i < 4; i++) {
        char ch = *p++;
        *u <<= 4;//u 指向的变量左移 4 位，为下一次累加腾出位置。
        if (ch >= '0' && ch <= '9')  *u |= ch - '0';//如果 ch 是数字字符（'0' 到 '9'），将其转换为相应的整数值并累加到 u。
        else if (ch >= 'A' && ch <= 'F')  *u |= ch - ('A' - 10);//: 使用按位或操作将该值添加到 u 中
        else if (ch >= 'a' && ch <= 'f')  *u |= ch - ('a' - 10);
        else return NULL;
    }
    return p;// 返回处理过的字符串指针，即 p 现在指向的是解析完毕后的位置。
}
static void lept_encode_utf8(lept_context* c, unsigned u) {
   //lept_encode_utf8 是一个静态函数，接收一个指向解析上下文 lept_context 的指针 c 和一个无符号整数 u，
   // 代表要编码的 Unicode 码点
    /* \TODO */
    if (u <= 0x7F)
        PUTC(c, u & 0xFF);//u & 0xFF 确保只保留低 8 位（虽然这里并不需要，因为 u 已经小于等于 0x7F）。
    //使用 PUTC 宏将这个字节存储到解析上下文 c 的堆栈中
    else if (u <= 0x7FF) {
        PUTC(c, 0xC0 | ((u >> 6) & 0xFF));//第一个字节格式：110xxxxx，通过 0xC0 | ((u >> 6) & 0xFF) 生成。
        PUTC(c, 0x80 | (u & 0x3F));//0x80 设置前两个比特为 10
    }
    else if (u <= 0xFFFF) {//u 小于等于 0xFFFF，表示它可以用三个字节编码。
        PUTC(c, 0xE0 | ((u >> 12) & 0xFF));//u >> 12 将码点右移 12 位，得到最高位部分,0xE0 设置前四个比特为 1110。
        PUTC(c, 0x80 | ((u >> 6) & 0x3F));//0x80 设置前两个比特为 10,u >> 6 将码点右移 6 位，得到中间部分
        PUTC(c, 0x80 | (u & 0x3F));//u & 0x3F 提取低 6 位,0x80 设置前两个比特为 10.
        //使用 PUTC 宏将这三个字节存储到解析上下文 c 的堆栈中。
    }
    else {
        assert(u <= 0x10FFFF);//u 小于等于 0x10FFFF，表示它可以用四个字节编码。使用 assert 确保 u 在有效的 Unicode 范围内
        PUTC(c, 0xF0 | ((u >> 18) & 0xFF));//u >> 18 将码点右移 18 位，得到最高位部分。
        PUTC(c, 0x80 | ((u >> 12) & 0x3F));
        //通过 0x80 | ((u >> 12) & 0x3F) 生成。
        //u >> 12 将码点右移 12 位，得到次高位部分。
        //0x80 设置前两个比特为 10。
        PUTC(c, 0x80 | ((u >> 6) & 0x3F));
        //通过 0x80 | ((u >> 6) & 0x3F) 生成。
         //u >> 6 将码点右移 6 位，得到中间部分。
         //0x80 设置前两个比特为 10。
        PUTC(c, 0x80 | (u & 0x3F));//u & 0x3F 提取低 6 位。
        //0x80 设置前两个比特为 10。
    }
}

#define STRING_ERROR(ret) do { c->top = head; return ret; } while(0)

static int lept_parse_string(lept_context* c, lept_value* v) {
    size_t head = c->top, len;
    unsigned u, u2;//用于处理 Unicode 转义字符
    const char* p;//p 是指向当前解析位置的指针
    EXPECT(c, '\"');//确保字符串以双引号开始。
    p = c->json;// 将指针设置为当前解析上下文的位置
    for (;;) {//字符串解析循环
        char ch = *p++;
        switch (ch) {
        case '\"':
            len = c->top - head;
            lept_set_string(v, (const char*)lept_context_pop(c, len), len);
            c->json = p;
            return LEPT_PARSE_OK;
        case '\\':
            switch (*p++) {
            case '\"': PUTC(c, '\"'); break;
            case '\\': PUTC(c, '\\'); break;
            case '/':  PUTC(c, '/'); break;
            case 'b':  PUTC(c, '\b'); break;
            case 'f':  PUTC(c, '\f'); break;
            case 'n':  PUTC(c, '\n'); break;
            case 'r':  PUTC(c, '\r'); break;
            case 't':  PUTC(c, '\t'); break;
            case 'u'://处理代理对
                if (!(p = lept_parse_hex4(p, &u)))
                    STRING_ERROR(LEPT_PARSE_INVALID_UNICODE_HEX);
                if (u >= 0xD800 && u <= 0xDBFF) { /* surrogate pair */
                    //如果 u 落在高代理对范围 (0xD800 到 0xDBFF)，则可能需要处理代理对。
                    if (*p++ != '\\')
                        STRING_ERROR(LEPT_PARSE_INVALID_UNICODE_SURROGATE);
                    if (*p++ != 'u')
                        STRING_ERROR(LEPT_PARSE_INVALID_UNICODE_SURROGATE);
                    //检查接下来的字符是否是 \\u，如果不是，则触发 STRING_ERROR 宏，返回 LEPT_PARSE_INVALID_UNICODE_SURROGATE 错误。
                    if (!(p = lept_parse_hex4(p, &u2)))
                        STRING_ERROR(LEPT_PARSE_INVALID_UNICODE_HEX);
                    //再次调用 lept_parse_hex4 解析低代理对的四个十六进制字符，
                    // 结果存储在 u2 中。如果解析失败，返回 NULL 并触发 STRING_ERROR 宏，
                    // 返回 LEPT_PARSE_INVALID_UNICODE_HEX 错误。
                    if (u2 < 0xDC00 || u2 > 0xDFFF)
                        //u2 是否在低代理对范围(0xDC00 到 0xDFFF)，如果不是，则触发 STRING_ERROR 宏，返回 LEPT_PARSE_INVALID_UNICODE_SURROGATE 错误。
                        STRING_ERROR(LEPT_PARSE_INVALID_UNICODE_SURROGATE);
                    u = (((u - 0xD800) << 10) | (u2 - 0xDC00)) + 0x10000;//组合高低代理对生成一个完整的 Unicode 码点，并存储在 u 中。
                }
                lept_encode_utf8(c, u);
                //调用 lept_encode_utf8 将最终的 Unicode 码点 u 编码为 UTF-8 并存储在解析上下文的堆栈中。
                break;
            default:
                STRING_ERROR(LEPT_PARSE_INVALID_STRING_ESCAPE);
            }
            break;
        case '\0':
            STRING_ERROR(LEPT_PARSE_MISS_QUOTATION_MARK);
        default:
            if ((unsigned char)ch < 0x20)
                STRING_ERROR(LEPT_PARSE_INVALID_STRING_CHAR);
            PUTC(c, ch);
        }
    }
}

static int lept_parse_value(lept_context* c, lept_value* v) {
    //此函数根据当前字符决定使用哪个具体的解析函数来处理 JSON 数据，如解析布尔值、数字、字符串或 null。
    switch (*c->json) {
        case 't':  return lept_parse_literal(c, v, "true", LEPT_TRUE);
        case 'f':  return lept_parse_literal(c, v, "false", LEPT_FALSE);
        case 'n':  return lept_parse_literal(c, v, "null", LEPT_NULL);
        default:   return lept_parse_number(c, v);
        case '"':  return lept_parse_string(c, v);
        case '\0': return LEPT_PARSE_EXPECT_VALUE;
    }
}

int lept_parse(lept_value* v, const char* json) {
    // JSON 解析的主函数。它初始化解析上下文，解析 JSON 字符串，并处理不是单一值的情况（例如，在一个 JSON 文档中解析多个值)
    lept_context c;
    int ret;
    assert(v != NULL);
    c.json = json;
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
    assert(c.top == 0);
    free(c.stack);
    return ret;
}

void lept_free(lept_value* v) {//这个函数用于释放 lept_value 中分配的资源，如字符串。
    assert(v != NULL);
    if (v->type == LEPT_STRING)
        free(v->u.s.s);
    v->type = LEPT_NULL;
}

lept_type lept_get_type(const lept_value* v) {// lept_value 中存储的值的类型。
    assert(v != NULL);
    return v->type;
}

int lept_get_boolean(const lept_value* v) {//这两个函数用于获取和设置布尔值。
    assert(v != NULL && (v->type == LEPT_TRUE || v->type == LEPT_FALSE));
    return v->type == LEPT_TRUE;
}
void lept_set_boolean(lept_value* v, int b) {
    lept_free(v);
    v->type = b ? LEPT_TRUE : LEPT_FALSE;
}
//用于获取和设置 JSON 数字类型的值。
double lept_get_number(const lept_value* v) {
    assert(v != NULL && v->type == LEPT_NUMBER);
    return v->u.n;
}
void lept_set_number(lept_value* v, double n) {
    lept_free(v);
    v->u.n = n;
    v->type = LEPT_NUMBER;
}
//这些函数用于获取和设置 JSON 字符串的值，包括获取字符串长度和内容，以及设置新的字符串值
const char* lept_get_string(const lept_value* v) {
    assert(v != NULL && v->type == LEPT_STRING);
    return v->u.s.s;
}

size_t lept_get_string_length(const lept_value* v) {
    assert(v != NULL && v->type == LEPT_STRING);
    return v->u.s.len;
}

void lept_set_string(lept_value* v, const char* s, size_t len) {
    assert(v != NULL && (s != NULL || len == 0));
    lept_free(v);
    v->u.s.s = (char*)malloc(len + 1);
    memcpy(v->u.s.s, s, len);
    v->u.s.s[len] = '\0';
    v->u.s.len = len;
    v->type = LEPT_STRING;
}
