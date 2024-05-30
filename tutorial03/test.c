#ifdef _WINDOWS
#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#endif
#include <stdio.h>
#include <stdlib.h>//提供内存分配功能，如 malloc 和 free
#include <string.h>//提供字符串操作功能，如 memcmp 和 strlen
#include "leptjson.h"
//全局变量

static int main_ret = 0;
static int test_count = 0;
static int test_pass = 0;
//EXPECT_EQ_BASE：用于比较预期值和实际值，如果相等则测试通过，否则打印错误信息并更新全局状态变量 main_ret。
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
//EXPECT_EQ_INT：用于比较整数。
//EXPECT_EQ_DOUBLE：用于比较双精度浮点数。
//EXPECT_EQ_STRING：用于比较字符串，alength 是实际字符串的长度。
//EXPECT_TRUE 和 EXPECT_FALSE：用于比较布尔值。
#define EXPECT_EQ_INT(expect, actual) EXPECT_EQ_BASE((expect) == (actual), expect, actual, "%d")
#define EXPECT_EQ_DOUBLE(expect, actual) EXPECT_EQ_BASE((expect) == (actual), expect, actual, "%.17g")
#define EXPECT_EQ_STRING(expect, actual, alength) \
    EXPECT_EQ_BASE(sizeof(expect) - 1 == (alength) && memcmp(expect, actual, alength) == 0, expect, actual, "%s")
//memcmp是比较内存区域buf1和buf2的前count个字节。该函数是按字节比较的
//比较内存区域buf1和buf2的前count个字节。
//当buf1 < buf2时，返回值<0
//当buf1 = buf2时，返回值 = 0
//当buf1>buf2时，返回值>0
#define EXPECT_TRUE(actual) EXPECT_EQ_BASE((actual) != 0, "true", "false", "%s")
#define EXPECT_FALSE(actual) EXPECT_EQ_BASE((actual) == 0, "false", "true", "%s")

static void test_parse_null() {
    lept_value v;
    lept_init(&v);//调用lept_init(&v)之后，v不是空指针，而是指向某个已经分配的内存位置，其中的type字段被设置为LEPT_NULL
    lept_set_boolean(&v, 0);
    EXPECT_EQ_INT(LEPT_PARSE_OK, lept_parse(&v, "null"));
    EXPECT_EQ_INT(LEPT_NULL, lept_get_type(&v));
    lept_free(&v);
}

static void test_parse_true() {
    lept_value v;
    lept_init(&v);
    lept_set_boolean(&v, 0);
    EXPECT_EQ_INT(LEPT_PARSE_OK, lept_parse(&v, "true"));
    EXPECT_EQ_INT(LEPT_TRUE, lept_get_type(&v));
    lept_free(&v);
}

static void test_parse_false() {
    lept_value v;
    lept_init(&v);
    lept_set_boolean(&v, 1);
    EXPECT_EQ_INT(LEPT_PARSE_OK, lept_parse(&v, "false"));
    EXPECT_EQ_INT(LEPT_FALSE, lept_get_type(&v));
    lept_free(&v);
}
//判断能否解析成功
//获取解析的类型
//得到数字
#define TEST_NUMBER(expect, json)\
    do {\
        lept_value v;\
        lept_init(&v);\
        EXPECT_EQ_INT(LEPT_PARSE_OK, lept_parse(&v, json));\
        EXPECT_EQ_INT(LEPT_NUMBER, lept_get_type(&v));\
        EXPECT_EQ_DOUBLE(expect, lept_get_number(&v));\
        lept_free(&v);\
    } while(0)

