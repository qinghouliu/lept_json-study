#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "leptjson.h"
//��̬ȫ�ֱ��� main_ret��test_count��test_pass�����ڼ�¼���Խ����ͳ�Ʋ���ͨ���ʡ�
static int main_ret = 0;
static int test_count = 0;
static int test_pass = 0;
//�����ж�����ֵ�Ƿ���ȣ�����������ͨ�����������������Ϣ������ͳ�����ݡ�
//�������ȣ�ʹ�� fprintf ��ӡ������Ϣ����׼������ stderr����ʾԤ��ֵ��ʵ��ֵ��
//���� main_ret������������ֵ������Ϊ 1����ʾ����ʧ�ܡ�
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
//���������� EXPECT_EQ_INT �� EXPECT_EQ_DOUBLE�����ڱȽ������͸������������
#define EXPECT_EQ_INT(expect, actual) EXPECT_EQ_BASE((expect) == (actual), expect, actual, "%d")
#define EXPECT_EQ_DOUBLE(expect, actual) EXPECT_EQ_BASE((expect) == (actual), expect, actual, "%.17g")

//���Խ������Ƿ�����ȷ����nullֵ����ʼ��������ΪLEPT_FALSE��Ԥ�ڽ���������ΪLEPT_NULL��
static void test_parse_null() {
    lept_value v;
    v.type = LEPT_FALSE;
    EXPECT_EQ_INT(LEPT_PARSE_OK, lept_parse(&v, "null"));
    EXPECT_EQ_INT(LEPT_NULL, lept_get_type(&v));
}
//���Խ������Ƿ�����ȷ����trueֵ����ʼ��������ΪLEPT_FALSE��Ԥ�ڽ���������ΪLEPT_NULL��
static void test_parse_true() {
    lept_value v;
    v.type = LEPT_FALSE;
    EXPECT_EQ_INT(LEPT_PARSE_OK, lept_parse(&v, "true"));
    EXPECT_EQ_INT(LEPT_TRUE, lept_get_type(&v));
}
//���Խ������Ƿ�����ȷ����falseֵ����ʼ��������ΪLEPT_FALSE��Ԥ�ڽ���������ΪLEPT_NULL��
static void test_parse_false() {
    lept_value v;
    v.type = LEPT_TRUE;
    EXPECT_EQ_INT(LEPT_PARSE_OK, lept_parse(&v, "false"));
    EXPECT_EQ_INT(LEPT_FALSE, lept_get_type(&v));
}
//����� TEST_NUMBER�����ڲ��Խ������Ƿ�����ȷ���� JSON �е����֡�
//������һ�� lept_value ���͵ı��� v�����ڴ洢�������ֵ
//EXPECT_EQ_INT ����в��ԣ������������� json �ַ����Ƿ񷵻��������Ľ���״̬
//ʹ�� EXPECT_EQ_INT ����в��ԣ����������ֵ v �������Ƿ��� LEPT_NUMBER����ʾ�������Ϊ����
//ʹ�� EXPECT_EQ_DOUBLE ����в��ԣ����������ֵ v ������ֵ�Ƿ���Ԥ�ڵ� expect ��ȡ�
//lept_get_number(&v) ���ؽ����������ֵ��
#define TEST_NUMBER(expect, json)\
    do {\
        lept_value v;\
        EXPECT_EQ_INT(LEPT_PARSE_OK, lept_parse(&v, json));\
        EXPECT_EQ_INT(LEPT_NUMBER, lept_get_type(&v));\
        EXPECT_EQ_DOUBLE(expect, lept_get_number(&v));\
    } while(0)
//��δ��뽲�����ȼ�����������ܲ��ܱ����������н���
//�����ɹ����жϽ�����ֵ�ǲ�������
//�����ֵĻ����������ֵ�ǲ������



//������Ժ��� test_parse_number������ִ�ж������ֽ����Ĳ��ԡ�
//expect �������Ľ����������Ԥ�ڵ���ֵ��
//json ��Ҫ������ JSON �����ַ�����
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
//����� TEST_ERROR�����ڲ��Խ���������������ʱ����Ϊ��
#define TEST_ERROR(error, json)\
    do {\
        lept_value v;\
        v.type = LEPT_FALSE;\
        EXPECT_EQ_INT(error, lept_parse(&v, json));\
        EXPECT_EQ_INT(LEPT_NULL, lept_get_type(&v));\
    } while(0)

