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

#ifndef LEPT_PARSE_STACK_INIT_SIZE
#define LEPT_PARSE_STACK_INIT_SIZE 256
#endif

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
//定义 lept_context
static void* lept_context_push(lept_context* c, size_t size) {
    //就是将元素推入栈中（动态堆栈操作）
    //void*是一个指针类型，它可以指向任意类型的数据，包括基本类型、对象、数组等。void*的返回类型表示该函数返回的是一个指针，但并未指定具体指向的数据类型。
    //在函数lept_context_push中，返回类型为void* ，表示该函数返回一个指针，指向某个内存位置。具体指向的是什么类型的数据由函数的调用者决定，因为在函数内部并没有指定具体的类型
    //size_t 是一个在 C 和 C++ 程序设计语言中用于表示大小和计数的无符号整数类型。它是一个类型定义（typedef)
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
    //c->stack 是指向堆栈底部的指针。
    //c->top 表示当前堆栈顶部的偏移量（以字节为单位）。
    //通过 c->stack + c->top，此表达式计算出当前堆栈顶部的地址，并将这个地址赋给 ret。这样，ret 就指向了可以放置新数据的堆栈位置
    c->top += size;
    //c->top 的值，增加的量是即将存储在堆栈中的数据的大小（size）。
    return ret;
}

static void* lept_context_pop(lept_context* c, size_t size) {
    assert(c->top >= size);
    return c->stack + (c->top -= size);
    //c->top -= size：首先，从当前栈顶指针（c->top）中减去出栈数据的大小（size），更新栈顶指针，指向新的栈顶位置。这步操作实际上是将栈顶向下移动了 size 字节。
    //c->stack + (c->top -= size)：然后，通过 c->stack（堆栈底部的指针）加上新的 c->top 值，计算出更新后的栈顶的内存地址。这个地址是出栈数据的起始位置。
}