static void test_parse_number() {
    TEST_NUMBER(0.0, "0");
    TEST_NUMBER(0.0, "-0");
    TEST_NUMBER(0.0, "-0.0");
    TEST_NUMBER(1.0, "1");
    TEST_NUMBER(-1.0, "-1");
    TEST_NUMBER(1.5, "1.5");
    TEST_NUMBER(-1.5, "-1.5");
    TEST_NUMBER(3.1416, "3.1416");
    TEST_NUMBER(1E10, "1E10");
    TEST_NUMBER(1e10, "1e10");
    TEST_NUMBER(1E+10, "1E+10");
    TEST_NUMBER(1E-10, "1E-10");
    TEST_NUMBER(-1E10, "-1E10");
    TEST_NUMBER(-1e10, "-1e10");
    TEST_NUMBER(-1E+10, "-1E+10");
    TEST_NUMBER(-1E-10, "-1E-10");
    TEST_NUMBER(1.234E+10, "1.234E+10");
    TEST_NUMBER(1.234E-10, "1.234E-10");
    TEST_NUMBER(0.0, "1e-10000"); /* must underflow */

    TEST_NUMBER(1.0000000000000002, "1.0000000000000002"); /* the smallest number > 1 */
    TEST_NUMBER( 4.9406564584124654e-324, "4.9406564584124654e-324"); /* minimum denormal */
    TEST_NUMBER(-4.9406564584124654e-324, "-4.9406564584124654e-324");
    TEST_NUMBER( 2.2250738585072009e-308, "2.2250738585072009e-308");  /* Max subnormal double */
    TEST_NUMBER(-2.2250738585072009e-308, "-2.2250738585072009e-308");
    TEST_NUMBER( 2.2250738585072014e-308, "2.2250738585072014e-308");  /* Min normal positive double */
    TEST_NUMBER(-2.2250738585072014e-308, "-2.2250738585072014e-308");
    TEST_NUMBER( 1.7976931348623157e+308, "1.7976931348623157e+308");  /* Max double */
    TEST_NUMBER(-1.7976931348623157e+308, "-1.7976931348623157e+308");
}

#define TEST_STRING(expect, json)\
    do {\
        lept_value v;\
        lept_init(&v);\
        EXPECT_EQ_INT(LEPT_PARSE_OK, lept_parse(&v, json));\
        EXPECT_EQ_INT(LEPT_STRING, lept_get_type(&v));\
        EXPECT_EQ_STRING(expect, lept_get_string(&v), lept_get_string_length(&v));\
        lept_free(&v);\
    } while(0)

static void test_parse_string() {//用于测试TEST_STRING
    TEST_STRING("", "\"\"");
    TEST_STRING("Hello", "\"Hello\"");
    TEST_STRING("Hello\nWorld", "\"Hello\\nWorld\"");
    TEST_STRING("\" \\ / \b \f \n \r \t", "\"\\\" \\\\ \\/ \\b \\f \\n \\r \\t\"");

}

#define TEST_ERROR(error, json)\
    do {\
        lept_value v;\
        lept_init(&v);\
        v.type = LEPT_FALSE;\
        EXPECT_EQ_INT(error, lept_parse(&v, json));\
        EXPECT_EQ_INT(LEPT_NULL, lept_get_type(&v));\
        lept_free(&v);\
    } while(0)
static void test_parse_expect_value() {
    TEST_ERROR(LEPT_PARSE_EXPECT_VALUE, "");//空输入：
    TEST_ERROR(LEPT_PARSE_EXPECT_VALUE, " ");//仅包含空白字符的输入：
}

static void test_parse_invalid_value() {//无效的JSON字符串
    TEST_ERROR(LEPT_PARSE_INVALID_VALUE, "nul");
    TEST_ERROR(LEPT_PARSE_INVALID_VALUE, "?");

    /* invalid number *///无效的数字：
    TEST_ERROR(LEPT_PARSE_INVALID_VALUE, "+0");
    TEST_ERROR(LEPT_PARSE_INVALID_VALUE, "+1");
    TEST_ERROR(LEPT_PARSE_INVALID_VALUE, ".123"); /* at least one digit before '.' */
    TEST_ERROR(LEPT_PARSE_INVALID_VALUE, "1.");   /* at least one digit after '.' */
    TEST_ERROR(LEPT_PARSE_INVALID_VALUE, "INF");
    TEST_ERROR(LEPT_PARSE_INVALID_VALUE, "inf");
    TEST_ERROR(LEPT_PARSE_INVALID_VALUE, "NAN");
    TEST_ERROR(LEPT_PARSE_INVALID_VALUE, "nan");
}
static void test_parse_root_not_singular() {
    TEST_ERROR(LEPT_PARSE_ROOT_NOT_SINGULAR, "null x");//JSON根元素后还有额外的非空白字符：

    /* invalid number */
    TEST_ERROR(LEPT_PARSE_ROOT_NOT_SINGULAR, "0123"); /* after zero should be '.' or nothing */
    TEST_ERROR(LEPT_PARSE_ROOT_NOT_SINGULAR, "0x0");
    TEST_ERROR(LEPT_PARSE_ROOT_NOT_SINGULAR, "0x123");
}

static void test_parse_number_too_big() {//超大正数和负数：
    TEST_ERROR(LEPT_PARSE_NUMBER_TOO_BIG, "1e309");
    TEST_ERROR(LEPT_PARSE_NUMBER_TOO_BIG, "-1e309");
}

