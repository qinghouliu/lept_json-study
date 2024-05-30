#include "leptjson.h"
#include <assert.h>  /* assert() */
#include <errno.h>   /* errno, ERANGE *///errno.h 定义了一个名为 errno 的外部全局变量，用于报告函数调用产生的错误代码
#include <math.h>    /* HUGE_VAL */
#include <stdlib.h>  /* NULL, strtod() */
//宏 EXPECT(c, ch)，用于检查当前解析位置 c->json 的字符是否为 ch，如果不是则触发断言失败，移动解析位置。
//两个宏 ISDIGIT(ch) 和 ISDIGIT1TO9(ch)，用于判断字符 ch 是否为数字字符或者在 '1' 到 '9' 之间的数字字符。
#define EXPECT(c, ch)       do { assert(*c->json == (ch)); c->json++; } while(0)
#define ISDIGIT(ch)         ((ch) >= '0' && (ch) <= '9')
#define ISDIGIT1TO9(ch)     ((ch) >= '1' && (ch) <= '9')
//结构体 lept_context，用于存储解析过程中的上下文信息，其中包含一个指向当前解析位置的指针 json。
typedef struct {
    const char* json;
}lept_context;
//静态函数 lept_parse_whitespace，用于跳过空白字符，包括空格、制表符、换行符和回车符。
static void lept_parse_whitespace(lept_context* c) {
    const char* p = c->json;
    while (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r')
        p++;
    c->json = p;
}
//一个静态函数 lept_parse_literal，用于解析 JSON 字面值（如 "true"、"false"、"null"
//在这个函数中，实现了对教程1的重构，也就是将3个函数进行整合
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
//定义了一个静态函数 lept_parse_number，用于解析 JSON 数字。
//lept_parse_number 函数用于解析 JSON 中的数字，并将解析结果存储在 lept_value 结构体 v
static int lept_parse_number(lept_context* c, lept_value* v) {
    const char* p = c->json;
    if (*p == '-') p++;//先判断是不是负数
    if (*p == '0') p++;//前导0去掉
    else {
        if (!ISDIGIT1TO9(*p)) return LEPT_PARSE_INVALID_VALUE;       //判断是不是数字开头
        for (p++; ISDIGIT(*p); p++);//进行整数部分的遍历
    }
    if (*p == '.') {//遍历到小数部分
        p++;
        if (!ISDIGIT(*p)) return LEPT_PARSE_INVALID_VALUE;//小数部分首个数字可为0,0-9
        for (p++; ISDIGIT(*p); p++);// 0 - 9,直至遍历完小数部分
    }
    if (*p == 'e' || *p == 'E') {//遍历指数
        p++;
        if (*p == '+' || *p == '-') p++;//指数上面的整数的符号
        if (!ISDIGIT(*p)) return LEPT_PARSE_INVALID_VALUE;
        for (p++; ISDIGIT(*p); p++);//指数可以为0,判断的范围为0-9
    }

 //使用 strtod 函数将解析完成的数字字符串转换为双精度浮点数，并将结果存储在 v->n 中。在这之前会将 errno 设置为 0，
 // 如果 strtod 函数执行出错（例如数字过大导致溢出），则 errno 会被设置为 ERANGE。
 // 如果 errno 等于 ERANGE 并且解析结果为 HUGE_VAL 或 -HUGE_VAL，则返回解析错误 LEPT_PARSE_NUMBER_TOO_BIG。
    errno = 0;
    v->n = strtod(c->json, NULL);
    if (errno == ERANGE && (v->n == HUGE_VAL || v->n == -HUGE_VAL))
        return LEPT_PARSE_NUMBER_TOO_BIG;
    v->type = LEPT_NUMBER;
    c->json = p;//表示解析完这个数字
    return LEPT_PARSE_OK;//成功解析
}
//静态函数 lept_parse_value，用于解析 JSON 值，根据首字符决定调用哪个解析函数
static int lept_parse_value(lept_context* c, lept_value* v) {
    switch (*c->json) {
    case 't':  return lept_parse_literal(c, v, "true", LEPT_TRUE);
    case 'f':  return lept_parse_literal(c, v, "false", LEPT_FALSE);
    case 'n':  return lept_parse_literal(c, v, "null", LEPT_NULL);
    default:   return lept_parse_number(c, v);
    case '\0': return LEPT_PARSE_EXPECT_VALUE;
    }
}
//JSON 解析函数 lept_parse，接受一个 JSON 字符串 json，将解析结果存储在 lept_value 结构体 v 中。
int lept_parse(lept_value* v, const char* json) {
    lept_context c;
    int ret;
    assert(v != NULL);
    c.json = json;
    v->type = LEPT_NULL;
    lept_parse_whitespace(&c);
    if ((ret = lept_parse_value(&c, v)) == LEPT_PARSE_OK) {
        lept_parse_whitespace(&c);
        if (*c.json != '\0') {
            v->type = LEPT_NULL;
            ret = LEPT_PARSE_ROOT_NOT_SINGULAR;
        }
    }
    return ret;
}
//定义了获取 JSON 值类型的函数 lept_get_type，用于返回 lept_value 结构体中存储的 JSON 值的类型。
lept_type lept_get_type(const lept_value* v) {
    assert(v != NULL);
    return v->type;
}

double lept_get_number(const lept_value* v) {
    assert(v != NULL && v->type == LEPT_NUMBER);
    return v->n;
}