static void lept_parse_whitespace(lept_context* c) {//跳过一些无实际意义的符号，指针指到有效字符位
    const char* p = c->json;
    while (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r')
        p++;
    c->json = p;
}

static int lept_parse_literal(lept_context* c, lept_value* v, const char* literal, lept_type type) {
    // 与之前的相似，这里用了重构技术，判断NULL、FALSE TRUE等情况
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
static const char* lept_parse_hex4(const char* p, unsigned* u) {//在这里传的是引用，实际上完成对u的赋值
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
    else {
    //对于最大值为 0xFFFF 的 u，需要三个字节。编码将第一个字节的前缀设为 1110，后面依次是每个6位组成的字节，
    //每个字节前缀为 10。
        assert(u <= 0x10FFFF);
        PUTC(c, 0xF0 | ((u >> 18) & 0xFF));
        PUTC(c, 0x80 | ((u >> 12) & 0x3F));
        PUTC(c, 0x80 | ((u >> 6) & 0x3F));
        PUTC(c, 0x80 | (u & 0x3F));
    }
}

#define STRING_ERROR(ret) do { c->top = head; return ret; } while(0)
//此宏定义用于处理字符串解析中出现的错误。当调用此宏时，它将栈顶指针 (c->top) 重置为 head（即字符串解析开始时的位置），
//然后返回错误代码 ret。这样做是为了丢弃在解析过程中可能已经加入到堆栈的部分数据，保持数据一致性。

static int lept_parse_string_raw(lept_context* c, char** str, size_t* len) {//这个函数尝试从 JSON 文本中解析一个字符串。
    //lept_context* c 表示 c 是一个指向 lept_context 结构体的指针。lept_context 可能是一个结构体，包含了处理 JSON 数据所需要的所有上下文信息，如当前解析位置、堆栈指针、堆栈大小等。
    //char** str 表示 str 是一个指向字符指针的指针。这种类型通常用于函数内部需要修改指针本身的值。
    //size_t* len 表示 len 是一个指向 size_t 类型的指针。size_t 通常用于表示数据的大小或长度。
    //在这个函数中，len 用于返回解析出的字符串的长度。通过修改* len（即 len 指向的大小）来设置字符串长度，这个长度是计算得到的，即字符串结束位置减去起始位置
    size_t head = c->top;//记录函数开始时的栈顶位置，以便在发生错误时可以回滚
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

static int lept_parse_string(lept_context* c, lept_value* v) {//对其进行重构，和之前的不一样
    int ret;
    char* s;
    size_t len;
    if ((ret = lept_parse_string_raw(c, &s, &len)) == LEPT_PARSE_OK)
        lept_set_string(v, s, len);
    return ret;
}

static int lept_parse_value(lept_context* c, lept_value* v);

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

static int lept_parse_object(lept_context* c, lept_value* v) {//ept_parse_object 接受两个参数：解析上下文 c 和一个将存储结果的 JSON 值 v。
    size_t i, size;
    lept_member m;//lept_member m; 定义一个对象成员变量，包含键和值。
    int ret;//int ret; 用来存储函数返回的状态码
    EXPECT(c, '{');
    lept_parse_whitespace(c);
    if (*c->json == '}') {//如果遇到 }，表示对象为空
        c->json++;
        v->type = LEPT_OBJECT;//更新解析位置，设置 JSON 值的类型为对象，初始化成员指针和大小为 0。
        v->u.o.m = 0;//指针赋值为0，也就是NULL
        v->u.o.size = 0;
        return LEPT_PARSE_OK;//返回解析成功状态码
    }
    m.k = NULL;
    //初始化键指针 m.k 为 NULL，以便后续分配内存。
    //使用无限循环开始解析对象中的键值对。
    size = 0;
    for (;;){
        char* str;
        lept_init(&m.v);
        /* parse key */
        if (*c->json != '"') {//确保键以引号开头，如果不是，则设置返回状态为键缺失。
            ret = LEPT_PARSE_MISS_KEY;
            break;
        }
        if ((ret = lept_parse_string_raw(c, &str, &m.klen)) != LEPT_PARSE_OK)//调用 lept_parse_string_raw 解析键字符串。
            break;
        memcpy(m.k = (char*)malloc(m.klen + 1), str, m.klen); //为键分配内存并复制解析得到的字符串，加上结束符 \0
        m.k[m.klen] = '\0';
        /* parse ws colon ws */
        lept_parse_whitespace(c);
        if (*c->json != ':') {//跳过任何空白，检查并消耗冒号,如果冒号缺失，则设置返回状态
            ret = LEPT_PARSE_MISS_COLON;
            break;
        }
        c->json++;
        lept_parse_whitespace(c);
        /* parse value */
        if ((ret = lept_parse_value(c, &m.v)) != LEPT_PARSE_OK)//解析 键对应的值，如果失败，中断循环。
            break;
        memcpy(lept_context_push(c, sizeof(lept_member)), &m, sizeof(lept_member));//将键值对推入上下文的堆栈。
        size++;
        m.k = NULL; /* ownership is transferred to member on stack */ 
        /* parse ws [comma | right-curly-brace] ws */
        lept_parse_whitespace(c);//解析键值对之后的空白字符
        if (*c->json == ',') {//如果遇到逗号，则表示后面还有其他成员，消耗逗号并继续解析。
            c->json++;
            lept_parse_whitespace(c);
        }
        else if (*c->json == '}') {
            size_t s = sizeof(lept_member) * size;
            //如果遇到对象结束标志 }，则完成所有成员的解析，计算总大小，从堆栈中弹出所有成员，分配内存，
            //变量 size 表示当前存储或将要存储的键值对（lept_member 结构体）的数量。
            //通过将 sizeof(lept_member) 乘以 size，你正在计算存储 size 个 lept_member 实例所需的总内存量。
            c->json++;
            v->type = LEPT_OBJECT;
            v->u.o.size = size;
            memcpy(v->u.o.m = (lept_member*)malloc(s), lept_context_pop(c, s), s);
            //（使用 malloc 或类似函数）来存储 lept_member 实例数组之前进行。在提供的代码中，一旦知道所需的总大小（s），
            //就分配相应的内存并将所有 lept_member 结构从临时存储（通常是解析上下文的一部分的栈）复制到这个新分配的内存中。
            //这一步骤是解析 JSON 对象并存储其所有成员的一部分：
            return LEPT_PARSE_OK;
        }
        else {
            ret = LEPT_PARSE_MISS_COMMA_OR_CURLY_BRACKET;
            break;
        }
    }
    /* Pop and free members on the stack */
    
    //在循环结束后（通常是因为发生了错误），释放已分配的键内存（如果存在）。
    //遍历堆栈，弹出所有成员，并释放每个成员的键和值所占用的资源。
    //将 v 的类型设置为 LEPT_NULL，表示解析失败，返回错误代码。
    free(m.k);
    for (i = 0; i < size; i++) {
        lept_member* m = (lept_member*)lept_context_pop(c, sizeof(lept_member));
        free(m->k);
        lept_free(&m->v);
    }
    v->type = LEPT_NULL;
    return ret;
}

static int lept_parse_value(lept_context* c, lept_value* v) {
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

int lept_parse(lept_value* v, const char* json) {
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
