#ifdef _WINDOWS
#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#endif
#include "leptjson.h"
#include <assert.h>  /* assert() */
#include <errno.h>   /* errno, ERANGE */
#include <math.h>    /* HUGE_VAL */
#include <stdio.h>   /* sprintf() */
#include <stdlib.h>  /* NULL, malloc(), realloc(), free(), strtod() */
#include <string.h>  /* memcpy() */

#ifndef LEPT_PARSE_STACK_INIT_SIZE
#define LEPT_PARSE_STACK_INIT_SIZE 256
#endif

#ifndef LEPT_PARSE_STRINGIFY_INIT_SIZE
#define LEPT_PARSE_STRINGIFY_INIT_SIZE 256
#endif

#define EXPECT(c, ch)       do { assert(*c->json == (ch)); c->json++; } while(0)
#define ISDIGIT(ch)         ((ch) >= '0' && (ch) <= '9')
#define ISDIGIT1TO9(ch)     ((ch) >= '1' && (ch) <= '9')
#define PUTC(c, ch)         do { *(char*)lept_context_push(c, sizeof(char)) = (ch); } while(0)
#define PUTS(c, s, len)     memcpy(lept_context_push(c, len), s, len)

typedef struct {
    const char* json;
    char* stack;
    size_t size, top;
}lept_context;

static void* lept_context_push(lept_context* c, size_t size) {//添加元素
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
    c->top += size;
    return ret;
}

static void* lept_context_pop(lept_context* c, size_t size) {//弹出元素
    assert(c->top >= size);
    return c->stack + (c->top -= size);
}