//������Ժ��� test_parse_expect_value�����Խ�������Ԥ��ֵȱʧ����µ���Ϊ��
static void test_parse_expect_value() {
    TEST_ERROR(LEPT_PARSE_EXPECT_VALUE, "");
    TEST_ERROR(LEPT_PARSE_EXPECT_VALUE, " ");
}
//������Ժ��� test_parse_invalid_value�����Խ�������������Чֵ����µ���Ϊ��
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
//�����һ����Ч��JSONֵ֮���������ַ�����ô��Ӧ�ñ���Ϊһ������
//������Ժ��� test_parse_root_not_singular�����Խ������ڸ��ڵ㲻Ψһ����µ���Ϊ��
static void test_parse_root_not_singular() {
    TEST_ERROR(LEPT_PARSE_ROOT_NOT_SINGULAR, "null x");
    /* invalid number */
    TEST_ERROR(LEPT_PARSE_ROOT_NOT_SINGULAR, "0123"); /* after zero should be '.' , 'E' , 'e' or nothing */
    TEST_ERROR(LEPT_PARSE_ROOT_NOT_SINGULAR, "0x0");
    TEST_ERROR(LEPT_PARSE_ROOT_NOT_SINGULAR, "0x123");
}

//������Ժ��� test_parse_number_too_big�����Խ������ڽ�����������ʱ����Ϊ��
static void test_parse_number_too_big() {
//#if 0
    TEST_ERROR(LEPT_PARSE_NUMBER_TOO_BIG, "1e309");
    TEST_ERROR(LEPT_PARSE_NUMBER_TOO_BIG, "-1e309");
//#endif
}
//���������Ժ��� test_parse������ִ�����н������Ĳ���
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
//�� main �����е��� test_parse ����ִ�����в��ԣ���������Խ����ͨ���ʣ�������̨��
int main() {
    test_parse();
    printf("%d/%d (%3.2f%%) passed\n", test_pass, test_count, test_pass * 100.0 / test_count);
    return main_ret;
}

//TEST_NUMBER(0.0, "1e-10000"); : �����������ּ�ڼ�����������Լ�С�Ŀ�ѧ��������ʾ������ʱ����Ϊ���ַ��� "1e-10000" ��ʾһ���ǳ��ӽ��������ֵ����ᵼ����ֵ���磨��ֵ̫С���޷���ʾ��������������Ϊ 0.0��
//TEST_NUMBER(1.0000000000000002, "1.0000000000000002"); : �����������������֤�������Ծ���Ҫ��ϸߵĸ������Ĵ����������ַ��� "1.0000000000000002" ��ʾһ����ӽ��� 1.0 ����΢��һ�����ֵ�����ڼ��������ڱ������������ȷ����׼ȷ�ԡ�
//TEST_NUMBER(4.9406564584124654e-324, "4.9406564584124654e-324"); : ��������������Խ�����������С�ķǹ�񻯸�������denormal����֧�֡������ֵ�ӽ��� C �����е� DBL_MIN�����ڼ��������Լ�Сֵ�Ĵ����Ƿ���ȷ��
//TEST_NUMBER(-4.9406564584124654e-324, "-4.9406564584124654e-324"); : ��������һ�������������������˸�����ʽ�����ڲ��Խ������Ը����Ĵ���
//TEST_NUMBER(2.2250738585072009e-308, "2.2250738585072009e-308"); : �����������������֤�������������ķǹ�񻯸�������subnormal����֧�֡�
//TEST_NUMBER(-2.2250738585072009e-308, "-2.2250738585072009e-308"); : ��������һ�������������������˸�����ʽ�����ڲ��Խ������Ը����Ĵ���
//TEST_NUMBER(2.2250738585072014e-308, "2.2250738585072014e-308"); : �����������������֤������������С��������������normal positive double����֧�֡�
//TEST_NUMBER(-2.2250738585072014e-308, "-2.2250738585072014e-308"); : ��������һ�������������������˸�����ʽ�����ڲ��Խ������Ը����Ĵ���
//TEST_NUMBER(1.7976931348623157e+308, "1.7976931348623157e+308"); : ��������������Խ�������������˫���ȸ�������DBL_MAX����֧�֡�
//TEST_NUMBER(-1.7976931348623157e+308, "-1.7976931348623157e+308"); : ��������һ�������������������˸�����ʽ�����ڲ��Խ������Ը����Ĵ���