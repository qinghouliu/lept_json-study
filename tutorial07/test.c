#ifdef _WINDOWS
#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "leptjson.h"

//全局测试统计变量：
static int main_ret = 0;
static int test_count = 0;
static int test_pass = 0;
//用于测试表达式的基础宏，如果测试通过，增加通过计数；如果失败，打印错误信息并设置 main_ret 为 1
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
//EXPECT_EQ_INT、EXPECT_EQ_DOUBLE、EXPECT_EQ_STRING、EXPECT_TRUE、EXPECT_FALSE：这些宏用于测试不同类型的预期和实际值是否相等。
#define EXPECT_EQ_INT(expect, actual) EXPECT_EQ_BASE((expect) == (actual), expect, actual, "%d")
#define EXPECT_EQ_DOUBLE(expect, actual) EXPECT_EQ_BASE((expect) == (actual), expect, actual, "%.17g")
#define EXPECT_EQ_STRING(expect, actual, alength) \
    EXPECT_EQ_BASE(sizeof(expect) - 1 == alength && memcmp(expect, actual, alength + 1) == 0, expect, actual, "%s")
#define EXPECT_TRUE(actual) EXPECT_EQ_BASE((actual) != 0, "true", "false", "%s")
#define EXPECT_FALSE(actual) EXPECT_EQ_BASE((actual) == 0, "false", "true", "%s")

//用于测试 size_t 类型的值，由于不同编译器对 size_t 的打印格式支持不同，分别为 MSVC 和其他编译器定义了不同的格式化字符串。
#if defined(_MSC_VER)
#define EXPECT_EQ_SIZE_T(expect, actual) EXPECT_EQ_BASE((expect) == (actual), (size_t)expect, (size_t)actual, "%Iu")
#else
#define EXPECT_EQ_SIZE_T(expect, actual) EXPECT_EQ_BASE((expect) == (actual), (size_t)expect, (size_t)actual, "%zu")
#endif

static void test_parse_null() {//test_parse_null解析 null
    lept_value v;
    lept_init(&v);
    lept_set_boolean(&v, 0);
    EXPECT_EQ_INT(LEPT_PARSE_OK, lept_parse(&v, "null"));
    EXPECT_EQ_INT(LEPT_NULL, lept_get_type(&v));
    lept_free(&v);
}

static void test_parse_true() {//解析 true
    lept_value v;
    lept_init(&v);
    lept_set_boolean(&v, 0);
    EXPECT_EQ_INT(LEPT_PARSE_OK, lept_parse(&v, "true"));
    EXPECT_EQ_INT(LEPT_TRUE, lept_get_type(&v));
    lept_free(&v);
}

static void test_parse_false() {//解析 false
    lept_value v;
    lept_init(&v);
    lept_set_boolean(&v, 1);
    EXPECT_EQ_INT(LEPT_PARSE_OK, lept_parse(&v, "false"));
    EXPECT_EQ_INT(LEPT_FALSE, lept_get_type(&v));
    lept_free(&v);
}
//用于测试 JSON 数字解析。初始化 JSON 值，解析给定的 JSON 文本，检查解析结果是否为预期的数字类型和值，最后释放资源。
#define TEST_NUMBER(expect, json)\
    do {\
        lept_value v;\
        lept_init(&v);\
        EXPECT_EQ_INT(LEPT_PARSE_OK, lept_parse(&v, json));\
        EXPECT_EQ_INT(LEPT_NUMBER, lept_get_type(&v));\
        EXPECT_EQ_DOUBLE(expect, lept_get_number(&v));\
        lept_free(&v);\
    } while(0)
