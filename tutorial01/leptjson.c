#include "leptjson.h"
#include <assert.h>  /* assert() *///断言库
#include <stdlib.h>  /* NULL *///标准库
#define EXPECT(c, ch)       do { assert(*c->json == (ch)); c->json++; } while(0)
//用于断言当前 JSON 字符和期望的字符匹配，并将字符指针向前移动一位
typedef struct {
    const char* json;
}lept_context;
//lept_context 结构体，用于存储 JSON 字符串的指针。
static void lept_parse_whitespace(lept_context* c) {
    const char *p = c->json;
    while (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r')
        p++;
    c->json = p;
}
//函数用于跳过 JSON 中的空白字符（空格、制表符、换行符、回车符）
//////////////////////////////////////////////////////////////////////////

static int lept_parse_true(lept_context* c, lept_value* v) {
    EXPECT(c, 't');//首先通过宏 EXPECT 确保当前字符是 't
    if (c->json[0] != 'r' || c->json[1] != 'u' || c->json[2] != 'e')
        return LEPT_PARSE_INVALID_VALUE;
    //LEPT_PARSE_INVALID_VALUE 的值是 2，这意味着如果解析的不是 "true"，那么函数将返回 2，表示解析的值是无效的。
    c->json += 3;
    v->type = LEPT_TRUE;//说明经过判断是字符true
    return LEPT_PARSE_OK;//解析成功
}

static int lept_parse_false(lept_context* c, lept_value* v) {
    EXPECT(c, 'f');
    if (c->json[0] != 'a' || c->json[1] != 'l' || c->json[2] != 's' || c->json[3] != 'e')
        return LEPT_PARSE_INVALID_VALUE;
    //同上所描述的
    c->json += 4;
    v->type = LEPT_FALSE;//说明经过判断是字符false
    return LEPT_PARSE_OK;//解析成功
}
//////////////////////////////////////////////////////////////////////////
static int lept_parse_null(lept_context* c, lept_value* v) {
    EXPECT(c, 'n');
    if (c->json[0] != 'u' || c->json[1] != 'l' || c->json[2] != 'l')
        return LEPT_PARSE_INVALID_VALUE;
    c->json += 3;
    v->type = LEPT_NULL;
    return LEPT_PARSE_OK;
}

static int lept_parse_value(lept_context* c, lept_value* v) {
    switch (*c->json) {
     //////////////////////////////////////////////////////////////////////
    case 't':  return lept_parse_true(c, v);
    case 'f':  return lept_parse_false(c, v);
    case 'n':  return lept_parse_null(c, v);
    case '\0': return LEPT_PARSE_EXPECT_VALUE;
    //当前字符是字符串的结尾（即空字符 '\0'），则表示解析的 JSON 值为空（即空字符串），返回 LEPT_PARSE_EXPECT_VALUE，表示期望值。
    default:   return LEPT_PARSE_INVALID_VALUE;//无效值
    ////////////////////////////////////////////////////////////////////////
    }
}
int lept_parse(lept_value* v, const char* json) {
    ///////////////////////////////////////////////////
    lept_context c;
    int ret;
    assert(v != NULL);
    c.json = json;//JSON 字符串 json 的地址赋值给 c 结构体实例的 json 成员
    v->type = LEPT_NULL;
    //lept_value 结构体指针 v 的 type 成员初始化为 LEPT_NULL，这是一个初始状态，表示 JSON 值的类型尚未确定。
    lept_parse_whitespace(&c);
    //调用 lept_parse_whitespace 函数，用于跳过 JSON 字符串中的空白字符，确保解析操作从有效数据开始。
    if ((ret = lept_parse_value(&c, v)) == LEPT_PARSE_OK) {
        lept_parse_whitespace(&c);
        //再次调用 lept_parse_whitespace 函数，跳过解析后可能存在的额外空白字符。
        if (*c.json != '\0')
            ret = LEPT_PARSE_ROOT_NOT_SINGULAR;
        //否存在多余的字符，如果当前字符不是字符串的结尾（即不是空字符 \0），
        //则表示 JSON 字符串中存在额外的未解析内容，这是不合法的，因此将解析结果设置为 
    }
    return ret;
    /////////////////////////////////////////////////
}

lept_type lept_get_type(const lept_value* v) {
    assert(v != NULL);
    return v->type;
}
//获取给定 JSON 值的类型。它首先使用 assert(v != NULL) 断言确保参数 v 不为空，然后返回 v 的 type 成员，即表示 JSON 值的类型。
