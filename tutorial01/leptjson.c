#include "leptjson.h"
#include <assert.h>  /* assert() *///���Կ�
#include <stdlib.h>  /* NULL *///��׼��
#define EXPECT(c, ch)       do { assert(*c->json == (ch)); c->json++; } while(0)
//���ڶ��Ե�ǰ JSON �ַ����������ַ�ƥ�䣬�����ַ�ָ����ǰ�ƶ�һλ
typedef struct {
    const char* json;
}lept_context;
//lept_context �ṹ�壬���ڴ洢 JSON �ַ�����ָ�롣
static void lept_parse_whitespace(lept_context* c) {
    const char *p = c->json;
    while (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r')
        p++;
    c->json = p;
}
//������������ JSON �еĿհ��ַ����ո��Ʊ�������з����س�����
//////////////////////////////////////////////////////////////////////////

static int lept_parse_true(lept_context* c, lept_value* v) {
    EXPECT(c, 't');//����ͨ���� EXPECT ȷ����ǰ�ַ��� 't
    if (c->json[0] != 'r' || c->json[1] != 'u' || c->json[2] != 'e')
        return LEPT_PARSE_INVALID_VALUE;
    //LEPT_PARSE_INVALID_VALUE ��ֵ�� 2������ζ����������Ĳ��� "true"����ô���������� 2����ʾ������ֵ����Ч�ġ�
    c->json += 3;
    v->type = LEPT_TRUE;//˵�������ж����ַ�true
    return LEPT_PARSE_OK;//�����ɹ�
}

static int lept_parse_false(lept_context* c, lept_value* v) {
    EXPECT(c, 'f');
    if (c->json[0] != 'a' || c->json[1] != 'l' || c->json[2] != 's' || c->json[3] != 'e')
        return LEPT_PARSE_INVALID_VALUE;
    //ͬ����������
    c->json += 4;
    v->type = LEPT_FALSE;//˵�������ж����ַ�false
    return LEPT_PARSE_OK;//�����ɹ�
}
//////////////////////////////////////////////////////////////////////////
static int lept_parse_null(lept_context* c, lept_value* v) {
    EXPECT(c, 'n');
    if (c->json[0] != 'u' || c->json[1] != 'l' || c->json[2] != 'l')
        return LEPT_PARSE_INVALID_VALUE;
    c->json += 3;
    v->type = LEPT_NULL;
    return LEPT_PARSE_OK;
}

static int lept_parse_value(lept_context* c, lept_value* v) {
    switch (*c->json) {
     //////////////////////////////////////////////////////////////////////
    case 't':  return lept_parse_true(c, v);
    case 'f':  return lept_parse_false(c, v);
    case 'n':  return lept_parse_null(c, v);
    case '\0': return LEPT_PARSE_EXPECT_VALUE;
    //��ǰ�ַ����ַ����Ľ�β�������ַ� '\0'�������ʾ������ JSON ֵΪ�գ������ַ����������� LEPT_PARSE_EXPECT_VALUE����ʾ����ֵ��
    default:   return LEPT_PARSE_INVALID_VALUE;//��Чֵ
    ////////////////////////////////////////////////////////////////////////
    }
}
int lept_parse(lept_value* v, const char* json) {
    ///////////////////////////////////////////////////
    lept_context c;
    int ret;
    assert(v != NULL);
    c.json = json;//JSON �ַ��� json �ĵ�ַ��ֵ�� c �ṹ��ʵ���� json ��Ա
    v->type = LEPT_NULL;
    //lept_value �ṹ��ָ�� v �� type ��Ա��ʼ��Ϊ LEPT_NULL������һ����ʼ״̬����ʾ JSON ֵ��������δȷ����
    lept_parse_whitespace(&c);
    //���� lept_parse_whitespace �������������� JSON �ַ����еĿհ��ַ���ȷ��������������Ч���ݿ�ʼ��
    if ((ret = lept_parse_value(&c, v)) == LEPT_PARSE_OK) {
        lept_parse_whitespace(&c);
        //�ٴε��� lept_parse_whitespace ������������������ܴ��ڵĶ���հ��ַ���
        if (*c.json != '\0')
            ret = LEPT_PARSE_ROOT_NOT_SINGULAR;
        //����ڶ�����ַ��������ǰ�ַ������ַ����Ľ�β�������ǿ��ַ� \0����
        //���ʾ JSON �ַ����д��ڶ����δ�������ݣ����ǲ��Ϸ��ģ���˽������������Ϊ 
    }
    return ret;
    /////////////////////////////////////////////////
}

lept_type lept_get_type(const lept_value* v) {
    assert(v != NULL);
    return v->type;
}
//��ȡ���� JSON ֵ�����͡�������ʹ�� assert(v != NULL) ����ȷ������ v ��Ϊ�գ�Ȼ�󷵻� v �� type ��Ա������ʾ JSON ֵ�����͡�
