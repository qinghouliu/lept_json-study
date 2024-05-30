#ifndef LEPTJSON_H__
#define LEPTJSON_H__
//�����˱�׼��ͷ�ļ� <stddef.h>���Ա�ʹ�� size_t ���͡�
#include <stddef.h> /* size_t */
//������3���������� LEPT_STRING, LEPT_ARRAY, LEPT_OBJECT�����������ӳ����JSON����
typedef enum { LEPT_NULL, LEPT_FALSE, LEPT_TRUE, LEPT_NUMBER, LEPT_STRING, LEPT_ARRAY, LEPT_OBJECT } lept_type;
//lept_value �ṹ�����ڱ�ʾ JSON ֵ������һ�������� u ��һ����ʾֵ���͵�ö�� type��
//������ u ���Դ洢�������͵�ֵ���ַ�����ͨ�� s ��Ա��ʾ�������ַ���ָ�� s ���ַ������� len����˫���ȸ�������ͨ�� n ��Ա��ʾ����
//type ��һ�� lept_type ö�����͵ı��������ڱ�ʶ u �д洢��ֵ���͡�
typedef struct {
    union {
        struct { char* s; size_t len; }s;  /* string: null-terminated string, string length */
        double n;                          /* number */
    }u;
    lept_type type;
}lept_value;

//�����ӵ�LEPT_PARSE_MISS_QUOTATION_MARK,
//  LEPT_PARSE_INVALID_STRING_ESCAPE,
//  LEPT_PARSE_INVALID_STRING_CHAR

enum {
    LEPT_PARSE_OK = 0,
    LEPT_PARSE_EXPECT_VALUE,
    LEPT_PARSE_INVALID_VALUE,
    LEPT_PARSE_ROOT_NOT_SINGULAR,
    LEPT_PARSE_NUMBER_TOO_BIG,
    LEPT_PARSE_MISS_QUOTATION_MARK,
    LEPT_PARSE_INVALID_STRING_ESCAPE,
    LEPT_PARSE_INVALID_STRING_CHAR
};
//lept_init(v): ������һ���꣬���ڳ�ʼ�� lept_value �ṹ�塣չ�����Ч���ǽ� v �� type ��Ա����Ϊ LEPT_NULL
#define lept_init(v) do { (v)->type = LEPT_NULL; } while(0)
//��������� JSON �ַ��� json��������������洢�� lept_value �ṹ�� v �С�
int lept_parse(lept_value* v, const char* json);
//�ͷ� lept_value �ṹ�� v �е���Դ�������ַ����ڴ��
void lept_free(lept_value* v);
//��ȡ lept_value �ṹ�� v �д洢��ֵ����
lept_type lept_get_type(const lept_value* v);
//�궨�壬���ڽ� lept_value �ṹ�� v �е�ֵ��Ϊ LEPT_NULL ���ͷ������Դ
#define lept_set_null(v) lept_free(v)
//��ȡ lept_value �ṹ�� v �д洢�Ĳ���ֵ
int lept_get_boolean(const lept_value* v);
//���� lept_value �ṹ�� v �е�ֵΪ�������͡�
void lept_set_boolean(lept_value* v, int b);
//��ȡ lept_value �ṹ�� v �д洢����ֵ��
double lept_get_number(const lept_value* v);
//���� lept_value �ṹ�� v �е�ֵΪ��ֵ���͡�
void lept_set_number(lept_value* v, double n);
//��ȡ lept_value �ṹ�� v �д洢���ַ���
const char* lept_get_string(const lept_value* v);
//��ȡ lept_value �ṹ�� v �д洢���ַ�������
size_t lept_get_string_length(const lept_value* v);
//���� lept_value �ṹ�� v �е�ֵΪ�ַ�������
void lept_set_string(lept_value* v, const char* s, size_t len);
#endif /* LEPTJSON_H__ */
