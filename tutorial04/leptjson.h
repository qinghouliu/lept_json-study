#ifndef LEPTJSON_H__
#define LEPTJSON_H__
//��ֹͬһ���ļ�����ͬһ���뵥Ԫ�ڰ����Ĳ�ͬ�ļ��ж�ΰ���ͬһ��ͷ�ļ���
// ���LEPTJSON_H__δ�����壬Ԥ�������ᶨ�����������ļ����ݡ�����Ѷ��壬����������
#include <stddef.h> /* size_t */
//��׼ͷ�ļ�<stddef.h>���������˶������ͺͺ꣬�ر���size_t�����ڱ�ʾ�ڴ��С��
typedef enum { LEPT_NULL, LEPT_FALSE, LEPT_TRUE, LEPT_NUMBER, LEPT_STRING, LEPT_ARRAY, LEPT_OBJECT } lept_type;
//lept_type��ö�٣��г���һ�� JSON ʵ����ܾ��е�ֵ���ͣ������� JSON �����е����ͣ�null��boolean��number��string��array��object��
typedef struct {
    union {
        struct { char* s; size_t len; }s;  /* string: null-terminated string, string length */
        double n;                          /* number */
    }u;
    lept_type type;
}lept_value;
//lept_value����ʾһ�� JSON ֵ��ʹ��һ��union���洢�ַ��������ַ�ָ��ͳ���size_t��ʾ�������֣�double����
//lept_type��Աָ�������嵱ǰ�洢��ֵ�����͡�
enum {
    LEPT_PARSE_OK = 0,
    LEPT_PARSE_EXPECT_VALUE,
    LEPT_PARSE_INVALID_VALUE,
    LEPT_PARSE_ROOT_NOT_SINGULAR,
    LEPT_PARSE_NUMBER_TOO_BIG,
    LEPT_PARSE_MISS_QUOTATION_MARK,
    LEPT_PARSE_INVALID_STRING_ESCAPE,
    LEPT_PARSE_INVALID_STRING_CHAR,
    LEPT_PARSE_INVALID_UNICODE_HEX,
    LEPT_PARSE_INVALID_UNICODE_SURROGATE
};
//�г��� JSON ���������ܷ��صĸ��ִ�����룬�����˽��������з����Ĵ������͡�
#define lept_init(v) do { (v)->type = LEPT_NULL; } while(0)
//���ڳ�ʼ��һ��lept_valueʵ����������������ΪLEPT_NULL����ʹ�� do-while ѭ��ʹ���Ϊһ��������䣬���﷨�ϱ��ֵ���һ����ͨ�ĺ�������

int lept_parse(lept_value* v, const char* json);
//����һ��ָ��lept_value��ָ���һ�� JSON �����ַ����������� JSON �ַ�����������洢��lept_value�ṹ�С�

void lept_free(lept_value* v);
//����lept_free�������ͷź�����lept_valueʵ���е���Դ��

lept_type lept_get_type(const lept_value* v);//����lept_value�����͡�

#define lept_set_null(v) lept_free(v)//ͨ���ͷ�������lept_value����Ϊ null������lept_free������������ΪLEPT_NULL��

int lept_get_boolean(const lept_value* v);//��ȡ������lept_value����ֵ�ĺ���
void lept_set_boolean(lept_value* v, int b);

//��ȡ������lept_value��ֵ�ĺ���
double lept_get_number(const lept_value* v);
void lept_set_number(lept_value* v, double n);

//lept_value��ȡ�ַ���ֵ���䳤�ȵĺ������Լ������ַ���ֵ�ĺ�����
const char* lept_get_string(const lept_value* v);
size_t lept_get_string_length(const lept_value* v);
void lept_set_string(lept_value* v, const char* s, size_t len);
#endif /* LEPTJSON_H__ */
