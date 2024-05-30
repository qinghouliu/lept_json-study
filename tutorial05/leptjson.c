#ifdef _WINDOWS
#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#endif
//这部分代码检查是否在 Windows 环境下编译。如果是，它定义了 _CRTDBG_MAP_ALLOC，
//这有助于在使用 Microsoft 的 CRT 函数时追踪内存泄漏。
#include "leptjson.h"
#include <assert.h>  /* assert() */
#include <errno.h>   /* errno, ERANGE */
#include <math.h>    /* HUGE_VAL */
#include <stdlib.h>  /* NULL, malloc(), realloc(), free(), strtod() */
#include <string.h>  /* memcpy() */

#ifndef LEPT_PARSE_STACK_INIT_SIZE
#define LEPT_PARSE_STACK_INIT_SIZE 256
#endif

//定义了一个宏，设置 JSON 解析堆栈的初始大小为 256 字节，除非已经有定义。
#define EXPECT(c, ch)       do { assert(*c->json == (ch)); c->json++; } while(0)
#define ISDIGIT(ch)         ((ch) >= '0' && (ch) <= '9')
#define ISDIGIT1TO9(ch)     ((ch) >= '1' && (ch) <= '9')
#define PUTC(c, ch)         do { *(char*)lept_context_push(c, sizeof(char)) = (ch); } while(0)
//将字符 ch 放入解析上下文 c 的堆栈中。

typedef struct {
    const char* json;
    char* stack;
    size_t size, top;
}lept_context;

