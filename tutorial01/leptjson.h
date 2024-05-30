#ifndef LEPTJSON_H__
#define LEPTJSON_H__
//#ifndef ����Ƿ��Ѷ����� LEPTJSON_H__ �꣬���δ���壬�����ִ������Ĵ��룬
//�������������ļ���һ����˵������������ƻ��ͷ�ļ�������һ����ֻ�Ǽ�����һ�� _H__ ��׺����ȷ��Ψһ��
typedef enum { LEPT_NULL, LEPT_FALSE, LEPT_TRUE, LEPT_NUMBER, LEPT_STRING, LEPT_ARRAY, LEPT_OBJECT } lept_type;
//ö�����͵����������ڶ��� JSON ֵ�����͡�
// �������� null��false��true��number��string��array �� object �⼸�����͡�
typedef struct {
    lept_type type;//ö����������
    //��ʾ JSON ֵ����������һ����Ϊ type �ĳ�Ա�����ڴ洢 JSON ֵ�����͡�
}lept_value;
enum {
    LEPT_PARSE_OK = 0,//�����ɹ�
    LEPT_PARSE_EXPECT_VALUE,//����ֵ
    //��һ�� JSON ֻ���пհף����� LEPT_PARSE_EXPECT_VALUE
    LEPT_PARSE_INVALID_VALUE,//��Чֵ
    //��һ��ֵ֮���ڿհ�֮���������ַ������� LEPT_PARSE_ROOT_NOT_SINGULAR
    LEPT_PARSE_ROOT_NOT_SINGULAR//���ڵ㲻Ψһ��
    //��ֵ��������������ֵ������ LEPT_PARSE_INVALID_VALUE
};

int lept_parse(lept_value* v, const char* json);
//���� JSON �ַ���������������洢�� lept_value �ṹ���С�
//������һ��ָ�� lept_value �ṹ���ָ�� v ��һ��ָ�� JSON �ַ�����ָ�� json
lept_type lept_get_type(const lept_value* v);
//���ڻ�ȡ���� JSON ֵ�����͡�������һ��ָ�� lept_value �ṹ��ĳ���ָ�� v�������ظ� JSON ֵ�����͡�

#endif /* LEPTJSON_H__ */
//��ͷ�ļ��Ľ�����־�����ڽ��� #ifndef �顣
// �����#endif �����ע�� LEPTJSON_H__ ��Ϊ��ȷ���ñ�־�� #ifndef �ı���������ƥ�䡣