//测试各种数字的解析，包括正负数、不同格式的科学记数法、极小和极大的浮点数等。每个测试案例使用 TEST_NUMBER 宏来执行
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
    TEST_NUMBER(4.9406564584124654e-324, "4.9406564584124654e-324"); /* minimum denormal */
    TEST_NUMBER(-4.9406564584124654e-324, "-4.9406564584124654e-324");
    TEST_NUMBER(2.2250738585072009e-308, "2.2250738585072009e-308");  /* Max subnormal double */
    TEST_NUMBER(-2.2250738585072009e-308, "-2.2250738585072009e-308");
    TEST_NUMBER(2.2250738585072014e-308, "2.2250738585072014e-308");  /* Min normal positive double */
    TEST_NUMBER(-2.2250738585072014e-308, "-2.2250738585072014e-308");
    TEST_NUMBER(1.7976931348623157e+308, "1.7976931348623157e+308");  /* Max double */
    TEST_NUMBER(-1.7976931348623157e+308, "-1.7976931348623157e+308");
}
// TEST_STRING 宏
//定义一个宏 TEST_STRING，用于测试 JSON 解析器能否正确解析 JSON 字符串。
//创建 lept_value v 对象并初始化。
//使用 lept_parse 解析 JSON 字符串 json，期望返回 LEPT_PARSE_OK。
//检查解析后的类型是否为 LEPT_STRING。
//检查解析得到的字符串是否与预期的 expect 一致，同时验证长度。
//最后释放 lept_value v。
#define TEST_STRING(expect, json)\
    do {\
        lept_value v;\
        lept_init(&v);\
        EXPECT_EQ_INT(LEPT_PARSE_OK, lept_parse(&v, json));\
        EXPECT_EQ_INT(LEPT_STRING, lept_get_type(&v));\
        EXPECT_EQ_STRING(expect, lept_get_string(&v), lept_get_string_length(&v));\
        lept_free(&v);\
    } while(0)

static void test_parse_string() {
    //通过 TEST_STRING 宏测试各种字符串场景，包括空字符串、普通字符串、包含转义字符的字符串、包含 Unicode 字符的字符串等。
    //特别注意测试了 JSON 字符串中 Unicode 转义序列的解析，如 "\u0024"（美元符号）、"\u00A2"（分币符号）、"\u20AC"（欧元符号）等。
    //还测试了具有复杂 Unicode 码点（如音乐符号 G clef U + 1D11E）的字符串解析
    TEST_STRING("", "\"\"");
    TEST_STRING("Hello", "\"Hello\"");
    TEST_STRING("Hello\nWorld", "\"Hello\\nWorld\"");
    TEST_STRING("\" \\ / \b \f \n \r \t", "\"\\\" \\\\ \\/ \\b \\f \\n \\r \\t\"");
    TEST_STRING("Hello\0World", "\"Hello\\u0000World\"");
    TEST_STRING("\x24", "\"\\u0024\"");         /* Dollar sign U+0024 */
    TEST_STRING("\xC2\xA2", "\"\\u00A2\"");     /* Cents sign U+00A2 */
    TEST_STRING("\xE2\x82\xAC", "\"\\u20AC\""); /* Euro sign U+20AC */
    TEST_STRING("\xF0\x9D\x84\x9E", "\"\\uD834\\uDD1E\"");  /* G clef sign U+1D11E */
    TEST_STRING("\xF0\x9D\x84\x9E", "\"\\ud834\\udd1e\"");  /* G clef sign U+1D11E */
}

