////////////////////////////////////////////////////////////////////////��һ����ϰ
#include <stdio.h>//��׼��������⣬����printf��fprintf�Ⱥ�����
#include <stdlib.h>
#include <string.h>//��׼��������⣬����printf��fprintf�Ⱥ�����
#include "leptjson.h"//JSON���������ͷ�ļ��������˿�Ľӿں����ݽṹ��

static int main_ret = 0;//����������ֵ������ָʾ�����Ƿ���ȫͨ����
static int test_count = 0;//���еĲ�������
static int test_pass = 0;//ͨ���Ĳ�����

//����һ���꣬���ڱȽ�����ֵ��ʵ��ֵ�Ƿ���ȡ������ȣ�������test_pass��������ȣ����ӡ������Ϣ������main_retΪ1
//equality ��ʾԤ�ڵ�����ԣ����٣���expect ��ʾԤ�ڵ�ֵ��actual ��ʾʵ�ʵ�ֵ��format �Ǵ�ӡֵ�ĸ�ʽ�ַ�����
// do { ... } while(0) �ṹ�����������ں���ʹ�ö���������ᵼ���﷨����
//��ʾͨ���Ĳ�������1�����򣬴�ӡ������Ϣ����׼������ stderr �У����� main_ret ��Ϊ 1����ʾ����ʧ�ܡ�
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
//ר�����������Ƚϵĺ꣬�������涨���EXPECT_EQ_BASE��
//����һ������ EXPECT_EQ_BASE ���ר�����������Ƚϵĺꡣ����������������expect ��ʾԤ�ڵ�����ֵ��actual ��ʾʵ�ʵ�����ֵ��
//�������� EXPECT_EQ_BASE �꣬�� expect �� actual ���ݸ��������� equality ��Ϊ(expect) == (actual)����ʾԤ�ڵ�����ֵ��ʵ�ʵ�����ֵ�Ƿ���ȡ�
#define EXPECT_EQ_INT(expect, actual) EXPECT_EQ_BASE((expect) == (actual), expect, actual, "%d")

//���Խ������Ƿ�����ȷ����nullֵ����ʼ��������ΪLEPT_FALSE��Ԥ�ڽ���������ΪLEPT_NULL��
static void test_parse_null() {
    lept_value v;
    v.type = LEPT_FALSE;
    EXPECT_EQ_INT(LEPT_PARSE_OK, lept_parse(&v, "null"));
    EXPECT_EQ_INT(LEPT_NULL, lept_get_type(&v));
}

/// /////////////////////////////////////////////////////////
//���Խ������Ƿ�����ȷ����trueֵ����ʼ��������ΪLEPT_FALSE��Ԥ�ڽ���������ΪLEPT_NULL��
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
//����JSON�������Բ�ͬ����Ĵ���������
//���Ե�JSON�ַ������벻����Ԥ��ʱ��JSON����������Ϊ������أ��������˽������Կ��ַ����ͽ������հ��ַ����ַ����Ĵ���
//��JSON�У���Ч������������Ҫ����һ��ֵ����null��false��true�����֡��ַ������������󣩡��������Ϊ�ջ�ֻ�пհף�
//������JSON���﷨�������Ӧ�÷���һ���ض��Ĵ�����롣
static void test_parse_expect_value() {
    lept_value v;
    v.type = LEPT_FALSE;
    EXPECT_EQ_INT(LEPT_PARSE_EXPECT_VALUE, lept_parse(&v, ""));
    EXPECT_EQ_INT(LEPT_NULL, lept_get_type(&v));
    v.type = LEPT_FALSE;
    EXPECT_EQ_INT(LEPT_PARSE_EXPECT_VALUE, lept_parse(&v, " "));
    EXPECT_EQ_INT(LEPT_NULL, lept_get_type(&v));
}
//���۲��Ϸ������
static void test_parse_invalid_value() {
    lept_value v;
    v.type = LEPT_FALSE;
    EXPECT_EQ_INT(LEPT_PARSE_INVALID_VALUE, lept_parse(&v, "nul"));
    EXPECT_EQ_INT(LEPT_NULL, lept_get_type(&v));

    v.type = LEPT_FALSE;
    EXPECT_EQ_INT(LEPT_PARSE_INVALID_VALUE, lept_parse(&v, "?"));
    EXPECT_EQ_INT(LEPT_NULL, lept_get_type(&v));
}
//һ���Ϸ���JSON�ĵ�Ӧ��ֻ����һ����һ��ֵ����һ���������顢��ֵ���ַ���������ֵ��null��
//�����һ����Ч��JSONֵ֮���������ַ�����ô��Ӧ�ñ���Ϊһ������
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
