////////////////////////////////////////////////////////////////////////第一个练习
#include <stdio.h>//标准输入输出库，用于printf和fprintf等函数。
#include <stdlib.h>
#include <string.h>//标准输入输出库，用于printf和fprintf等函数。
#include "leptjson.h"//JSON解析器库的头文件，定义了库的接口和数据结构。

static int main_ret = 0;//主函数返回值，用于指示测试是否完全通过。
static int test_count = 0;//运行的测试总数
static int test_pass = 0;//通过的测试数

//这是一个宏，用于比较期望值和实际值是否相等。如果相等，则增加test_pass；如果不等，则打印错误信息并设置main_ret为1
//equality 表示预期的相等性（真或假），expect 表示预期的值，actual 表示实际的值，format 是打印值的格式字符串。
// do { ... } while(0) 结构，这样可以在宏中使用多个语句而不会导致语法错误。
//表示通过的测试数加1；否则，打印错误信息到标准错误流 stderr 中，并将 main_ret 设为 1，表示测试失败。
#define EXPECT_EQ_BASE(equality, expect, actual, format) \
    do {\
        test_count++;\
        if (equality)\
            test_pass++;\
        else {\
            fprintf(stderr, "%s:%d: expect: " format " actual: " format "\n", __FILE__, __LINE__, expect, actual);\
            main_ret = 1;\
        }\
    } while(0)
//专门用于整数比较的宏，调用上面定义的EXPECT_EQ_BASE宏
//这是一个基于 EXPECT_EQ_BASE 宏的专门用于整数比较的宏。它接受两个参数：expect 表示预期的整数值，actual 表示实际的整数值。
//它调用了 EXPECT_EQ_BASE 宏，将 expect 和 actual 传递给它，并将 equality 设为(expect) == (actual)，表示预期的整数值和实际的整数值是否相等。
#define EXPECT_EQ_INT(expect, actual) EXPECT_EQ_BASE((expect) == (actual), expect, actual, "%d")

//测试解析器是否能正确解析null值。初始类型设置为LEPT_FALSE，预期解析后类型为LEPT_NULL。
static void test_parse_null() {
    lept_value v;
    v.type = LEPT_FALSE;
    EXPECT_EQ_INT(LEPT_PARSE_OK, lept_parse(&v, "null"));
    EXPECT_EQ_INT(LEPT_NULL, lept_get_type(&v));
}

/// /////////////////////////////////////////////////////////
//测试解析器是否能正确解析true值。初始类型设置为LEPT_FALSE，预期解析后类型为LEPT_NULL。
static void test_parse_true() {
    lept_value v;
    v.type = LEPT_FALSE;
    EXPECT_EQ_INT(LEPT_PARSE_OK, lept_parse(&v, "true"));
    EXPECT_EQ_INT(LEPT_TRUE, lept_get_type(&v));
}

static void test_parse_false() {
    lept_value v;
    v.type = LEPT_TRUE;
    EXPECT_EQ_INT(LEPT_PARSE_OK, lept_parse(&v, "false"));
    EXPECT_EQ_INT(LEPT_FALSE, lept_get_type(&v));
}
/////////////////////////////////////////////////////////////
//讨论JSON解析器对不同情况的处理能力：
//测试当JSON字符串输入不符合预期时，JSON解析器的行为。具体地，它测试了解析器对空字符串和仅包含空白字符的字符串的处理。
//在JSON中，有效的输入至少需要包含一个值（如null、false、true、数字、字符串、数组或对象）。如果输入为空或只有空白，
//不符合JSON的语法规则，因此应该返回一个特定的错误代码。
static void test_parse_expect_value() {
    lept_value v;
    v.type = LEPT_FALSE;
    EXPECT_EQ_INT(LEPT_PARSE_EXPECT_VALUE, lept_parse(&v, ""));
    EXPECT_EQ_INT(LEPT_NULL, lept_get_type(&v));
    v.type = LEPT_FALSE;
    EXPECT_EQ_INT(LEPT_PARSE_EXPECT_VALUE, lept_parse(&v, " "));
    EXPECT_EQ_INT(LEPT_NULL, lept_get_type(&v));
}
//讨论不合法的情况
static void test_parse_invalid_value() {
    lept_value v;
    v.type = LEPT_FALSE;
    EXPECT_EQ_INT(LEPT_PARSE_INVALID_VALUE, lept_parse(&v, "nul"));
    EXPECT_EQ_INT(LEPT_NULL, lept_get_type(&v));

    v.type = LEPT_FALSE;
    EXPECT_EQ_INT(LEPT_PARSE_INVALID_VALUE, lept_parse(&v, "?"));
    EXPECT_EQ_INT(LEPT_NULL, lept_get_type(&v));
}
//一个合法的JSON文档应该只包含一个单一的值（如一个对象、数组、数值、字符串、布尔值或null）
//如果在一个有效的JSON值之后还有其他字符，那么这应该被视为一个错误。
static void test_parse_root_not_singular() {
    lept_value v;
    v.type = LEPT_FALSE;
    EXPECT_EQ_INT(LEPT_PARSE_ROOT_NOT_SINGULAR, lept_parse(&v, "null x"));
    EXPECT_EQ_INT(LEPT_NULL, lept_get_type(&v));
}

static void test_parse() {
    test_parse_null();
    //////////////////////////////////////////////////////////
    test_parse_true();
    test_parse_false();
    /////////////////////////////////////////////////////////////
    test_parse_expect_value();
    test_parse_invalid_value();
    test_parse_root_not_singular();
}

int main() {
    test_parse();
    printf("%d/%d (%3.2f%%) passed\n", test_pass, test_count, test_pass * 100.0 / test_count);
    return main_ret;
}