static void test_parse_array() {
    //首先测试空数组 [] 的解析，检查类型和数组大小。
    //测试一个包含多种类型元素的数组[null, false, true, 123, "abc"]，验证数组大小和每个元素的类型及值。
    //进一步测试嵌套数组的解析，如 [[ ], [0], [0, 1], [0, 1, 2]] ，检查外层数组和内层数组的大小及元素的类型和值。
    //对于每个内层数组，使用嵌套的循环检查每个元素是否正确解析为预期的数字，并且数组的大小是否正确。
    size_t i, j;
    lept_value v;
    lept_init(&v);
    EXPECT_EQ_INT(LEPT_PARSE_OK, lept_parse(&v, "[ ]"));
    EXPECT_EQ_INT(LEPT_ARRAY, lept_get_type(&v));
    EXPECT_EQ_SIZE_T(0, lept_get_array_size(&v));
    lept_free(&v);

    lept_init(&v);
    EXPECT_EQ_INT(LEPT_PARSE_OK, lept_parse(&v, "[ null , false , true , 123 , \"abc\" ]"));
    EXPECT_EQ_INT(LEPT_ARRAY, lept_get_type(&v));
    EXPECT_EQ_SIZE_T(5, lept_get_array_size(&v));
    EXPECT_EQ_INT(LEPT_NULL, lept_get_type(lept_get_array_element(&v, 0)));
    EXPECT_EQ_INT(LEPT_FALSE, lept_get_type(lept_get_array_element(&v, 1)));
    EXPECT_EQ_INT(LEPT_TRUE, lept_get_type(lept_get_array_element(&v, 2)));
    EXPECT_EQ_INT(LEPT_NUMBER, lept_get_type(lept_get_array_element(&v, 3)));
    EXPECT_EQ_INT(LEPT_STRING, lept_get_type(lept_get_array_element(&v, 4)));
    EXPECT_EQ_DOUBLE(123.0, lept_get_number(lept_get_array_element(&v, 3)));
    EXPECT_EQ_STRING("abc", lept_get_string(lept_get_array_element(&v, 4)), lept_get_string_length(lept_get_array_element(&v, 4)));
    lept_free(&v);

    lept_init(&v);
    EXPECT_EQ_INT(LEPT_PARSE_OK, lept_parse(&v, "[ [ ] , [ 0 ] , [ 0 , 1 ] , [ 0 , 1 , 2 ] ]"));
    //使用 lept_parse 函数解析一个具有嵌套数组的 JSON 字符串。测试期望解析返回 LEPT_PARSE_OK，表示没有错误发生。
    EXPECT_EQ_INT(LEPT_ARRAY, lept_get_type(&v));
    // 检查解析后的结果类型是否为 LEPT_ARRAY，确保 v 中存储的是一个数组。
    EXPECT_EQ_SIZE_T(4, lept_get_array_size(&v));// 验证最外层数组的大小是否为 4，即包含四个子数组。
    for (i = 0; i < 4; i++) {// 对最外层数组的每一个元素进行遍历
        lept_value* a = lept_get_array_element(&v, i);
        EXPECT_EQ_INT(LEPT_ARRAY, lept_get_type(a));//验证获取的子数组 a 是否确实为一个数组类型。
        EXPECT_EQ_SIZE_T(i, lept_get_array_size(a));//验证子数组 a 的大小是否正确（第 i 个数组包含 i 个元素）。
        for (j = 0; j < i; j++) {//遍历子数组 a 的每一个元素
            lept_value* e = lept_get_array_element(a, j);
            EXPECT_EQ_INT(LEPT_NUMBER, lept_get_type(e));
            EXPECT_EQ_DOUBLE((double)j, lept_get_number(e));//确保数字的值与索引 j 相等，验证数字的准确性。
        }
    }
    lept_free(&v);// 在测试结束后释放 v 所占用的资源。这一步是清理操作，防止内存泄漏
}