static void lept_parse_whitespace(lept_context* c) {//跳过无意义的字符
    const char* p = c->json;
    while (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r')
        p++;
    c->json = p;
}

static int lept_parse_literal(lept_context* c, lept_value* v, const char* literal, lept_type type) {
    //对 null false true进行重构直接进行解析
    size_t i;
    EXPECT(c, literal[0]);
    for (i = 0; literal[i + 1]; i++)
        if (c->json[i] != literal[i + 1])
            return LEPT_PARSE_INVALID_VALUE;
    c->json += i;
    v->type = type;
    return LEPT_PARSE_OK;
}

static int lept_parse_number(lept_context* c, lept_value* v) {//进行数字的解析
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
//这个函数的目的是从一个字符串中解析四个十六进制字符，并将其转换成一个无符号整数。
static const char* lept_parse_hex4(const char* p, unsigned* u) {
    int i;
    *u = 0;
    for (i = 0; i < 4; i++) {
        char ch = *p++;
        *u <<= 4;
        if (ch >= '0' && ch <= '9')  *u |= ch - '0';
        else if (ch >= 'A' && ch <= 'F')  *u |= ch - ('A' - 10);
        else if (ch >= 'a' && ch <= 'f')  *u |= ch - ('a' - 10);
        else return NULL;
    }
    return p;
}
////这个函数将 Unicode 码点 u 转换成 UTF-8 编码形式，并将每个字节推入管理在上下文 c 中的堆栈。
static void lept_encode_utf8(lept_context* c, unsigned u) {
    if (u <= 0x7F)
        PUTC(c, u & 0xFF);
    else if (u <= 0x7FF) {
        PUTC(c, 0xC0 | ((u >> 6) & 0xFF));
        PUTC(c, 0x80 | (u & 0x3F));
    }
    else if (u <= 0xFFFF) {
        PUTC(c, 0xE0 | ((u >> 12) & 0xFF));
        PUTC(c, 0x80 | ((u >> 6) & 0x3F));
        PUTC(c, 0x80 | (u & 0x3F));
    }
    else {
        assert(u <= 0x10FFFF);
        PUTC(c, 0xF0 | ((u >> 18) & 0xFF));
        PUTC(c, 0x80 | ((u >> 12) & 0x3F));
        PUTC(c, 0x80 | ((u >> 6) & 0x3F));
        PUTC(c, 0x80 | (u & 0x3F));
    }
}

#define STRING_ERROR(ret) do { c->top = head; return ret; } while(0)
// 此宏定义用于处理字符串解析中出现的错误。当调用此宏时，它将栈顶指针(c->top) 重置为 head（即字符串解析开始时的位置），
//然后返回错误代码 ret。这样做是为了丢弃在解析过程中可能已经加入到堆栈的部分数据，保持数据一致性。
static int lept_parse_string_raw(lept_context* c, char** str, size_t* len) {
    size_t head = c->top;
    unsigned u, u2;
    const char* p;
    EXPECT(c, '\"');
    p = c->json;
    for (;;) {
        char ch = *p++;
        switch (ch) {
        case '\"':
            *len = c->top - head;
            *str = lept_context_pop(c, *len);
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
            case 'u':
                if (!(p = lept_parse_hex4(p, &u)))
                    STRING_ERROR(LEPT_PARSE_INVALID_UNICODE_HEX);
                if (u >= 0xD800 && u <= 0xDBFF) { /* surrogate pair */
                    if (*p++ != '\\')
                        STRING_ERROR(LEPT_PARSE_INVALID_UNICODE_SURROGATE);
                    if (*p++ != 'u')
                        STRING_ERROR(LEPT_PARSE_INVALID_UNICODE_SURROGATE);
                    if (!(p = lept_parse_hex4(p, &u2)))
                        STRING_ERROR(LEPT_PARSE_INVALID_UNICODE_HEX);
                    if (u2 < 0xDC00 || u2 > 0xDFFF)
                        STRING_ERROR(LEPT_PARSE_INVALID_UNICODE_SURROGATE);
                    u = (((u - 0xD800) << 10) | (u2 - 0xDC00)) + 0x10000;
                }
                lept_encode_utf8(c, u);
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

static int lept_parse_string(lept_context* c, lept_value* v) {//对其进行重构，和之前的不一样
    int ret;
    char* s;
    size_t len;
    if ((ret = lept_parse_string_raw(c, &s, &len)) == LEPT_PARSE_OK)
        lept_set_string(v, s, len);
    return ret;
}

static int lept_parse_value(lept_context* c, lept_value* v);

static int lept_parse_array(lept_context* c, lept_value* v) {//解析数组元素
    size_t i, size = 0;
    int ret;
    EXPECT(c, '[');
    lept_parse_whitespace(c);
    if (*c->json == ']') {
        c->json++;
        v->type = LEPT_ARRAY;
        v->u.a.size = 0;
        v->u.a.e = NULL;
        return LEPT_PARSE_OK;
    }
    for (;;) {
        lept_value e;
        lept_init(&e);
        if ((ret = lept_parse_value(c, &e)) != LEPT_PARSE_OK)
            break;
        memcpy(lept_context_push(c, sizeof(lept_value)), &e, sizeof(lept_value));
        size++;
        lept_parse_whitespace(c);
        if (*c->json == ',') {
            c->json++;
            lept_parse_whitespace(c);
        }
        else if (*c->json == ']') {
            c->json++;
            v->type = LEPT_ARRAY;
            v->u.a.size = size;
            size *= sizeof(lept_value);
            memcpy(v->u.a.e = (lept_value*)malloc(size), lept_context_pop(c, size), size);
            return LEPT_PARSE_OK;
        }
        else {
            ret = LEPT_PARSE_MISS_COMMA_OR_SQUARE_BRACKET;
            break;
        }
    }
    /* Pop and free values on the stack */
    for (i = 0; i < size; i++)
        lept_free((lept_value*)lept_context_pop(c, sizeof(lept_value)));
    return ret;
}

static int lept_parse_object(lept_context* c, lept_value* v) {//解析对象元素
    size_t i, size;
    lept_member m;
    int ret;
    EXPECT(c, '{');
    lept_parse_whitespace(c);
    if (*c->json == '}') {
        c->json++;
        v->type = LEPT_OBJECT;
        v->u.o.m = 0;
        v->u.o.size = 0;
        return LEPT_PARSE_OK;
    }
    m.k = NULL;
    size = 0;
    for (;;) {
        char* str;
        lept_init(&m.v);
        /* parse key */
        if (*c->json != '"') {
            ret = LEPT_PARSE_MISS_KEY;
            break;
        }
        if ((ret = lept_parse_string_raw(c, &str, &m.klen)) != LEPT_PARSE_OK)
            break;
        memcpy(m.k = (char*)malloc(m.klen + 1), str, m.klen);
        m.k[m.klen] = '\0';
        /* parse ws colon ws */
        lept_parse_whitespace(c);
        if (*c->json != ':') {
            ret = LEPT_PARSE_MISS_COLON;
            break;
        }
        c->json++;
        lept_parse_whitespace(c);
        /* parse value */
        if ((ret = lept_parse_value(c, &m.v)) != LEPT_PARSE_OK)
            break;
        memcpy(lept_context_push(c, sizeof(lept_member)), &m, sizeof(lept_member));
        size++;
        m.k = NULL; /* ownership is transferred to member on stack */
        /* parse ws [comma | right-curly-brace] ws */
        lept_parse_whitespace(c);
        if (*c->json == ',') {
            c->json++;
            lept_parse_whitespace(c);
        }
        else if (*c->json == '}') {
            size_t s = sizeof(lept_member) * size;
            c->json++;
            v->type = LEPT_OBJECT;
            v->u.o.size = size;
            memcpy(v->u.o.m = (lept_member*)malloc(s), lept_context_pop(c, s), s);
            return LEPT_PARSE_OK;
        }
        else {
            ret = LEPT_PARSE_MISS_COMMA_OR_CURLY_BRACKET;
            break;
        }
    }
    /* Pop and free members on the stack */
    free(m.k);
    for (i = 0; i < size; i++) {
        lept_member* m = (lept_member*)lept_context_pop(c, sizeof(lept_member));
        free(m->k);
        lept_free(&m->v);
    }
    v->type = LEPT_NULL;
    return ret;
}

static int lept_parse_value(lept_context* c, lept_value* v) {//解析的主要框架
    switch (*c->json) {
    case 't':  return lept_parse_literal(c, v, "true", LEPT_TRUE);
    case 'f':  return lept_parse_literal(c, v, "false", LEPT_FALSE);
    case 'n':  return lept_parse_literal(c, v, "null", LEPT_NULL);
    default:   return lept_parse_number(c, v);
    case '"':  return lept_parse_string(c, v);
    case '[':  return lept_parse_array(c, v);
    case '{':  return lept_parse_object(c, v);
    case '\0': return LEPT_PARSE_EXPECT_VALUE;
    }
}

int lept_parse(lept_value* v, const char* json) {//解析
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

#if 0
// Unoptimized
static void lept_stringify_string(lept_context* c, const char* s, size_t len) {
    size_t i;
    assert(s != NULL);
    PUTC(c, '"');
    for (i = 0; i < len; i++) {
        unsigned char ch = (unsigned char)s[i];
        switch (ch) {
        case '\"': PUTS(c, "\\\"", 2); break;
        case '\\': PUTS(c, "\\\\", 2); break;
        case '\b': PUTS(c, "\\b", 2); break;
        case '\f': PUTS(c, "\\f", 2); break;
        case '\n': PUTS(c, "\\n", 2); break;
        case '\r': PUTS(c, "\\r", 2); break;
        case '\t': PUTS(c, "\\t", 2); break;
        default:
            if (ch < 0x20) {
                char buffer[7];
                sprintf(buffer, "\\u%04X", ch);
                PUTS(c, buffer, 6);
            }
            else
                PUTC(c, s[i]);
        }
    }
    PUTC(c, '"');
}
#else
//lept_stringify_string 主要用于在 JSON 字符串化过程中处理和转换字符串，包括转义特殊字符
static void lept_stringify_string(lept_context* c, const char* s, size_t len) {
    static const char hex_digits[] = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F' };
    //这个数组 hex_digits 用于后续将特定字符编码为 \uXXXX 格式的16进制表示。
    size_t i, size;//i 用于循环遍历字符串，size 用来计算需要的内存空间
    char* head, * p;
    //head 和 p 是字符指针，分别用来保存字符串的开始位置和当前操作的位置。
    assert(s != NULL);
    p = head = lept_context_push(c, size = len * 6 + 2); /* "\u00xx..." */
    //这行代码调用 lept_context_push 函数，
    //预留足够的空间来存储转义后的字符串。每个字符最多可能需要6个字符的空间（如 \u00xx），再加上两个引号。
    *p++ = '"';// 将开头的引号写入字符串
    for (i = 0; i < len; i++) {//for 循环逐字符处理输入字符串 s
        unsigned char ch = (unsigned char)s[i];
        switch (ch) {
        case '\"': *p++ = '\\'; *p++ = '\"'; break;//双引号 ", 需要转义为 \"
        case '\\': *p++ = '\\'; *p++ = '\\'; break;//反斜杠 \, 需要转义为 
        case '\b': *p++ = '\\'; *p++ = 'b';  break;// 退格符 \b, 需要转义为 \\b
        case '\f': *p++ = '\\'; *p++ = 'f';  break; //换页符 \f, 需要转义为 \\f
        case '\n': *p++ = '\\'; *p++ = 'n';  break;//换行符 \n, 需要转义为 \\n
        case '\r': *p++ = '\\'; *p++ = 'r';  break;//回车符 \r, 需要转义为 \\r
        case '\t': *p++ = '\\'; *p++ = 't';  break;//制表符 \t, 需要转义为 \\t
        default:
            if (ch < 0x20) {//如果字符是控制字符（ASCII 值小于 0x20），则将其转换成 \u00xx 格式。
            //否则，将字符直接加入到结果字符串。
                *p++ = '\\'; *p++ = 'u'; *p++ = '0'; *p++ = '0';
                *p++ = hex_digits[ch >> 4];
                *p++ = hex_digits[ch & 15];
                //unsigned char 的变量 ch。即使 ch 原本表示一个字符，但在 C/C++ 中，
                //unsigned char 实质上是一个 8 位的无符号整数。这使得我们可以对它进行位运算。
                //将字符的 ASCII 值转换为16进制并写入。这里使用位移和掩码操作来得到字符的高位和低位16进制数值。
                //hex_digits[ch >> 4] 获取对应于字节上半部分的十六进制数字。
                //hex_digits[ch & 15] 获取对应于字节下半部分的十六进制数字
            }
            else
                *p++ = s[i];// 如果字符不需要特殊处理，则直接写入到目标字符串中。
        }
    }
    *p++ = '"';//循环结束后，加入闭合引号 *p++ = '"';
    c->top -= size - (p - head);//c->top -= size - (p - head); 更新 lept_context 的 top 属性，减去实际未使用的预留空间。
    // 是一个指向当前操作位置的指针，head 是转义开始时字符串的起始位置。因此，p - head 表示已经用于存储转义字符串的字符数。
}
#endif

static void lept_stringify_value(lept_context* c, const lept_value* v) {
    size_t i;//size_t i; 用于循环中的索引。
    switch (v->type) { //根据 JSON 值的类型进行不同的处理
    case LEPT_NULL:   PUTS(c, "null", 4); break;//如果类型是 LEPT_NULL，输出字符串 "null"。
    case LEPT_FALSE:  PUTS(c, "false", 5); break;//如果类型是 LEPT_FALSE，输出字符串 "false"。
    case LEPT_TRUE:   PUTS(c, "true", 4); break;//如果类型是 LEPT_TRUE，输出字符串 "true"。
    case LEPT_NUMBER: c->top -= 32 - sprintf(lept_context_push(c, 32), "%.17g", v->u.n); break;
    //对于数字类型，调用 sprintf 将数字格式化为字符串（最多 17 位有效数字），并存储到 c 的栈中。调整 top 值以反映实际占用的空间
    case LEPT_STRING: lept_stringify_string(c, v->u.s.s, v->u.s.len); break;
    //调用 lept_stringify_string 函数处理 JSON 字符串值，包括转义等。
    case LEPT_ARRAY://开始数组的处理
        PUTC(c, '[');//输出数组开始的 [ 符号。
        for (i = 0; i < v->u.a.size; i++) {
            if (i > 0)
                PUTC(c, ',');// 在元素之间输出逗号分隔符
            lept_stringify_value(c, &v->u.a.e[i]);//递归调用此函数处理每个数组元素
        }
        PUTC(c, ']');// 输出数组结束的 ] 符号
        break;
    case LEPT_OBJECT:
        PUTC(c, '{');//输出对象开始的 { 符号
        for (i = 0; i < v->u.o.size; i++) {
            if (i > 0)
                PUTC(c, ',');// 在元素之间输出逗号分隔符
            lept_stringify_string(c, v->u.o.m[i].k, v->u.o.m[i].klen);// 处理并输出成员的键
            PUTC(c, ':');// 输出键值对之间的冒号
            lept_stringify_value(c, &v->u.o.m[i].v);//递归调用此函数处理成员的值
        }
        PUTC(c, '}');//输出对象结束的 } 符号
        break;
    default: assert(0 && "invalid type");//如果遇到无效的类型，则触发断言，表示程序出现了预料之外的错误。
    }
}

char* lept_stringify(const lept_value* v, size_t* length) {
    //lept_stringify 是一个用于将 lept_value 结构（代表 JSON 数据）转换成一个字符串的顶层函数
    lept_context c;//lept_context c; 定义一个 lept_context 结构体 c，该结构用于存储字符串化过程中的临时数据和最终的字符串。

    assert(v != NULL);//使用断言确保传入的 v 不是空指针，防止运行时错误

    c.stack = (char*)malloc(c.size = LEPT_PARSE_STRINGIFY_INIT_SIZE);//c.stack 分配初始大小的内存，这块内存用于存储生成的字符串

    c.top = 0;//c.top = 0; 初始化 top 属性为 0，top 用于跟踪当前缓冲区的使用情况（即字符串的当前长度）。

    lept_stringify_value(&c, v);
    //lept_stringify_value(&c, v); 调用 lept_stringify_value 函数，传入上下文 c 和 JSON 值 v，开始转换过程。
    //这个函数负责将 v 中的数据按照 JSON 格式转换成字符串，并存储在 c.stack 中。

    if (length)// 检查 length 指针是否非空
        *length = c.top;//如果非空，通过 length 指针返回生成的字符串的长度
    PUTC(&c, '\0');//在字符串的末尾添加空字符 '\0'，确保返回的字符串是正确终止的 C 字符串。
    return c.stack;//返回包含 JSON 文本的字符串
}

void lept_free(lept_value* v) {
    size_t i;
    assert(v != NULL);
    switch (v->type) {
    case LEPT_STRING:
        free(v->u.s.s);
        break;
    case LEPT_ARRAY:
        for (i = 0; i < v->u.a.size; i++)
            lept_free(&v->u.a.e[i]);
        free(v->u.a.e);
        break;
    case LEPT_OBJECT:
        for (i = 0; i < v->u.o.size; i++) {
            free(v->u.o.m[i].k);
            lept_free(&v->u.o.m[i].v);
        }
        free(v->u.o.m);
        break;
    default: break;
    }
    v->type = LEPT_NULL;
}

lept_type lept_get_type(const lept_value* v) {
    assert(v != NULL);
    return v->type;
}

int lept_get_boolean(const lept_value* v) {
    assert(v != NULL && (v->type == LEPT_TRUE || v->type == LEPT_FALSE));
    return v->type == LEPT_TRUE;
}

void lept_set_boolean(lept_value* v, int b) {
    lept_free(v);
    v->type = b ? LEPT_TRUE : LEPT_FALSE;
}

double lept_get_number(const lept_value* v) {
    assert(v != NULL && v->type == LEPT_NUMBER);
    return v->u.n;
}

void lept_set_number(lept_value* v, double n) {
    lept_free(v);
    v->u.n = n;
    v->type = LEPT_NUMBER;
}

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

size_t lept_get_array_size(const lept_value* v) {
    assert(v != NULL && v->type == LEPT_ARRAY);
    return v->u.a.size;
}

lept_value* lept_get_array_element(const lept_value* v, size_t index) {
    assert(v != NULL && v->type == LEPT_ARRAY);
    assert(index < v->u.a.size);
    return &v->u.a.e[index];
}

size_t lept_get_object_size(const lept_value* v) {
    assert(v != NULL && v->type == LEPT_OBJECT);
    return v->u.o.size;
}

const char* lept_get_object_key(const lept_value* v, size_t index) {
    assert(v != NULL && v->type == LEPT_OBJECT);
    assert(index < v->u.o.size);
    return v->u.o.m[index].k;
}

size_t lept_get_object_key_length(const lept_value* v, size_t index) {
    assert(v != NULL && v->type == LEPT_OBJECT);
    assert(index < v->u.o.size);
    return v->u.o.m[index].klen;
}

lept_value* lept_get_object_value(const lept_value* v, size_t index) {
    assert(v != NULL && v->type == LEPT_OBJECT);
    assert(index < v->u.o.size);
    return &v->u.o.m[index].v;
}
