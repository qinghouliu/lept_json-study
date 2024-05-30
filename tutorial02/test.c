#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "leptjson.h"
//静态全局变量 main_ret、test_count、test_pass，用于记录测试结果和统计测试通过率。
static int main_ret = 0;
static int test_count = 0;
static int test_pass = 0;
//用于判断两个值是否相等，若相等则测试通过，否则输出错误信息并更新统计数据。
//如果不相等，使用 fprintf 打印错误信息到标准错误流 stderr，显示预期值和实际值，
//并将 main_ret（主函数返回值）设置为 1，表示测试失败。
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
//定义两个宏 EXPECT_EQ_INT 和 EXPECT_EQ_DOUBLE，用于比较整数和浮点数的相等性
#define EXPECT_EQ_INT(expect, actual) EXPECT_EQ_BASE((expect) == (actual), expect, actual, "%d")
#define EXPECT_EQ_DOUBLE(expect, actual) EXPECT_EQ_BASE((expect) == (actual), expect, actual, "%.17g")

//测试解析器是否能正确解析null值。初始类型设置为LEPT_FALSE，预期解析后类型为LEPT_NULL。
static void test_parse_null() {
    lept_value v;
    v.type = LEPT_FALSE;
    EXPECT_EQ_INT(LEPT_PARSE_OK, lept_parse(&v, "null"));
    EXPECT_EQ_INT(LEPT_NULL, lept_get_type(&v));
}
//测试解析器是否能正确解析true值。初始类型设置为LEPT_FALSE，预期解析后类型为LEPT_NULL。
static void test_parse_true() {
    lept_value v;
    v.type = LEPT_FALSE;
    EXPECT_EQ_INT(LEPT_PARSE_OK, lept_parse(&v, "true"));
    EXPECT_EQ_INT(LEPT_TRUE, lept_get_type(&v));
}
//测试解析器是否能正确解析false值。初始类型设置为LEPT_FALSE，预期解析后类型为LEPT_NULL。
static void test_parse_false() {
    lept_value v;
    v.type = LEPT_TRUE;
    EXPECT_EQ_INT(LEPT_PARSE_OK, lept_parse(&v, "false"));
    EXPECT_EQ_INT(LEPT_FALSE, lept_get_type(&v));
}
//定义宏 TEST_NUMBER，用于测试解析器是否能正确解析 JSON 中的数字。
//声明了一个 lept_value 类型的变量 v，用于存储解析后的值
//EXPECT_EQ_INT 宏进行测试，检查解析给定的 json 字符串是否返回了期望的解析状态
//使用 EXPECT_EQ_INT 宏进行测试，检查解析后的值 v 的类型是否是 LEPT_NUMBER（表示解析结果为数字
//使用 EXPECT_EQ_DOUBLE 宏进行测试，检查解析后的值 v 的数字值是否与预期的 expect 相等。
//lept_get_number(&v) 返回解析后的数字值。
#define TEST_NUMBER(expect, json)\
    do {\
        lept_value v;\
        EXPECT_EQ_INT(LEPT_PARSE_OK, lept_parse(&v, json));\
        EXPECT_EQ_INT(LEPT_NUMBER, lept_get_type(&v));\
        EXPECT_EQ_DOUBLE(expect, lept_get_number(&v));\
    } while(0)
//这段代码讲的是先检验这个东西能不能被解析器进行解析
//解析成功，判断解析的值是不是数字
//是数字的话，和输入的值是不是相等



//定义测试函数 test_parse_number，用于执行多种数字解析的测试。
//expect 是期望的解析结果，即预期的数值。
//json 是要解析的 JSON 数字字符串。
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
    //////////////////////////////////////////////////////////////////////////////////////////////////
    TEST_NUMBER(1.0000000000000002, "1.0000000000000002"); /* the smallest number > 1 */
    TEST_NUMBER(4.9406564584124654e-324, "4.9406564584124654e-324"); /* minimum denormal */
    TEST_NUMBER(-4.9406564584124654e-324, "-4.9406564584124654e-324");
    TEST_NUMBER(2.2250738585072009e-308, "2.2250738585072009e-308");  /* Max subnormal double */
    TEST_NUMBER(-2.2250738585072009e-308, "-2.2250738585072009e-308");
    TEST_NUMBER(2.2250738585072014e-308, "2.2250738585072014e-308");  /* Min normal positive double */
    TEST_NUMBER(-2.2250738585072014e-308, "-2.2250738585072014e-308");
    TEST_NUMBER(1.7976931348623157e+308, "1.7976931348623157e+308");  /* Max double */
    TEST_NUMBER(-1.7976931348623157e+308, "-1.7976931348623157e+308");
    /////////////////////////////////////////////////////////////////////////////////////////////////

}
//定义宏 TEST_ERROR，用于测试解析器在遇到错误时的行为。
#define TEST_ERROR(error, json)\
    do {\
        lept_value v;\
        v.type = LEPT_FALSE;\
        EXPECT_EQ_INT(error, lept_parse(&v, json));\
        EXPECT_EQ_INT(LEPT_NULL, lept_get_type(&v));\
    } while(0)