static void test_parse_object() {
    lept_value v;// 声明一个 lept_value 类型的变量 v
    size_t i;//声明一个 size_t 类型的变量 i，用于循环索引。

    lept_init(&v);
    //解析空字符串
    EXPECT_EQ_INT(LEPT_PARSE_OK, lept_parse(&v, " { } "));
    EXPECT_EQ_INT(LEPT_OBJECT, lept_get_type(&v));
    EXPECT_EQ_SIZE_T(0, lept_get_object_size(&v));
    lept_free(&v);//lept_free(&v); 释放与 v 相关联的资源
    //
    lept_init(&v);//lept_init(&v); 再次初始化 v
    //使用 lept_parse 解析一个包含多个键值对的 JSON 对象。这个对象包括各种数据类型，如 null、布尔值、数字、字符串、数组和嵌套对象。
    EXPECT_EQ_INT(LEPT_PARSE_OK, lept_parse(&v,
        " { "
        "\"n\" : null , "
        "\"f\" : false , "
        "\"t\" : true , "
        "\"i\" : 123 , "
        "\"s\" : \"abc\", "
        "\"a\" : [ 1, 2, 3 ],"
        "\"o\" : { \"1\" : 1, \"2\" : 2, \"3\" : 3 }"
        " } "
    ));
    EXPECT_EQ_INT(LEPT_OBJECT, lept_get_type(&v));// 确认解析结果为对象。
    EXPECT_EQ_SIZE_T(7, lept_get_object_size(&v));//确认对象中有 7 个键值对
    EXPECT_EQ_STRING("n", lept_get_object_key(&v, 0), lept_get_object_key_length(&v, 0));
    EXPECT_EQ_INT(LEPT_NULL, lept_get_type(lept_get_object_value(&v, 0)));
    EXPECT_EQ_STRING("f", lept_get_object_key(&v, 1), lept_get_object_key_length(&v, 1));
    EXPECT_EQ_INT(LEPT_FALSE, lept_get_type(lept_get_object_value(&v, 1)));
    EXPECT_EQ_STRING("t", lept_get_object_key(&v, 2), lept_get_object_key_length(&v, 2));
    EXPECT_EQ_INT(LEPT_TRUE, lept_get_type(lept_get_object_value(&v, 2)));
    EXPECT_EQ_STRING("i", lept_get_object_key(&v, 3), lept_get_object_key_length(&v, 3));
    EXPECT_EQ_INT(LEPT_NUMBER, lept_get_type(lept_get_object_value(&v, 3)));
    EXPECT_EQ_DOUBLE(123.0, lept_get_number(lept_get_object_value(&v, 3)));
    EXPECT_EQ_STRING("s", lept_get_object_key(&v, 4), lept_get_object_key_length(&v, 4));
    EXPECT_EQ_INT(LEPT_STRING, lept_get_type(lept_get_object_value(&v, 4)));
    EXPECT_EQ_STRING("abc", lept_get_string(lept_get_object_value(&v, 4)), lept_get_string_length(lept_get_object_value(&v, 4)));
    EXPECT_EQ_STRING("a", lept_get_object_key(&v, 5), lept_get_object_key_length(&v, 5));
    EXPECT_EQ_INT(LEPT_ARRAY, lept_get_type(lept_get_object_value(&v, 5)));
    EXPECT_EQ_SIZE_T(3, lept_get_array_size(lept_get_object_value(&v, 5)));
    for (i = 0; i < 3; i++) {
        lept_value* e = lept_get_array_element(lept_get_object_value(&v, 5), i);
        EXPECT_EQ_INT(LEPT_NUMBER, lept_get_type(e));
        EXPECT_EQ_DOUBLE(i + 1.0, lept_get_number(e));
    }
    EXPECT_EQ_STRING("o", lept_get_object_key(&v, 6), lept_get_object_key_length(&v, 6));
    {
        lept_value* o = lept_get_object_value(&v, 6);
        EXPECT_EQ_INT(LEPT_OBJECT, lept_get_type(o));
        for (i = 0; i < 3; i++) {
            lept_value* ov = lept_get_object_value(o, i);
            EXPECT_TRUE('1' + i == lept_get_object_key(o, i)[0]);
            EXPECT_EQ_SIZE_T(1, lept_get_object_key_length(o, i));
            EXPECT_EQ_INT(LEPT_NUMBER, lept_get_type(ov));
            EXPECT_EQ_DOUBLE(i + 1.0, lept_get_number(ov));
        }
    }
    lept_free(&v);
    //测试函数通过构造一个包含各种数据类型的复杂 JSON 对象，系统地验证解析器能否正确处理和解析 JSON 对象。
    //每个步骤通过使用宏来验证期望的结果，确保解析器的行为符合预期
}

//#define TEST_PARSE_ERROR(error, json) 定义了一个宏，简化错误测试过程。
//它创建一个 lept_value v 对象，初始化它，并初始将其类型设置为 LEPT_FALSE。
//该宏使用 lept_parse 函数解析提供的 json 字符串，并期望得到特定的错误代码 error。
//然后检查解析后 v 的类型是否为 LEPT_NULL，这是解析错误后的预期类型。
//最后，使用 lept_free(v) 释放与 v 相关的资源。

#define TEST_PARSE_ERROR(error, json)\
    do {\
        lept_value v;\
        lept_init(&v);\
        v.type = LEPT_FALSE;\
        EXPECT_EQ_INT(error, lept_parse(&v, json));\
        EXPECT_EQ_INT(LEPT_NULL, lept_get_type(&v));\
        lept_free(&v);\
    } while(0)

static void test_parse_expect_value() {//测试当JSON解析器期望一个值但没有找到时的情况（如空字符串或只包含空格的字符串）。
    TEST_PARSE_ERROR(LEPT_PARSE_EXPECT_VALUE, "");
    TEST_PARSE_ERROR(LEPT_PARSE_EXPECT_VALUE, " ");
}

