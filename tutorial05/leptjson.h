#ifndef LEPTJSON_H__           // ���û�ж����LEPTJSON_H__
#define LEPTJSON_H__           // �����LEPTJSON_H__

#include <stddef.h>           // ����ͷ�ļ�stddef.h���ṩsize_t������

typedef enum {                 // ����һ��ö������lept_type�����ڱ�ʾJSONֵ������
    LEPT_NULL,                 // JSON��null����
    LEPT_FALSE,                // JSON��false����
    LEPT_TRUE,                 // JSON��true����
    LEPT_NUMBER,               // JSON��number����
    LEPT_STRING,               // JSON��string����
    LEPT_ARRAY,                // JSON��array����
    LEPT_OBJECT                // JSON��object����
} lept_type;

typedef struct lept_value lept_value; // ǰ������һ���ṹ��lept_value

struct lept_value {                 // ����ṹ��lept_value�����ڱ�ʾһ��JSONֵ
    union {                         // ʹ������������ʡ�ڴ棬ֻ�洢��ǰֵ���������������
        struct { lept_value* e; size_t size; }a;    // array���ͣ�ָ��Ԫ�ص�ָ���Ԫ������
        struct { char* s; size_t len; }s;           // string���ͣ�ָ���ַ�����ָ����ַ�������
        double n;                                   // number���ͣ���double��ʾ
    }u;
    lept_type type;                 // JSONֵ������
};

enum {                             // ����һ��ö�٣����ڱ�ʾ����JSONʱ���������Ĵ���
    LEPT_PARSE_OK = 0,             // �����ɹ�
    LEPT_PARSE_EXPECT_VALUE,       // ����һ��ֵ��û�еõ�
    LEPT_PARSE_INVALID_VALUE,      // ��Ч��ֵ
    LEPT_PARSE_ROOT_NOT_SINGULAR,  // ���ڵ���滹�������ַ�
    LEPT_PARSE_NUMBER_TOO_BIG,     // ����̫��
    LEPT_PARSE_MISS_QUOTATION_MARK,  // ȱʧ����
    LEPT_PARSE_INVALID_STRING_ESCAPE,  // �ַ���������Ч��ת���ַ�
    LEPT_PARSE_INVALID_STRING_CHAR,  // �ַ���������Ч���ַ�
    LEPT_PARSE_INVALID_UNICODE_HEX,  // ��Ч��Unicodeʮ�������ַ�
    LEPT_PARSE_INVALID_UNICODE_SURROGATE,  // ��Ч��Unicode�����
    LEPT_PARSE_MISS_COMMA_OR_SQUARE_BRACKET  // ȱʧ���Ż�����
};

#define lept_init(v) do { (v)->type = LEPT_NULL; } while(0)  
// �꣬���ڳ�ʼ��lept_value���͵ı���������������ΪLEPT_NULL

int lept_parse(lept_value* v, const char* json); // �������������ڽ���JSON�ַ���

void lept_free(lept_value* v);  // ���������������ͷ�lept_value�ṹ��ռ�õ���Դ

lept_type lept_get_type(const lept_value* v);  // �������������ڻ�ȡJSONֵ������

#define lept_set_null(v) lept_free(v)  // �꣬��lept_value����ΪNULL����

int lept_get_boolean(const lept_value* v);  // �������������ڻ�ȡJSONֵ�Ĳ���ֵ
void lept_set_boolean(lept_value* v, int b);  // ������������������JSONֵ�Ĳ���ֵ

double lept_get_number(const lept_value* v);  // �������������ڻ�ȡJSONֵ������
void lept_set_number(lept_value* v, double n);  // ������������������JSONֵ������

const char* lept_get_string(const lept_value* v);  // �������������ڻ�ȡJSONֵ���ַ���
size_t lept_get_string_length(const lept_value* v);  // �������������ڻ�ȡJSON�ַ����ĳ���
void lept_set_string(lept_value* v, const char* s, size_t len);  // ������������������JSONֵ���ַ���


size_t lept_get_array_size(const lept_value* v);  // �������������ڻ�ȡJSON����Ĵ�С
lept_value* lept_get_array_element(const lept_value* v, size_t index);  // �������������ڻ�ȡJSON�����Ԫ��

#endif /* LEPTJSON_H__ */  // �����궨��
