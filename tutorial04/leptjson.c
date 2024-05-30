#ifdef _WINDOWS
#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#endif
//������뻷��Ϊ Windows������ _CRTDBG_MAP_ALLOC�������� <crtdbg.h> ͷ�ļ��������ڴ�й©��⡣
#include "leptjson.h"//������ JSON �������ͷ�ļ�
#include <assert.h>  /* assert() */
#include <errno.h>   /* errno, ERANGE */
#include <math.h>    /* HUGE_VAL */
#include <stdlib.h>  /* NULL, malloc(), realloc(), free(), strtod() */
#include <string.h>  /* memcpy() */
#ifndef LEPT_PARSE_STACK_INIT_SIZE
#define LEPT_PARSE_STACK_INIT_SIZE 256
#endif
//�����ʼ��ջ��С�����δ���ȶ��壬��ջ��СĬ��Ϊ 256��

#define EXPECT(c, ch)       do { assert(*c->json == (ch)); c->json++; } while(0)//EXPECT: ȷ�ϵ�ǰ�ַ��Ƿ�ΪԤ���ַ������ǣ�����ǰ�ƶ���
#define ISDIGIT(ch)         ((ch) >= '0' && (ch) <= '9')//�ж��ַ��Ƿ�Ϊ���ֻ�1��9֮������֡�
#define ISDIGIT1TO9(ch)     ((ch) >= '1' && (ch) <= '9')
#define PUTC(c, ch)         do { *(char*)lept_context_push(c, sizeof(char)) = (ch); } while(0)
//ȷ�����ʹ����������һ������ʹ�ڲ���Ҫ�ֺŵ���������Ҳ�ܰ�ȫʹ�á�
//��������ȷ�����㹻�Ŀռ��ڶ�ջ�ϴ洢�ַ���������һ��ָ���¿ռ��ָ�롣
//���ַ� ch �洢��ͨ�� lept_context_push ��ȡ���ڴ��ַ�ϡ�
//
typedef struct {
    const char* json;
    char* stack;
    size_t size, top;
}lept_context;
//һ�����������Ľṹ�壬���� JSON �ַ�������̬��ջ����ջ��С�͵�ǰ��ջ������������
static void* lept_context_push(lept_context* c, size_t size) {//�ڽ�����ջ�з���ָ����С�Ŀռ䡣����ռ䲻�㣬�����·������Ķ�ջ��
    void* ret;
    assert(size > 0);
    if (c->top + size >= c->size) {
        if (c->size == 0)
            c->size = LEPT_PARSE_STACK_INIT_SIZE;
        while (c->top + size >= c->size)
            c->size += c->size >> 1;  /* c->size * 1.5 */
        c->stack = (char*)realloc(c->stack, c->size);
    }
    ret = c->stack + c->top;
    //��ַ����һ��������ƫ������ͨ������ָ�����㡣��������Ľ����һ���µĵ�ַ������ԭ��ַƫ��һ������֮���λ�á�
    c->top += size;
    return ret;
}