//定义测试函数 test_parse_expect_value，测试解析器在预期值缺失情况下的行为。
static void test_parse_expect_value() {
    TEST_ERROR(LEPT_PARSE_EXPECT_VALUE, "");
    TEST_ERROR(LEPT_PARSE_EXPECT_VALUE, " ");
}
//定义测试函数 test_parse_invalid_value，测试解析器在遇到无效值情况下的行为。
static void test_parse_invalid_value() {
    TEST_ERROR(LEPT_PARSE_INVALID_VALUE, "nul");
    TEST_ERROR(LEPT_PARSE_INVALID_VALUE, "?");
/////////////////////////////////////////////////////////////////////////
//#if 0
    /* invalid number */
    TEST_ERROR(LEPT_PARSE_INVALID_VALUE, "+0");
    TEST_ERROR(LEPT_PARSE_INVALID_VALUE, "+1");
    TEST_ERROR(LEPT_PARSE_INVALID_VALUE, ".123"); /* at least one digit before '.' */
    TEST_ERROR(LEPT_PARSE_INVALID_VALUE, "1.");   /* at least one digit after '.' */
    TEST_ERROR(LEPT_PARSE_INVALID_VALUE, "INF");
    TEST_ERROR(LEPT_PARSE_INVALID_VALUE, "inf");
    TEST_ERROR(LEPT_PARSE_INVALID_VALUE, "NAN");
    TEST_ERROR(LEPT_PARSE_INVALID_VALUE, "nan");
//#endif
/////////////////////////////////////////////////////////////////////////
}
//如果在一个有效的JSON值之后还有其他字符，那么这应该被视为一个错误。
//定义测试函数 test_parse_root_not_singular，测试解析器在根节点不唯一情况下的行为。
static void test_parse_root_not_singular() {
    TEST_ERROR(LEPT_PARSE_ROOT_NOT_SINGULAR, "null x");
    /* invalid number */
    TEST_ERROR(LEPT_PARSE_ROOT_NOT_SINGULAR, "0123"); /* after zero should be '.' , 'E' , 'e' or nothing */
    TEST_ERROR(LEPT_PARSE_ROOT_NOT_SINGULAR, "0x0");
    TEST_ERROR(LEPT_PARSE_ROOT_NOT_SINGULAR, "0x123");
}

//定义测试函数 test_parse_number_too_big，测试解析器在解析过大数字时的行为。
static void test_parse_number_too_big() {
//#if 0
    TEST_ERROR(LEPT_PARSE_NUMBER_TOO_BIG, "1e309");
    TEST_ERROR(LEPT_PARSE_NUMBER_TOO_BIG, "-1e309");
//#endif
}
//定义主测试函数 test_parse，用于执行所有解析器的测试
static void test_parse() {
    test_parse_null();
    test_parse_true();
    test_parse_false();
    test_parse_number();
    test_parse_expect_value();
    test_parse_invalid_value();
    test_parse_root_not_singular();
    test_parse_number_too_big();
}
//在 main 函数中调用 test_parse 函数执行所有测试，并输出测试结果（通过率）到控制台。
int main() {
    test_parse();
    printf("%d/%d (%3.2f%%) passed\n", test_pass, test_count, test_pass * 100.0 / test_count);
    return main_ret;
}

//TEST_NUMBER(0.0, "1e-10000"); : 这个测试用例旨在检查解析器在面对极小的科学计数法表示的数字时的行为。字符串 "1e-10000" 表示一个非常接近于零的数值，这会导致数值下溢（数值太小而无法表示），因此期望结果为 0.0。
//TEST_NUMBER(1.0000000000000002, "1.0000000000000002"); : 这个测试用例用于验证解析器对精度要求较高的浮点数的处理能力。字符串 "1.0000000000000002" 表示一个最接近于 1.0 但稍微大一点的数值，用于检查解析器在保留浮点数精度方面的准确性。
//TEST_NUMBER(4.9406564584124654e-324, "4.9406564584124654e-324"); : 这个测试用例测试解析器对于最小的非规格化浮点数（denormal）的支持。这个数值接近于 C 语言中的 DBL_MIN，用于检查解析器对极小值的处理是否正确。
//TEST_NUMBER(-4.9406564584124654e-324, "-4.9406564584124654e-324"); : 类似于上一个测试用例，但包括了负数形式，用于测试解析器对负数的处理。
//TEST_NUMBER(2.2250738585072009e-308, "2.2250738585072009e-308"); : 这个测试用例用于验证解析器对于最大的非规格化浮点数（subnormal）的支持。
//TEST_NUMBER(-2.2250738585072009e-308, "-2.2250738585072009e-308"); : 类似于上一个测试用例，但包括了负数形式，用于测试解析器对负数的处理。
//TEST_NUMBER(2.2250738585072014e-308, "2.2250738585072014e-308"); : 这个测试用例用于验证解析器对于最小的正常浮点数（normal positive double）的支持。
//TEST_NUMBER(-2.2250738585072014e-308, "-2.2250738585072014e-308"); : 类似于上一个测试用例，但包括了负数形式，用于测试解析器对负数的处理。
//TEST_NUMBER(1.7976931348623157e+308, "1.7976931348623157e+308"); : 这个测试用例测试解析器对于最大的双精度浮点数（DBL_MAX）的支持。
//TEST_NUMBER(-1.7976931348623157e+308, "-1.7976931348623157e+308"); : 类似于上一个测试用例，但包括了负数形式，用于测试解析器对负数的处理