static void test_parse_invalid_value() {//测试几乎是有效JSON字面量的字符串（如 "nul" 而不是 "null"）或完全不相关的字符（如 "?"）。
    TEST_PARSE_ERROR(LEPT_PARSE_INVALID_VALUE, "nul");
    TEST_PARSE_ERROR(LEPT_PARSE_INVALID_VALUE, "?");

    /* invalid number */
    //检查不符合JSON数字规范的格式错误数字，例如带有前导加号的数字、小数点周围缺失数字，或无效字面量如 "INF" 或 "NAN"。
    TEST_PARSE_ERROR(LEPT_PARSE_INVALID_VALUE, "+0");
    TEST_PARSE_ERROR(LEPT_PARSE_INVALID_VALUE, "+1");
    TEST_PARSE_ERROR(LEPT_PARSE_INVALID_VALUE, ".123"); /* at least one digit before '.' */
    TEST_PARSE_ERROR(LEPT_PARSE_INVALID_VALUE, "1.");   /* at least one digit after '.' */
    TEST_PARSE_ERROR(LEPT_PARSE_INVALID_VALUE, "INF");
    TEST_PARSE_ERROR(LEPT_PARSE_INVALID_VALUE, "inf");
    TEST_PARSE_ERROR(LEPT_PARSE_INVALID_VALUE, "NAN");
    TEST_PARSE_ERROR(LEPT_PARSE_INVALID_VALUE, "nan");

    /* invalid value in array */
    //同时检查数组中错误的尾随逗号或数组内拼写错误的值
    TEST_PARSE_ERROR(LEPT_PARSE_INVALID_VALUE, "[1,]");
    TEST_PARSE_ERROR(LEPT_PARSE_INVALID_VALUE, "[\"a\", nul]");
}

static void test_parse_root_not_singular() {
    //测试在有效JSON文本后出现额外字符的情况，这是不允许的。JSON文本必须是单一的。
    //包括测试前导零的数字，这在JSON中除非数字为零后跟小数点，否则不允许。
    TEST_PARSE_ERROR(LEPT_PARSE_ROOT_NOT_SINGULAR, "null x");

    /* invalid number */
    TEST_PARSE_ERROR(LEPT_PARSE_ROOT_NOT_SINGULAR, "0123"); /* after zero should be '.' or nothing */
    TEST_PARSE_ERROR(LEPT_PARSE_ROOT_NOT_SINGULAR, "0x0");
    TEST_PARSE_ERROR(LEPT_PARSE_ROOT_NOT_SINGULAR, "0x123");
}

static void test_parse_number_too_big() {
    TEST_PARSE_ERROR(LEPT_PARSE_NUMBER_TOO_BIG, "1e309");
    TEST_PARSE_ERROR(LEPT_PARSE_NUMBER_TOO_BIG, "-1e309");
}

static void test_parse_miss_quotation_mark() {
    //检查缺少结束引号的字符串
    TEST_PARSE_ERROR(LEPT_PARSE_MISS_QUOTATION_MARK, "\"");
    TEST_PARSE_ERROR(LEPT_PARSE_MISS_QUOTATION_MARK, "\"abc");
}

static void test_parse_invalid_string_escape() {
    //测试在JSON中无效的字符串转义，如 \v（垂直制表符不是有效的JSON转义序列）和 \x12（十六进制转义在JSON中不有效）
    TEST_PARSE_ERROR(LEPT_PARSE_INVALID_STRING_ESCAPE, "\"\\v\"");
    TEST_PARSE_ERROR(LEPT_PARSE_INVALID_STRING_ESCAPE, "\"\\'\"");
    TEST_PARSE_ERROR(LEPT_PARSE_INVALID_STRING_ESCAPE, "\"\\0\"");
    TEST_PARSE_ERROR(LEPT_PARSE_INVALID_STRING_ESCAPE, "\"\\x12\"");
}