static void* lept_context_pop(lept_context* c, size_t size) {//�Ӷ�ջ�е���ָ����С�����ݡ�
    assert(c->top >= size);
    return c->stack + (c->top -= size);
}
static void lept_parse_whitespace(lept_context* c) {//���� JSON �ַ����еĿհ��ַ���
    const char *p = c->json;
    while (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r')
        p++;
    c->json = p;
}
static int lept_parse_literal(lept_context* c, lept_value* v, const char* literal, lept_type type) {//���� JSON �е����������� true��false��null����
    size_t i;
    EXPECT(c, literal[0]);
    for (i = 0; literal[i + 1]; i++)
        if (c->json[i] != literal[i + 1])
            return LEPT_PARSE_INVALID_VALUE;
    c->json += i;
    v->type = type;
    return LEPT_PARSE_OK;
}

static int lept_parse_number(lept_context* c, lept_value* v) {//���� JSON �е����֣���������ܵ����ַ�Χ����
    const char* p = c->json;
    if (*p == '-') p++;
    if (*p == '0') p++;
    else {
        if (!ISDIGIT1TO9(*p)) return LEPT_PARSE_INVALID_VALUE;
        for (p++; ISDIGIT(*p); p++);
    }
    if (*p == '.') {
        p++;
        if (!ISDIGIT(*p)) return LEPT_PARSE_INVALID_VALUE;
        for (p++; ISDIGIT(*p); p++);
    }
    if (*p == 'e' || *p == 'E') {
        p++;
        if (*p == '+' || *p == '-') p++;
        if (!ISDIGIT(*p)) return LEPT_PARSE_INVALID_VALUE;
        for (p++; ISDIGIT(*p); p++);
    }
    errno = 0;
    v->u.n = strtod(c->json, NULL);
    if (errno == ERANGE && (v->u.n == HUGE_VAL || v->u.n == -HUGE_VAL))//���ֹ���ʱ�ᱨ��
        return LEPT_PARSE_NUMBER_TOO_BIG;
    v->type = LEPT_NUMBER;
    c->json = p;
    return LEPT_PARSE_OK;
}

static const char* lept_parse_hex4(const char* p, unsigned* u) {
    //����һ����̬���� lept_parse_hex4������һ��ָ���ַ���ָ�� p ��һ��ָ���޷���������ָ�� u��
    //lept_parse_hex4 �������� 4 λʮ����������������洢�� u ָ��ı����У������ش�������ַ���ָ��
    //static �ؼ��ֱ�ʾ�ú���ֻ�ڵ�ǰԴ�ļ��ڿɼ���
    //const char* ��ʾ��������һ��ָ�����ַ���ָ��
    //ͨ��ÿһλ����λ�Ͱ�λ����������ǿ��Խ�ÿ��ʮ�������ַ����ۼӵ����յĽ���С�
    // �������ڴ����� \u3A7F ֮��*u ��ֵ���� 0x3A7F����ʾ��� Unicode ת�����ж�Ӧ���ַ���
    int i;
    *u = 0;//�� u ָ��ı�����ʼ��Ϊ 0��׼���ۼӽ�������ʮ������ֵ��
    for (i = 0; i < 4; i++) {
        char ch = *p++;
        *u <<= 4;//u ָ��ı������� 4 λ��Ϊ��һ���ۼ��ڳ�λ�á�
        if (ch >= '0' && ch <= '9')  *u |= ch - '0';//��� ch �������ַ���'0' �� '9'��������ת��Ϊ��Ӧ������ֵ���ۼӵ� u��
        else if (ch >= 'A' && ch <= 'F')  *u |= ch - ('A' - 10);//: ʹ�ð�λ���������ֵ��ӵ� u ��
        else if (ch >= 'a' && ch <= 'f')  *u |= ch - ('a' - 10);
        else return NULL;
    }
    return p;// ���ش�������ַ���ָ�룬�� p ����ָ����ǽ�����Ϻ��λ�á�
}
static void lept_encode_utf8(lept_context* c, unsigned u) {
   //lept_encode_utf8 ��һ����̬����������һ��ָ����������� lept_context ��ָ�� c ��һ���޷������� u��
   // ����Ҫ����� Unicode ���
    /* \TODO */
    if (u <= 0x7F)
        PUTC(c, u & 0xFF);//u & 0xFF ȷ��ֻ������ 8 λ����Ȼ���ﲢ����Ҫ����Ϊ u �Ѿ�С�ڵ��� 0x7F����
    //ʹ�� PUTC �꽫����ֽڴ洢������������ c �Ķ�ջ��
    else if (u <= 0x7FF) {
        PUTC(c, 0xC0 | ((u >> 6) & 0xFF));//��һ���ֽڸ�ʽ��110xxxxx��ͨ�� 0xC0 | ((u >> 6) & 0xFF) ���ɡ�
        PUTC(c, 0x80 | (u & 0x3F));//0x80 ����ǰ��������Ϊ 10
    }
    else if (u <= 0xFFFF) {//u С�ڵ��� 0xFFFF����ʾ�������������ֽڱ��롣
        PUTC(c, 0xE0 | ((u >> 12) & 0xFF));//u >> 12 ��������� 12 λ���õ����λ����,0xE0 ����ǰ�ĸ�����Ϊ 1110��
        PUTC(c, 0x80 | ((u >> 6) & 0x3F));//0x80 ����ǰ��������Ϊ 10,u >> 6 ��������� 6 λ���õ��м䲿��
        PUTC(c, 0x80 | (u & 0x3F));//u & 0x3F ��ȡ�� 6 λ,0x80 ����ǰ��������Ϊ 10.
        //ʹ�� PUTC �꽫�������ֽڴ洢������������ c �Ķ�ջ�С�
    }
    else {
        assert(u <= 0x10FFFF);//u С�ڵ��� 0x10FFFF����ʾ���������ĸ��ֽڱ��롣ʹ�� assert ȷ�� u ����Ч�� Unicode ��Χ��
        PUTC(c, 0xF0 | ((u >> 18) & 0xFF));//u >> 18 ��������� 18 λ���õ����λ���֡�
        PUTC(c, 0x80 | ((u >> 12) & 0x3F));
        //ͨ�� 0x80 | ((u >> 12) & 0x3F) ���ɡ�
        //u >> 12 ��������� 12 λ���õ��θ�λ���֡�
        //0x80 ����ǰ��������Ϊ 10��
        PUTC(c, 0x80 | ((u >> 6) & 0x3F));
        //ͨ�� 0x80 | ((u >> 6) & 0x3F) ���ɡ�
         //u >> 6 ��������� 6 λ���õ��м䲿�֡�
         //0x80 ����ǰ��������Ϊ 10��
        PUTC(c, 0x80 | (u & 0x3F));//u & 0x3F ��ȡ�� 6 λ��
        //0x80 ����ǰ��������Ϊ 10��
    }
}

#define STRING_ERROR(ret) do { c->top = head; return ret; } while(0)

static int lept_parse_string(lept_context* c, lept_value* v) {
    size_t head = c->top, len;
    unsigned u, u2;//���ڴ��� Unicode ת���ַ�
    const char* p;//p ��ָ��ǰ����λ�õ�ָ��
    EXPECT(c, '\"');//ȷ���ַ�����˫���ſ�ʼ��
    p = c->json;// ��ָ������Ϊ��ǰ���������ĵ�λ��
    for (;;) {//�ַ�������ѭ��
        char ch = *p++;
        switch (ch) {
        case '\"':
            len = c->top - head;
            lept_set_string(v, (const char*)lept_context_pop(c, len), len);
            c->json = p;
            return LEPT_PARSE_OK;
        case '\\':
            switch (*p++) {
            case '\"': PUTC(c, '\"'); break;
            case '\\': PUTC(c, '\\'); break;
            case '/':  PUTC(c, '/'); break;
            case 'b':  PUTC(c, '\b'); break;
            case 'f':  PUTC(c, '\f'); break;
            case 'n':  PUTC(c, '\n'); break;
            case 'r':  PUTC(c, '\r'); break;
            case 't':  PUTC(c, '\t'); break;
            case 'u'://��������
                if (!(p = lept_parse_hex4(p, &u)))
                    STRING_ERROR(LEPT_PARSE_INVALID_UNICODE_HEX);
                if (u >= 0xD800 && u <= 0xDBFF) { /* surrogate pair */
                    //��� u ���ڸߴ���Է�Χ (0xD800 �� 0xDBFF)���������Ҫ�������ԡ�
                    if (*p++ != '\\')
                        STRING_ERROR(LEPT_PARSE_INVALID_UNICODE_SURROGATE);
                    if (*p++ != 'u')
                        STRING_ERROR(LEPT_PARSE_INVALID_UNICODE_SURROGATE);
                    //�����������ַ��Ƿ��� \\u��������ǣ��򴥷� STRING_ERROR �꣬���� LEPT_PARSE_INVALID_UNICODE_SURROGATE ����
                    if (!(p = lept_parse_hex4(p, &u2)))
                        STRING_ERROR(LEPT_PARSE_INVALID_UNICODE_HEX);
                    //�ٴε��� lept_parse_hex4 �����ʹ���Ե��ĸ�ʮ�������ַ���
                    // ����洢�� u2 �С��������ʧ�ܣ����� NULL ������ STRING_ERROR �꣬
                    // ���� LEPT_PARSE_INVALID_UNICODE_HEX ����
                    if (u2 < 0xDC00 || u2 > 0xDFFF)
                        //u2 �Ƿ��ڵʹ���Է�Χ(0xDC00 �� 0xDFFF)��������ǣ��򴥷� STRING_ERROR �꣬���� LEPT_PARSE_INVALID_UNICODE_SURROGATE ����
                        STRING_ERROR(LEPT_PARSE_INVALID_UNICODE_SURROGATE);
                    u = (((u - 0xD800) << 10) | (u2 - 0xDC00)) + 0x10000;//��ϸߵʹ��������һ�������� Unicode ��㣬���洢�� u �С�
                }
                lept_encode_utf8(c, u);
                //���� lept_encode_utf8 �����յ� Unicode ��� u ����Ϊ UTF-8 ���洢�ڽ��������ĵĶ�ջ�С�
                break;
            default:
                STRING_ERROR(LEPT_PARSE_INVALID_STRING_ESCAPE);
            }
            break;
        case '\0':
            STRING_ERROR(LEPT_PARSE_MISS_QUOTATION_MARK);
        default:
            if ((unsigned char)ch < 0x20)
                STRING_ERROR(LEPT_PARSE_INVALID_STRING_CHAR);
            PUTC(c, ch);
        }
    }
}

static int lept_parse_value(lept_context* c, lept_value* v) {
    //�˺������ݵ�ǰ�ַ�����ʹ���ĸ�����Ľ������������� JSON ���ݣ����������ֵ�����֡��ַ����� null��
    switch (*c->json) {
        case 't':  return lept_parse_literal(c, v, "true", LEPT_TRUE);
        case 'f':  return lept_parse_literal(c, v, "false", LEPT_FALSE);
        case 'n':  return lept_parse_literal(c, v, "null", LEPT_NULL);
        default:   return lept_parse_number(c, v);
        case '"':  return lept_parse_string(c, v);
        case '\0': return LEPT_PARSE_EXPECT_VALUE;
    }
}

int lept_parse(lept_value* v, const char* json) {
    // JSON ������������������ʼ�����������ģ����� JSON �ַ������������ǵ�һֵ����������磬��һ�� JSON �ĵ��н������ֵ)
    lept_context c;
    int ret;
    assert(v != NULL);
    c.json = json;
    c.stack = NULL;
    c.size = c.top = 0;
    lept_init(v);
    lept_parse_whitespace(&c);
    if ((ret = lept_parse_value(&c, v)) == LEPT_PARSE_OK) {
        lept_parse_whitespace(&c);
        if (*c.json != '\0') {
            v->type = LEPT_NULL;
            ret = LEPT_PARSE_ROOT_NOT_SINGULAR;
        }
    }
    assert(c.top == 0);
    free(c.stack);
    return ret;
}

void lept_free(lept_value* v) {//������������ͷ� lept_value �з������Դ�����ַ�����
    assert(v != NULL);
    if (v->type == LEPT_STRING)
        free(v->u.s.s);
    v->type = LEPT_NULL;
}

lept_type lept_get_type(const lept_value* v) {// lept_value �д洢��ֵ�����͡�
    assert(v != NULL);
    return v->type;
}

int lept_get_boolean(const lept_value* v) {//�������������ڻ�ȡ�����ò���ֵ��
    assert(v != NULL && (v->type == LEPT_TRUE || v->type == LEPT_FALSE));
    return v->type == LEPT_TRUE;
}
void lept_set_boolean(lept_value* v, int b) {
    lept_free(v);
    v->type = b ? LEPT_TRUE : LEPT_FALSE;
}
//���ڻ�ȡ������ JSON �������͵�ֵ��
double lept_get_number(const lept_value* v) {
    assert(v != NULL && v->type == LEPT_NUMBER);
    return v->u.n;
}
void lept_set_number(lept_value* v, double n) {
    lept_free(v);
    v->u.n = n;
    v->type = LEPT_NUMBER;
}
//��Щ�������ڻ�ȡ������ JSON �ַ�����ֵ��������ȡ�ַ������Ⱥ����ݣ��Լ������µ��ַ���ֵ
const char* lept_get_string(const lept_value* v) {
    assert(v != NULL && v->type == LEPT_STRING);
    return v->u.s.s;
}

size_t lept_get_string_length(const lept_value* v) {
    assert(v != NULL && v->type == LEPT_STRING);
    return v->u.s.len;
}

void lept_set_string(lept_value* v, const char* s, size_t len) {
    assert(v != NULL && (s != NULL || len == 0));
    lept_free(v);
    v->u.s.s = (char*)malloc(len + 1);
    memcpy(v->u.s.s, s, len);
    v->u.s.s[len] = '\0';
    v->u.s.len = len;
    v->type = LEPT_STRING;
}