static void* lept_context_push(lept_context* c, size_t size) {//就是将元素推入栈中（动态堆栈操作）
    //void*是一个指针类型，它可以指向任意类型的数据，包括基本类型、对象、数组等。void*的返回类型表示该函数返回的是一个指针，但并未指定具体指向的数据类型。
    //在函数lept_context_push中，返回类型为void* ，表示该函数返回一个指针，指向某个内存位置。具体指向的是什么类型的数据由函数的调用者决定，因为在函数内部并没有指定具体的类型
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
//检查是否可以从堆栈中弹出指定大小的数据，更新顶部指针，并返回弹出数据的位置
static void* lept_context_pop(lept_context* c, size_t size) {
    assert(c->top >= size);
    return c->stack + (c->top -= size);
}

static void lept_parse_whitespace(lept_context* c) {//跳过一些无实际意义的符号，指针指到有效字符位
    const char* p = c->json;
    while (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r')
        p++;
    c->json = p;
}
//与之前的相似，这里用了重构技术，判断NULL、FALSE TRUE等情况
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
//进行数字的解析
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
//这个函数的目的是从一个字符串中解析四个十六进制字符，并将其转换成一个无符号整数。
static const char* lept_parse_hex4(const char* p, unsigned* u) {
    int i;
    *u = 0;
    for (i = 0; i < 4; i++) {
        char ch = *p++;
        *u <<= 4;//将 u 的当前值左移4位。这样做是为了在 u 的低4位腾出空间，用于存放新的十六进制数。
        if (ch >= '0' && ch <= '9')  *u |= ch - '0';
        else if (ch >= 'A' && ch <= 'F')  *u |= ch - ('A' - 10);
        else if (ch >= 'a' && ch <= 'f')  *u |= ch - ('a' - 10);
        //ch 是否为有效的十六进制字符，并将其转换为对应的数值
        //（例如，'A' 转换为 10，'B' 转换为 11 等），然后加到 u 的低4位中。这里使用了位或操作（|=）。
        else return NULL;
    }
    return p;//处理完四个字符后，函数返回新的指针位置 p，此时 p 指向最后处理的十六进制数字之后的字符。
}
//这个函数将 Unicode 码点 u 转换成 UTF-8 编码形式，并将每个字节推入管理在上下文 c 中的堆栈。
static void lept_encode_utf8(lept_context* c, unsigned u) {
    if (u <= 0x7F)
        PUTC(c, u & 0xFF);
    //如果 u 小于等于 0x7F，它可以在 UTF-8 中用一个字节表示。代码通过与 0xFF 进行位与操作
    //（实际上这里没有变化，因为 u 已经小于 0xFF），然后推入堆栈。
    else if (u <= 0x7FF) {
        PUTC(c, 0xC0 | ((u >> 6) & 0xFF));
        PUTC(c, 0x80 | (u & 0x3F));
    }
    //如果 u 小于等于 0x7FF，它需要两个字节来表示。
    //第一个字节以 110 开头，后跟 u 的高5位；第二个字节以 10 开头，后跟 u 的低6位。
    else if (u <= 0xFFFF) {
        PUTC(c, 0xE0 | ((u >> 12) & 0xFF));
        PUTC(c, 0x80 | ((u >> 6) & 0x3F));
        PUTC(c, 0x80 | (u & 0x3F));
    }
    //对于最大值为 0xFFFF 的 u，需要三个字节。编码将第一个字节的前缀设为 1110，后面依次是每个6位组成的字节，
    //每个字节前缀为 10。
    else {
        assert(u <= 0x10FFFF);
        PUTC(c, 0xF0 | ((u >> 18) & 0xFF));
        PUTC(c, 0x80 | ((u >> 12) & 0x3F));
        PUTC(c, 0x80 | ((u >> 6) & 0x3F));
        PUTC(c, 0x80 | (u & 0x3F));
    }
}
#define STRING_ERROR(ret) do { c->top = head; return ret; } while(0)
static int lept_parse_string(lept_context* c, lept_value* v) {
    size_t head = c->top, len;
    //记录字符串开始前的堆栈顶部位置 head，用于后续计算字符串长度。初始化一些变量用于处理。
    //用 EXPECT 宏确认当前字符为引号（\"），这标志着字符串的开始
    unsigned u, u2;//这里用两个无符号数，来进行代替
    const char* p;
    EXPECT(c, '\"');//第一个开始的标志“
    p = c->json;
    for (;;) {
        char ch = *p++;
        switch (ch) {
        case '\"'://结束的标志“
            len = c->top - head;
            lept_set_string(v, (const char*)lept_context_pop(c, len), len);
            c->json = p;
            return LEPT_PARSE_OK;
            //如果遇到结束引号，计算字符串长度，
            //通过 lept_context_pop 弹出字符串并设置到 JSON 值 v 中。更新 JSON 处理位置 c->json 并返回成功状态。
        case '\\'://处理转义字符。如果是反斜杠，表明下一个字符是特殊字符或转义序列的开始。
            switch (*p++) {
            //对于常见的转义字符，如 \"、\\、\/、\b、\f、\n、\r、\t，直接将对应的字符值压入堆栈。
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
                //如果 lept_parse_hex4 返回 NULL（解析失败），则使用 STRING_ERROR 宏处理错误。
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
                    u = (((u - 0xD800) << 10) | (u2 - 0xDC00)) + 0x10000;
                    ///组合高低代理对生成一个完整的 Unicode 码点，并存储在 u 中。
                }
                lept_encode_utf8(c, u);
                //调用 lept_encode_utf8 将最终的 Unicode 码点 u 编码为 UTF-8 并存储在解析上下文的堆栈中。
                break;
            default:
                STRING_ERROR(LEPT_PARSE_INVALID_STRING_ESCAPE);//说明转义字符后面跟着的字母不对
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
static int lept_parse_value(lept_context* c, lept_value* v);//声明部分
static int lept_parse_array(lept_context* c, lept_value* v) {
    size_t i, size = 0;
    //i 用于循环计数，size 初始化为0，用来记录数组中元素的数量；
    int ret;
    //ret 用来存储函数返回值。
    //声明了两个变量，i 用于循环计数，size 用于记录数组中元素的数量。ret 用于存储函数返回值。
    EXPECT(c, '[');//使用 EXPECT 宏来确保当前解析的字符是数组开始的 [ 符号。如果不是，则会触发断言
    lept_parse_whitespace(c);//lept_parse_whitespace 函数来跳过可能存在的空白符，以便正确解析 JSON 数据。
    if (*c->json == ']') {
        c->json++;
        v->type = LEPT_ARRAY;
        v->u.a.size = 0;//size用来记录数组中元素的数量；
        v->u.a.e = NULL;
        return LEPT_PARSE_OK;
       //如果检测到空数组，将 JSON 指针向前移动，跳过 ]，设置 v 的类型为 LEPT_ARRAY，大小设置为0，元素指针设置为NULL，并返回解析成功的状态码
    }
    //如果在跳过空白符后直接遇到了数组结束的 ] 符号，这表示一个空数组。此时，更新 JSON 指针跳过 ]，
    //设置 v 的类型为数组但没有元素（size = 0 和 e = NULL），并返回解析成功的状态。
    for (;;) {
        //开始一个无限循环，用于逐个解析数组中的元素。为每个元素创建一个 lept_value 结构体 e，并使用 lept_init 初始化。
        lept_value e;
        lept_init(&e);
        if ((ret = lept_parse_value(c, &e)) != LEPT_PARSE_OK)//用于解析数组中的不同类型的数据
        //尝试解析一个 JSON 值并存储在 e 中。如果解析失败，则退出循环。
            break;
        memcpy(lept_context_push(c, sizeof(lept_value)), &e, sizeof(lept_value));
        //lept_context_push(c, sizeof(lept_value))：该函数用于将一个指定大小的内存块推入堆栈中，
        // 并返回指向该内存块的指针。这个函数在堆栈中为新的元素分配足够的空间，并返回指向该空间的指针。
        //memcpy(destination, &e, sizeof(lept_value))：这是一个内存拷贝函数，用于将e的内容复制到指定的目标内存位置。
        //destination是lept_context_push返回的指针，它指向堆栈中空闲的内存块。&e表示e变量的地址，sizeof(lept_value)表示要复制的字节数，
        //即lept_value类型的大小。这样，memcpy函数将e的内容复制到堆栈的适当位置。
        size++;
        lept_parse_whitespace(c);//再次跳过任何空白符，以准备解析下一个元素或检查数组结束标志。
        if (*c->json == ',') {//如果遇到元素分隔符 ,，则移动 JSON 指针跳过它，并处理后续的空白符。
            c->json++;
            lept_parse_whitespace(c);
        }
        else if (*c->json == ']') {
            c->json++;
            v->type = LEPT_ARRAY;
            v->u.a.size = size;
            size *= sizeof(lept_value);
            memcpy(v->u.a.e = (lept_value*)malloc(size), lept_context_pop(c, size), size);
            //(lept_value*)malloc(size)：使用malloc函数动态分配内存空间，大小为size字节。
            // 这里的size是之前循环中计算得到的数组中元素的数量乘以sizeof(lept_value)，即要分配的内存空间的总大小。
            // 强制类型转换(lept_value*)是将返回的指针转换为lept_value*类型，以便与v->u.a.e的类型相匹配。
            //lept_context_pop(c, size)：该函数从堆栈中弹出指定大小的元素。在这里，它用于从堆栈中弹出size个字节的内存块，这些内存块包含之前解析的数组元素。
            
            //memcpy(v->u.a.e, lept_context_pop(c, size), size)：这是一个内存拷贝函数，用于将堆栈中的元素复制到之前分配的内存空间中。
            // v->u.a.e表示数组元素的指针，它指向之前使用malloc分配的内存空间。lept_context_pop(c, size)用于获取堆栈中的元素，size表示要复制的字节数。
            //这样，memcpy函数将堆栈中的元素复制到新分配的内存空间中。

            return LEPT_PARSE_OK;
           
        }
        //如果遇到数组结束的 ]，更新 JSON 指针，设置 v 的类型为数组，并使用 malloc 分配恰好的内存空间存储所有元素。
        //从堆栈中弹出这些元素并复制到新分配的内存中，最后返回解析成功状态
        else {
            ret = LEPT_PARSE_MISS_COMMA_OR_SQUARE_BRACKET;
            break;
        }
        //如果既不是 , 也不是 ]，则说明 JSON 格式有误，设置错误类型并中断循环。
    }
    /* Pop and free values on the stack */
    for (i = 0; i < size; i++)
        lept_free((lept_value*)lept_context_pop(c, sizeof(lept_value)));
    return ret;
    //如果因为错误而退出循环，遍历堆栈上的所有元素，对每个元素调用 lept_free 来释放占用的资源，
    //并从堆栈中弹出。最后返回解析结果（通常是一个错误代码）。这一部分确保了任何分配的资源在错误发生时都被正确释放，避免内存泄漏。
    
}

static int lept_parse_value(lept_context* c, lept_value* v) {//主要的结构函数
    switch (*c->json) {
    case 't':  return lept_parse_literal(c, v, "true", LEPT_TRUE);
    case 'f':  return lept_parse_literal(c, v, "false", LEPT_FALSE);
    case 'n':  return lept_parse_literal(c, v, "null", LEPT_NULL);
    default:   return lept_parse_number(c, v);
    case '"':  return lept_parse_string(c, v);
    case '[':  return lept_parse_array(c, v);//在这里新增加的一个识别的
    case '\0': return LEPT_PARSE_EXPECT_VALUE;
    }
}

int lept_parse(lept_value* v, const char* json) {
    lept_context c;
    int ret;
    assert(v != NULL);
    //初始化解析上下文 c，声明返回变量 ret，并使用 assert 断言确认传入的 JSON 值指针 v 非空。
    c.json = json;
    c.stack = NULL;
    c.size = c.top = 0;
    //将 JSON 字符串指针赋给上下文，初始化堆栈及其相关变量。
    lept_init(v);
    //初始化 JSON 值 v 为 LEPT_NULL，并跳过 JSON 字符串前的空白字符。
    lept_parse_whitespace(&c);
    if ((ret = lept_parse_value(&c, v)) == LEPT_PARSE_OK) {
        lept_parse_whitespace(&c);
        if (*c.json != '\0') {
            v->type = LEPT_NULL;
            ret = LEPT_PARSE_ROOT_NOT_SINGULAR;
        }
    }
    //调用 lept_parse_value 解析 JSON 数据。如果解析成功且之后存在非空字符，则设置 JSON 值类型为 LEPT_NULL 并返回错误状态 LEPT_PARSE_ROOT_NOT_SINGULAR，
    // 表示 JSON 不是单一值。
    assert(c.top == 0);
    free(c.stack);
    return ret;
}

//这个函数用于释放 lept_value 类型的资源。它首先断言输入指针 v 非空。然后根据 v 的类型，执行相应的释放操作：
//如果 v 是字符串类型(LEPT_STRING)，释放字符串的内存。
//如果 v 是数组类型(LEPT_ARRAY)，递归释放数组中每个元素的内存，并释放数组本身。
//最后，将 v 的类型设置为 LEPT_NULL，表示这个值已被释放。

void lept_free(lept_value* v) {//资源释放函数 lept_free
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
    default: break;
    }
    v->type = LEPT_NULL;
}

lept_type lept_get_type(const lept_value* v) {//这个函数返回 JSON 值的类型。它首先断言输入指针 v 非空，然后返回 v 的类型。
    assert(v != NULL);
    return v->type;
}
//这个函数用于获取布尔值。它首先断言 v 非空并且 v 的类型是布尔类型（LEPT_TRUE 或 LEPT_FALSE）。
//然后返回一个整数值，表示布尔值（LEPT_TRUE 返回 1，LEPT_FALSE 返回 0）
int lept_get_boolean(const lept_value* v) {
    assert(v != NULL && (v->type == LEPT_TRUE || v->type == LEPT_FALSE));
    return v->type == LEPT_TRUE;
}

//这个函数用于设置布尔值。它首先调用 lept_free 释放 v 可能持有的任何资源。然后根据 b 的值设置 v 的类型为 LEPT_TRUE 或 LEPT_FALSE
void lept_set_boolean(lept_value* v, int b) {
    lept_free(v);
    v->type = b ? LEPT_TRUE : LEPT_FALSE;
}
//这个函数用于获取数字值。它首先断言 v 非空并且类型是 LEPT_NUMBER。然后返回数字值。
double lept_get_number(const lept_value* v) {
    assert(v != NULL && v->type == LEPT_NUMBER);
    return v->u.n;
}
//这个函数用于设置数字值。它首先调用 lept_free 释放 v 可能持有的任何资源。然后将 n 赋给 v 的数字字段，并将类型设置为 LEPT_NUMBER。
void lept_set_number(lept_value* v, double n) {
    lept_free(v);
    v->u.n = n;
    v->type = LEPT_NUMBER;
}
//这个函数用于获取字符串值。它首先断言 v 非空并且类型是 LEPT_STRING。然后返回字符串指针。
const char* lept_get_string(const lept_value* v) {
    assert(v != NULL && v->type == LEPT_STRING);
    return v->u.s.s;
}
//这个函数用于获取字符串的长度。它首先断言 v 非空并且类型是 LEPT_STRING。然后返回字符串的长度。
size_t lept_get_string_length(const lept_value* v) {
    assert(v != NULL && v->type == LEPT_STRING);
    return v->u.s.len;
}
//首先断言 v 非空，并且 s 非空或者长度为零。
// 然后调用 lept_free 释放 v 可能持有的任何资源。接着为字符串分配内存，将 s 复制到新分配的内存中，并设置字符串的长度和类型。
void lept_set_string(lept_value* v, const char* s, size_t len) {
    assert(v != NULL && (s != NULL || len == 0));
    lept_free(v);
    v->u.s.s = (char*)malloc(len + 1);
    memcpy(v->u.s.s, s, len); 
    v->u.s.s[len] = '\0';
    v->u.s.len = len;
    v->type = LEPT_STRING;
}

//这个函数用于获取数组的大小。它首先断言 v 非空并且类型是 LEPT_ARRAY。然后返回数组的大小。
size_t lept_get_array_size(const lept_value* v) {
    assert(v != NULL && v->type == LEPT_ARRAY);
    return v->u.a.size;
}
//这个函数用于获取数组中的某个元素。它首先断言 v 非空并且类型是 LEPT_ARRAY，
//然后断言索引在数组范围内。最后返回数组中指定索引的元素指针。
lept_value* lept_get_array_element(const lept_value* v, size_t index) {
    assert(v != NULL && v->type == LEPT_ARRAY);
    assert(index < v->u.a.size);
    return &v->u.a.e[index];
}