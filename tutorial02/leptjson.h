#ifndef LEPTJSON_H__
#define LEPTJSON_H__
//#ifndef ����Ƿ��Ѷ����� LEPTJSON_H__ �꣬���δ���壬�����ִ������Ĵ��룬
//�������������ļ���һ����˵������������ƻ��ͷ�ļ�������һ����ֻ�Ǽ�����һ�� _H__ ��׺����ȷ��Ψһ��
// 
//�ڽ̳�1�Ļ���֮�ϣ�������˼�������
typedef enum { LEPT_NULL, LEPT_FALSE, LEPT_TRUE, LEPT_NUMBER, LEPT_STRING, LEPT_ARRAY, LEPT_OBJECT } lept_type;

//���ڴ洢 JSON ֵ�ľ�����Ϣ������ֵ����ֵ��������������ͣ������͡�
typedef struct {
    double n;
    lept_type type;
}lept_value;

//���ڽ��� �����ӵĵ����ֹ���ʱ��ʧ�ܵ����
enum {
    LEPT_PARSE_OK = 0,
    LEPT_PARSE_EXPECT_VALUE,
    LEPT_PARSE_INVALID_VALUE,
    LEPT_PARSE_ROOT_NOT_SINGULAR,
    LEPT_PARSE_NUMBER_TOO_BIG//�����ӵģ�������ת����ת�����ˣ��������������
};
int lept_parse(lept_value* v, const char* json);//����
lept_type lept_get_type(const lept_value* v);//����
double lept_get_number(const lept_value* v);//����
//�����˻�ȡ JSON ����ֵ�ĺ��� lept_get_number�����ڷ��� lept_value �ṹ���д洢�� JSON ����ֵ��

#endif /* LEPTJSON_H__ */