static void test_parse_invalid_string_char() {
    //测试字符串中不允许的控制字符，例如在JSON中不允许的ASCII值小于0x20的字符。
    TEST_PARSE_ERROR(LEPT_PARSE_INVALID_STRING_CHAR, "\"\x01\"");
    TEST_PARSE_ERROR(LEPT_PARSE_INVALID_STRING_CHAR, "\"\x1F\"");
}
//测试字符串中格式错误的Unicode转义，如序列太短、包含非十六进制字符或格式无效。
static void test_parse_invalid_unicode_hex() {
    //测试字符串中格式错误的Unicode转义，如序列太短、包含非十六进制字符或格式无效。
    TEST_PARSE_ERROR(LEPT_PARSE_INVALID_UNICODE_HEX, "\"\\u\"");
    TEST_PARSE_ERROR(LEPT_PARSE_INVALID_UNICODE_HEX, "\"\\u0\"");
    TEST_PARSE_ERROR(LEPT_PARSE_INVALID_UNICODE_HEX, "\"\\u01\"");
    TEST_PARSE_ERROR(LEPT_PARSE_INVALID_UNICODE_HEX, "\"\\u012\"");
    TEST_PARSE_ERROR(LEPT_PARSE_INVALID_UNICODE_HEX, "\"\\u/000\"");
    TEST_PARSE_ERROR(LEPT_PARSE_INVALID_UNICODE_HEX, "\"\\uG000\"");
    TEST_PARSE_ERROR(LEPT_PARSE_INVALID_UNICODE_HEX, "\"\\u0/00\"");
    TEST_PARSE_ERROR(LEPT_PARSE_INVALID_UNICODE_HEX, "\"\\u0G00\"");
    TEST_PARSE_ERROR(LEPT_PARSE_INVALID_UNICODE_HEX, "\"\\u0/00\"");
    TEST_PARSE_ERROR(LEPT_PARSE_INVALID_UNICODE_HEX, "\"\\u00G0\"");
    TEST_PARSE_ERROR(LEPT_PARSE_INVALID_UNICODE_HEX, "\"\\u000/\"");
    TEST_PARSE_ERROR(LEPT_PARSE_INVALID_UNICODE_HEX, "\"\\u000G\"");
    TEST_PARSE_ERROR(LEPT_PARSE_INVALID_UNICODE_HEX, "\"\\u 123\"");
}

static void test_parse_invalid_unicode_surrogate() {//测试无效的Unicode替代对
    TEST_PARSE_ERROR(LEPT_PARSE_INVALID_UNICODE_SURROGATE, "\"\\uD800\"");
    TEST_PARSE_ERROR(LEPT_PARSE_INVALID_UNICODE_SURROGATE, "\"\\uDBFF\"");
    TEST_PARSE_ERROR(LEPT_PARSE_INVALID_UNICODE_SURROGATE, "\"\\uD800\\\\\"");
    TEST_PARSE_ERROR(LEPT_PARSE_INVALID_UNICODE_SURROGATE, "\"\\uD800\\uDBFF\"");
    TEST_PARSE_ERROR(LEPT_PARSE_INVALID_UNICODE_SURROGATE, "\"\\uD800\\uE000\"");
}

static void test_parse_miss_comma_or_square_bracket() {//测试缺少逗号或方括号
    TEST_PARSE_ERROR(LEPT_PARSE_MISS_COMMA_OR_SQUARE_BRACKET, "[1");
    TEST_PARSE_ERROR(LEPT_PARSE_MISS_COMMA_OR_SQUARE_BRACKET, "[1}");
    TEST_PARSE_ERROR(LEPT_PARSE_MISS_COMMA_OR_SQUARE_BRACKET, "[1 2");
    TEST_PARSE_ERROR(LEPT_PARSE_MISS_COMMA_OR_SQUARE_BRACKET, "[[]");
}

static void test_parse_miss_key() {//测试缺少键
    TEST_PARSE_ERROR(LEPT_PARSE_MISS_KEY, "{:1,");
    TEST_PARSE_ERROR(LEPT_PARSE_MISS_KEY, "{1:1,");
    TEST_PARSE_ERROR(LEPT_PARSE_MISS_KEY, "{true:1,");
    TEST_PARSE_ERROR(LEPT_PARSE_MISS_KEY, "{false:1,");
    TEST_PARSE_ERROR(LEPT_PARSE_MISS_KEY, "{null:1,");
    TEST_PARSE_ERROR(LEPT_PARSE_MISS_KEY, "{[]:1,");
    TEST_PARSE_ERROR(LEPT_PARSE_MISS_KEY, "{{}:1,");
    TEST_PARSE_ERROR(LEPT_PARSE_MISS_KEY, "{\"a\":1,");
}

static void test_parse_miss_colon() {//测试缺少冒号
    TEST_PARSE_ERROR(LEPT_PARSE_MISS_COLON, "{\"a\"}");
    TEST_PARSE_ERROR(LEPT_PARSE_MISS_COLON, "{\"a\",\"b\"}");
}

static void test_parse_miss_comma_or_curly_bracket() {//测试缺少逗号或花括号
    TEST_PARSE_ERROR(LEPT_PARSE_MISS_COMMA_OR_CURLY_BRACKET, "{\"a\":1");
    TEST_PARSE_ERROR(LEPT_PARSE_MISS_COMMA_OR_CURLY_BRACKET, "{\"a\":1]");
    TEST_PARSE_ERROR(LEPT_PARSE_MISS_COMMA_OR_CURLY_BRACKET, "{\"a\":1 \"b\"");
    TEST_PARSE_ERROR(LEPT_PARSE_MISS_COMMA_OR_CURLY_BRACKET, "{\"a\":{}");
}

static void test_parse() {
    test_parse_null();
    test_parse_true();
    test_parse_false();
    test_parse_number();
    test_parse_string();
    test_parse_array();
    test_parse_object();

    test_parse_expect_value();
    test_parse_invalid_value();
    test_parse_root_not_singular();
    test_parse_number_too_big();
    test_parse_miss_quotation_mark();
    test_parse_invalid_string_escape();
    test_parse_invalid_string_char();
    test_parse_invalid_unicode_hex();
    test_parse_invalid_unicode_surrogate();
    test_parse_miss_comma_or_square_bracket();
    test_parse_miss_key();
    test_parse_miss_colon();
    test_parse_miss_comma_or_curly_bracket();
}
//目的是验证 leptjson JSON 库的字符串化（stringify）功能是否能够准确地将解析后的 JSON 数据结构重新转换回其原始的字符串形式。
//下面逐行解释每个部分的功能和作用：
//定义一个宏，用于测试 JSON 字符串的解析和重新字符串化的往返（round-trip）过程。
//lept_value v;：声明一个 lept_value 类型的变量 v，用于存储解析后的 JSON 数据。
//char* json2;：声明一个字符指针 json2，用于接收字符串化后的 JSON 字符串。
//size_t length;：声明一个变量 length，用于存储 json2 的长度
//lept_init(&v);：初始化 v
//EXPECT_EQ_INT(LEPT_PARSE_OK, lept_parse(&v, json));：解析输入的 JSON 字符串 json，期望解析无误（返回 LEPT_PARSE_OK）
//json2 = lept_stringify(&v, &length);：调用 lept_stringify 函数，将解析后的数据结构 v 转换回 JSON 字符串，并存储其长度
//EXPECT_EQ_STRING(json, json2, length);：验证原始的 JSON 字符串 json 与转换回来的字符串 json2 是否完全相同。
//lept_free(&v);：释放 v 所占用的资源。
//free(json2);：释放 json2 所占用的内存
#define TEST_ROUNDTRIP(json)\
    do {\
        lept_value v;\
        char* json2;\
        size_t length;\
        lept_init(&v);\
        EXPECT_EQ_INT(LEPT_PARSE_OK, lept_parse(&v, json));\
        json2 = lept_stringify(&v, &length);\
        EXPECT_EQ_STRING(json, json2, length);\
        lept_free(&v);\
        free(json2);\
    } while(0)

static void test_stringify_number() {
    //测试不同类型的数字，包括普通数字、负数、小数、科学记数法表示的数字，以及一些特殊的浮点数边界值，例如最小的正常数和最大的双精度数。
    TEST_ROUNDTRIP("0");
    TEST_ROUNDTRIP("-0");
    TEST_ROUNDTRIP("1");
    TEST_ROUNDTRIP("-1");
    TEST_ROUNDTRIP("1.5");
    TEST_ROUNDTRIP("-1.5");
    TEST_ROUNDTRIP("3.25");
    TEST_ROUNDTRIP("1e+20");
    TEST_ROUNDTRIP("1.234e+20");
    TEST_ROUNDTRIP("1.234e-20");

    TEST_ROUNDTRIP("1.0000000000000002"); /* the smallest number > 1 */
    TEST_ROUNDTRIP("4.9406564584124654e-324"); /* minimum denormal */
    TEST_ROUNDTRIP("-4.9406564584124654e-324");
    TEST_ROUNDTRIP("2.2250738585072009e-308");  /* Max subnormal double */
    TEST_ROUNDTRIP("-2.2250738585072009e-308");
    TEST_ROUNDTRIP("2.2250738585072014e-308");  /* Min normal positive double */
    TEST_ROUNDTRIP("-2.2250738585072014e-308");
    TEST_ROUNDTRIP("1.7976931348623157e+308");  /* Max double */
    TEST_ROUNDTRIP("-1.7976931348623157e+308");
}

static void test_stringify_string() {
    //测试字符串的字符串化，包括空字符串、普通字符串、包含转义字符的字符串以及包含 Unicode 转义序列的字符串
    TEST_ROUNDTRIP("\"\"");
    TEST_ROUNDTRIP("\"Hello\"");
    TEST_ROUNDTRIP("\"Hello\\nWorld\"");
    TEST_ROUNDTRIP("\"\\\" \\\\ / \\b \\f \\n \\r \\t\"");
    TEST_ROUNDTRIP("\"Hello\\u0000World\"");
}

static void test_stringify_array() {
    //测试数组的字符串化，包括空数组和包含多种数据类型元素的数组
    TEST_ROUNDTRIP("[]");
    TEST_ROUNDTRIP("[null,false,true,123,\"abc\",[1,2,3]]");
}

static void test_stringify_object() {
    //测试对象的字符串化，包括空对象和包含多种类型键值对的对象。
    TEST_ROUNDTRIP("{}");
    TEST_ROUNDTRIP("{\"n\":null,\"f\":false,\"t\":true,\"i\":123,\"s\":\"abc\",\"a\":[1,2,3],\"o\":{\"1\":1,\"2\":2,\"3\":3}}");
}

static void test_stringify() {
    //调用所有子测试函数，测试 null、true、false 的字符串化，以及上述的数字、字符串、数组和对象的字符串化。
    TEST_ROUNDTRIP("null");
    TEST_ROUNDTRIP("false");
    TEST_ROUNDTRIP("true");
    test_stringify_number();
    test_stringify_string();
    test_stringify_array();
    test_stringify_object();
}

static void test_access_null() {//test_access_null
    lept_value v;
    lept_init(&v);
    lept_set_string(&v, "a", 1);
    lept_set_null(&v);
    EXPECT_EQ_INT(LEPT_NULL, lept_get_type(&v));
    lept_free(&v);
}

static void test_access_boolean() {
    lept_value v;
    lept_init(&v);
    lept_set_string(&v, "a", 1);
    lept_set_boolean(&v, 1);
    EXPECT_TRUE(lept_get_boolean(&v));
    lept_set_boolean(&v, 0);
    EXPECT_FALSE(lept_get_boolean(&v));
    lept_free(&v);
}

static void test_access_number() {
    lept_value v;
    lept_init(&v);
    lept_set_string(&v, "a", 1);
    lept_set_number(&v, 1234.5);
    EXPECT_EQ_DOUBLE(1234.5, lept_get_number(&v));
    lept_free(&v);
}

static void test_access_string() {
    lept_value v;
    lept_init(&v);
    lept_set_string(&v, "", 0);
    EXPECT_EQ_STRING("", lept_get_string(&v), lept_get_string_length(&v));
    lept_set_string(&v, "Hello", 5);
    EXPECT_EQ_STRING("Hello", lept_get_string(&v), lept_get_string_length(&v));
    lept_free(&v);
}

static void test_access() {
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
    test_stringify();
    test_access();
    printf("%d/%d (%3.2f%%) passed\n", test_pass, test_count, test_pass * 100.0 / test_count);
    return main_ret;
}