static void test_parse_missing_quotation_mark() {//测试用例用于测试缺少结束引号的字符串：
    TEST_ERROR(LEPT_PARSE_MISS_QUOTATION_MARK, "\"");
    TEST_ERROR(LEPT_PARSE_MISS_QUOTATION_MARK, "\"abc");
}

static void test_parse_invalid_string_escape() {//用于测试无效的转义字符：

    TEST_ERROR(LEPT_PARSE_INVALID_STRING_ESCAPE, "\"\\v\"");
    TEST_ERROR(LEPT_PARSE_INVALID_STRING_ESCAPE, "\"\\'\"");
    TEST_ERROR(LEPT_PARSE_INVALID_STRING_ESCAPE, "\"\\0\"");
    TEST_ERROR(LEPT_PARSE_INVALID_STRING_ESCAPE, "\"\\x12\"");

}

static void test_parse_invalid_string_char() {//无效的字符：

    TEST_ERROR(LEPT_PARSE_INVALID_STRING_CHAR, "\"\x01\"");
    TEST_ERROR(LEPT_PARSE_INVALID_STRING_CHAR, "\"\x1F\"");

}

static void test_access_null() {
    lept_value v;
    lept_init(&v);
    lept_set_string(&v, "a", 1);
    //调用 lept_set_string 函数，将字符串 "a"（长度为1）赋值给 v。
    lept_set_null(&v);//lept_set_null(&v);：将 v 的类型设置为 NULL。
    EXPECT_EQ_INT(LEPT_NULL, lept_get_type(&v));//使用断言 EXPECT_EQ_INT 检查 v 的类型是否为 NULL。
    lept_free(&v);
}
static void test_access_boolean() {//定义一个静态的、无返回值的函数，用于测试 lept_value 类型的布尔值访问。
    /* \TODO */
    /* Use EXPECT_TRUE() and EXPECT_FALSE() */
    lept_value v;
    lept_init(&v);
    lept_set_string(&v, "a", 1);
    //调用 lept_set_string 函数，将字符串 "a"（长度为1）赋值给 v。
    lept_set_boolean(&v, 1);//lept_set_boolean(&v, 1);：将 v 的值设置为布尔值 true（1 表示 true）。
    EXPECT_TRUE(lept_get_boolean(&v));//：使用断言 EXPECT_TRUE 检查 v 是否为布尔 true。
    lept_set_boolean(&v, 0);//将 v 的布尔值设置为 false（0 表示 false）
    EXPECT_FALSE(lept_get_boolean(&v));//使用断言 EXPECT_FALSE 检查 v 是否为布尔 false。
    lept_free(&v);// 释放 v 所占用的资源。
}

static void test_access_number() {//定义一个静态的、无返回值的函数，用于测试 lept_value 类型的数值访问。
    /* \TODO */
    lept_value v;
    lept_init(&v);//调用 lept_init 初始化 v。
    lept_set_string(&v, "a", 1);//将字符串 "a"（长度为1）赋值给 v
    lept_set_number(&v, 1234.5);//将 v 的值设置为数字 1234.5
    EXPECT_EQ_DOUBLE(1234.5, lept_get_number(&v));
    lept_free(&v);
}

static void test_access_string() {//：定义一个静态的、无返回值的函数，用于测试 lept_value 类型的字符串访问。
    lept_value v;
    lept_init(&v);
    lept_set_string(&v, "", 0);//将空字符串 ""（长度为0）赋值给 v
    EXPECT_EQ_STRING("", lept_get_string(&v), lept_get_string_length(&v));
    //断言检查 v 中的字符串是否为空字符串 ""。
    lept_set_string(&v, "Hello", 5);
    EXPECT_EQ_STRING("Hello", lept_get_string(&v), lept_get_string_length(&v));//断言检查 v 中的字符串是否等于 "Hello"。
    lept_free(&v);
}

static void test_parse() {
    test_parse_null();
    test_parse_true();
    test_parse_false();
    test_parse_number();
    test_parse_string();
    test_parse_expect_value();
    test_parse_invalid_value();
    test_parse_root_not_singular();
    test_parse_number_too_big();
    test_parse_missing_quotation_mark();
    test_parse_invalid_string_escape();
    test_parse_invalid_string_char();
    test_access_null();
    test_access_boolean();
    test_access_number();
    test_access_string();
}

int main() {
#ifdef _WINDOWS
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif
    test_parse();
    printf("%d/%d (%3.2f%%) passed\n", test_pass, test_count, test_pass * 100.0 / test_count);
    return main_ret;
}
