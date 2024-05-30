#include "leptjson.h"
#include <assert.h>  /* assert() */
#include <errno.h>   /* errno, ERANGE *///errno.h ������һ����Ϊ errno ���ⲿȫ�ֱ��������ڱ��溯�����ò����Ĵ������
#include <math.h>    /* HUGE_VAL */
#include <stdlib.h>  /* NULL, strtod() */
//�� EXPECT(c, ch)�����ڼ�鵱ǰ����λ�� c->json ���ַ��Ƿ�Ϊ ch����������򴥷�����ʧ�ܣ��ƶ�����λ�á�
//������ ISDIGIT(ch) �� ISDIGIT1TO9(ch)�������ж��ַ� ch �Ƿ�Ϊ�����ַ������� '1' �� '9' ֮��������ַ���
#define EXPECT(c, ch)       do { assert(*c->json == (ch)); c->json++; } while(0)
#define ISDIGIT(ch)         ((ch) >= '0' && (ch) <= '9')
#define ISDIGIT1TO9(ch)     ((ch) >= '1' && (ch) <= '9')
//�ṹ�� lept_context�����ڴ洢���������е���������Ϣ�����а���һ��ָ��ǰ����λ�õ�ָ�� json��
typedef struct {
    const char* json;
}lept_context;
//��̬���� lept_parse_whitespace�����������հ��ַ��������ո��Ʊ�������з��ͻس�����
static void lept_parse_whitespace(lept_context* c) {
    const char* p = c->json;
    while (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r')
        p++;
    c->json = p;
}
//һ����̬���� lept_parse_literal�����ڽ��� JSON ����ֵ���� "true"��"false"��"null"
//����������У�ʵ���˶Խ̳�1���ع���Ҳ���ǽ�3��������������
static int lept_parse_literal(lept_context* c, lept_value* v, const char* literal, lept_type type) {
    size_t i;
    EXPECT(c, literal[0]);
    for (i = 0; literal[i + 1]; i++)
        if (c->json[i] != literal[i + 1])
            return LEPT_PARSE_INVALID_VALUE;
    c->json += i;
    v->type = type;
    return LEPT_PARSE_OK;
}
//������һ����̬���� lept_parse_number�����ڽ��� JSON ���֡�
//lept_parse_number �������ڽ��� JSON �е����֣�������������洢�� lept_value �ṹ�� v
static int lept_parse_number(lept_context* c, lept_value* v) {
    const char* p = c->json;
    if (*p == '-') p++;//���ж��ǲ��Ǹ���
    if (*p == '0') p++;//ǰ��0ȥ��
    else {
        if (!ISDIGIT1TO9(*p)) return LEPT_PARSE_INVALID_VALUE;       //�ж��ǲ������ֿ�ͷ
        for (p++; ISDIGIT(*p); p++);//�����������ֵı���
    }
    if (*p == '.') {//������С������
        p++;
        if (!ISDIGIT(*p)) return LEPT_PARSE_INVALID_VALUE;//С�������׸����ֿ�Ϊ0,0-9
        for (p++; ISDIGIT(*p); p++);// 0 - 9,ֱ��������С������
    }
    if (*p == 'e' || *p == 'E') {//����ָ��
        p++;
        if (*p == '+' || *p == '-') p++;//ָ������������ķ���
        if (!ISDIGIT(*p)) return LEPT_PARSE_INVALID_VALUE;
        for (p++; ISDIGIT(*p); p++);//ָ������Ϊ0,�жϵķ�ΧΪ0-9
    }

 //ʹ�� strtod ������������ɵ������ַ���ת��Ϊ˫���ȸ���������������洢�� v->n �С�����֮ǰ�Ὣ errno ����Ϊ 0��
 // ��� strtod ����ִ�г����������ֹ�������������� errno �ᱻ����Ϊ ERANGE��
 // ��� errno ���� ERANGE ���ҽ������Ϊ HUGE_VAL �� -HUGE_VAL���򷵻ؽ������� LEPT_PARSE_NUMBER_TOO_BIG��
    errno = 0;
    v->n = strtod(c->json, NULL);
    if (errno == ERANGE && (v->n == HUGE_VAL || v->n == -HUGE_VAL))
        return LEPT_PARSE_NUMBER_TOO_BIG;
    v->type = LEPT_NUMBER;
    c->json = p;//��ʾ�������������
    return LEPT_PARSE_OK;//�ɹ�����
}
//��̬���� lept_parse_value�����ڽ��� JSON ֵ���������ַ����������ĸ���������
static int lept_parse_value(lept_context* c, lept_value* v) {
    switch (*c->json) {
    case 't':  return lept_parse_literal(c, v, "true", LEPT_TRUE);
    case 'f':  return lept_parse_literal(c, v, "false", LEPT_FALSE);
    case 'n':  return lept_parse_literal(c, v, "null", LEPT_NULL);
    default:   return lept_parse_number(c, v);
    case '\0': return LEPT_PARSE_EXPECT_VALUE;
    }
}
//JSON �������� lept_parse������һ�� JSON �ַ��� json������������洢�� lept_value �ṹ�� v �С�
int lept_parse(lept_value* v, const char* json) {
    lept_context c;
    int ret;
    assert(v != NULL);
    c.json = json;
    v->type = LEPT_NULL;
    lept_parse_whitespace(&c);
    if ((ret = lept_parse_value(&c, v)) == LEPT_PARSE_OK) {
        lept_parse_whitespace(&c);
        if (*c.json != '\0') {
            v->type = LEPT_NULL;
            ret = LEPT_PARSE_ROOT_NOT_SINGULAR;
        }
    }
    return ret;
}
//�����˻�ȡ JSON ֵ���͵ĺ��� lept_get_type�����ڷ��� lept_value �ṹ���д洢�� JSON ֵ�����͡�
lept_type lept_get_type(const lept_value* v) {
    assert(v != NULL);
    return v->type;
}

double lept_get_number(const lept_value* v) {
    assert(v != NULL && v->type == LEPT_NUMBER);
    return v->n;
}