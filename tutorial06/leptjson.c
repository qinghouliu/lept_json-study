#ifdef _WINDOWS
#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#endif
#include "leptjson.h"
#include <assert.h>  /* assert() */
#include <errno.h>   /* errno, ERANGE */
#include <math.h>    /* HUGE_VAL */
#include <stdlib.h>  /* NULL, malloc(), realloc(), free(), strtod() */
#include <string.h>  /* memcpy() */

#ifndef LEPT_PARSE_STACK_INIT_SIZE
#define LEPT_PARSE_STACK_INIT_SIZE 256
#endif

#define EXPECT(c, ch)       do { assert(*c->json == (ch)); c->json++; } while(0)
#define ISDIGIT(ch)         ((ch) >= '0' && (ch) <= '9')
#define ISDIGIT1TO9(ch)     ((ch) >= '1' && (ch) <= '9')
#define PUTC(c, ch)         do { *(char*)lept_context_push(c, sizeof(char)) = (ch); } while(0)
//���ַ� ch ������������� c �Ķ�ջ�С�
typedef struct {
    const char* json;
    char* stack;
    size_t size, top;
}lept_context;
//���� lept_context
static void* lept_context_push(lept_context* c, size_t size) {
    //���ǽ�Ԫ������ջ�У���̬��ջ������
    //void*��һ��ָ�����ͣ�������ָ���������͵����ݣ������������͡���������ȡ�void*�ķ������ͱ�ʾ�ú������ص���һ��ָ�룬����δָ������ָ����������͡�
    //�ں���lept_context_push�У���������Ϊvoid* ����ʾ�ú�������һ��ָ�룬ָ��ĳ���ڴ�λ�á�����ָ�����ʲô���͵������ɺ����ĵ����߾�������Ϊ�ں����ڲ���û��ָ�����������
    //size_t ��һ���� C �� C++ ����������������ڱ�ʾ��С�ͼ������޷����������͡�����һ�����Ͷ��壨typedef)
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
    //c->stack ��ָ���ջ�ײ���ָ�롣
    //c->top ��ʾ��ǰ��ջ������ƫ���������ֽ�Ϊ��λ����
    //ͨ�� c->stack + c->top���˱��ʽ�������ǰ��ջ�����ĵ�ַ�����������ַ���� ret��������ret ��ָ���˿��Է��������ݵĶ�ջλ��
    c->top += size;
    //c->top ��ֵ�����ӵ����Ǽ����洢�ڶ�ջ�е����ݵĴ�С��size����
    return ret;
}

static void* lept_context_pop(lept_context* c, size_t size) {
    assert(c->top >= size);
    return c->stack + (c->top -= size);
    //c->top -= size�����ȣ��ӵ�ǰջ��ָ�루c->top���м�ȥ��ջ���ݵĴ�С��size��������ջ��ָ�룬ָ���µ�ջ��λ�á��ⲽ����ʵ�����ǽ�ջ�������ƶ��� size �ֽڡ�
    //c->stack + (c->top -= size)��Ȼ��ͨ�� c->stack����ջ�ײ���ָ�룩�����µ� c->top ֵ����������º��ջ�����ڴ��ַ�������ַ�ǳ�ջ���ݵ���ʼλ�á�
}

static void lept_parse_whitespace(lept_context* c) {//����һЩ��ʵ������ķ��ţ�ָ��ָ����Ч�ַ�λ
    const char* p = c->json;
    while (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r')
        p++;
    c->json = p;
}

static int lept_parse_literal(lept_context* c, lept_value* v, const char* literal, lept_type type) {
    // ��֮ǰ�����ƣ����������ع��������ж�NULL��FALSE TRUE�����
    size_t i;
    EXPECT(c, literal[0]);
    for (i = 0; literal[i + 1]; i++)
        if (c->json[i] != literal[i + 1])
            return LEPT_PARSE_INVALID_VALUE;
    c->json += i;
    v->type = type;
    return LEPT_PARSE_OK;
}

static int lept_parse_number(lept_context* c, lept_value* v) {//�������ֵĽ���
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
    if (errno == ERANGE && (v->u.n == HUGE_VAL || v->u.n == -HUGE_VAL))
        return LEPT_PARSE_NUMBER_TOO_BIG;
    v->type = LEPT_NUMBER;
    c->json = p;
    return LEPT_PARSE_OK;
}
//���������Ŀ���Ǵ�һ���ַ����н����ĸ�ʮ�������ַ���������ת����һ���޷���������
static const char* lept_parse_hex4(const char* p, unsigned* u) {//�����ﴫ�������ã�ʵ������ɶ�u�ĸ�ֵ
    int i;
    *u = 0;
    for (i = 0; i < 4; i++) {
        char ch = *p++;
        *u <<= 4;
        if (ch >= '0' && ch <= '9')  *u |= ch - '0';
        else if (ch >= 'A' && ch <= 'F')  *u |= ch - ('A' - 10);
        else if (ch >= 'a' && ch <= 'f')  *u |= ch - ('a' - 10);
        else return NULL;
    }
    return p;
}
//��������� Unicode ��� u ת���� UTF-8 ������ʽ������ÿ���ֽ���������������� c �еĶ�ջ��
static void lept_encode_utf8(lept_context* c, unsigned u) {
    if (u <= 0x7F)
        PUTC(c, u & 0xFF);
    //��� u С�ڵ��� 0x7F���������� UTF-8 ����һ���ֽڱ�ʾ������ͨ���� 0xFF ����λ�����
  //��ʵ��������û�б仯����Ϊ u �Ѿ�С�� 0xFF����Ȼ�������ջ��
    else if (u <= 0x7FF) {
        PUTC(c, 0xC0 | ((u >> 6) & 0xFF));
        PUTC(c, 0x80 | (u & 0x3F));
    }
 //��� u С�ڵ��� 0x7FF������Ҫ�����ֽ�����ʾ��
 //��һ���ֽ��� 110 ��ͷ����� u �ĸ�5λ���ڶ����ֽ��� 10 ��ͷ����� u �ĵ�6λ��
    else if (u <= 0xFFFF) {
        PUTC(c, 0xE0 | ((u >> 12) & 0xFF));
        PUTC(c, 0x80 | ((u >> 6) & 0x3F));
        PUTC(c, 0x80 | (u & 0x3F));
    }
    else {
    //�������ֵΪ 0xFFFF �� u����Ҫ�����ֽڡ����뽫��һ���ֽڵ�ǰ׺��Ϊ 1110������������ÿ��6λ��ɵ��ֽڣ�
    //ÿ���ֽ�ǰ׺Ϊ 10��
        assert(u <= 0x10FFFF);
        PUTC(c, 0xF0 | ((u >> 18) & 0xFF));
        PUTC(c, 0x80 | ((u >> 12) & 0x3F));
        PUTC(c, 0x80 | ((u >> 6) & 0x3F));
        PUTC(c, 0x80 | (u & 0x3F));
    }
}

#define STRING_ERROR(ret) do { c->top = head; return ret; } while(0)
//�˺궨�����ڴ����ַ��������г��ֵĴ��󡣵����ô˺�ʱ������ջ��ָ�� (c->top) ����Ϊ head�����ַ���������ʼʱ��λ�ã���
//Ȼ�󷵻ش������ ret����������Ϊ�˶����ڽ��������п����Ѿ����뵽��ջ�Ĳ������ݣ���������һ���ԡ�

static int lept_parse_string_raw(lept_context* c, char** str, size_t* len) {//����������Դ� JSON �ı��н���һ���ַ�����
    //lept_context* c ��ʾ c ��һ��ָ�� lept_context �ṹ���ָ�롣lept_context ������һ���ṹ�壬�����˴��� JSON ��������Ҫ��������������Ϣ���統ǰ����λ�á���ջָ�롢��ջ��С�ȡ�
    //char** str ��ʾ str ��һ��ָ���ַ�ָ���ָ�롣��������ͨ�����ں����ڲ���Ҫ�޸�ָ�뱾���ֵ��
    //size_t* len ��ʾ len ��һ��ָ�� size_t ���͵�ָ�롣size_t ͨ�����ڱ�ʾ���ݵĴ�С�򳤶ȡ�
    //����������У�len ���ڷ��ؽ��������ַ����ĳ��ȡ�ͨ���޸�* len���� len ָ��Ĵ�С���������ַ������ȣ���������Ǽ���õ��ģ����ַ�������λ�ü�ȥ��ʼλ��
    size_t head = c->top;//��¼������ʼʱ��ջ��λ�ã��Ա��ڷ�������ʱ���Իع�
    unsigned u, u2;
    const char* p;
    EXPECT(c, '\"');
    p = c->json;
    for (;;) {
        char ch = *p++;
        switch (ch) {
        case '\"':
            *len = c->top - head;
            *str = lept_context_pop(c, *len);
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
            case 'u':
                if (!(p = lept_parse_hex4(p, &u)))
                    STRING_ERROR(LEPT_PARSE_INVALID_UNICODE_HEX);
                if (u >= 0xD800 && u <= 0xDBFF) { /* surrogate pair */
                    if (*p++ != '\\')
                        STRING_ERROR(LEPT_PARSE_INVALID_UNICODE_SURROGATE);
                    if (*p++ != 'u')
                        STRING_ERROR(LEPT_PARSE_INVALID_UNICODE_SURROGATE);
                    if (!(p = lept_parse_hex4(p, &u2)))
                        STRING_ERROR(LEPT_PARSE_INVALID_UNICODE_HEX);
                    if (u2 < 0xDC00 || u2 > 0xDFFF)
                        STRING_ERROR(LEPT_PARSE_INVALID_UNICODE_SURROGATE);
                    u = (((u - 0xD800) << 10) | (u2 - 0xDC00)) + 0x10000;
                }
                lept_encode_utf8(c, u);
                //���� lept_encode_utf8 �����յ� Unicode ��� u ����Ϊ UTF-8 ���洢�ڽ��������ĵĶ�ջ�С�
                break;
            default:
                STRING_ERROR(LEPT_PARSE_INVALID_STRING_ESCAPE);//˵��ת���ַ�������ŵ���ĸ����
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

static int lept_parse_string(lept_context* c, lept_value* v) {//��������ع�����֮ǰ�Ĳ�һ��
    int ret;
    char* s;
    size_t len;
    if ((ret = lept_parse_string_raw(c, &s, &len)) == LEPT_PARSE_OK)
        lept_set_string(v, s, len);
    return ret;
}

static int lept_parse_value(lept_context* c, lept_value* v);

static int lept_parse_array(lept_context* c, lept_value* v) {
    size_t i, size = 0;
    //i ����ѭ��������size ��ʼ��Ϊ0��������¼������Ԫ�ص�������
    int ret;
    //ret �����洢��������ֵ��
    //����������������i ����ѭ��������size ���ڼ�¼������Ԫ�ص�������ret ���ڴ洢��������ֵ��
    EXPECT(c, '[');//ʹ�� EXPECT ����ȷ����ǰ�������ַ������鿪ʼ�� [ ���š�������ǣ���ᴥ������
    lept_parse_whitespace(c);//lept_parse_whitespace �������������ܴ��ڵĿհ׷����Ա���ȷ���� JSON ���ݡ�
    if (*c->json == ']') {
        c->json++;
        v->type = LEPT_ARRAY;
        v->u.a.size = 0;//size������¼������Ԫ�ص�������
        v->u.a.e = NULL;
        return LEPT_PARSE_OK;
        //�����⵽�����飬�� JSON ָ����ǰ�ƶ������� ]������ v ������Ϊ LEPT_ARRAY����С����Ϊ0��Ԫ��ָ������ΪNULL�������ؽ����ɹ���״̬��
    }
    //����������հ׷���ֱ����������������� ] ���ţ����ʾһ�������顣��ʱ������ JSON ָ������ ]��
    //���� v ������Ϊ���鵫û��Ԫ�أ�size = 0 �� e = NULL���������ؽ����ɹ���״̬��
    for (;;) {
        //��ʼһ������ѭ��������������������е�Ԫ�ء�Ϊÿ��Ԫ�ش���һ�� lept_value �ṹ�� e����ʹ�� lept_init ��ʼ����
        lept_value e;
        lept_init(&e);
        if ((ret = lept_parse_value(c, &e)) != LEPT_PARSE_OK)//���ڽ��������еĲ�ͬ���͵�����
            //���Խ���һ�� JSON ֵ���洢�� e �С��������ʧ�ܣ����˳�ѭ����
            break;
        memcpy(lept_context_push(c, sizeof(lept_value)), &e, sizeof(lept_value));
        //lept_context_push(c, sizeof(lept_value))���ú������ڽ�һ��ָ����С���ڴ�������ջ�У�
        // ������ָ����ڴ���ָ�롣��������ڶ�ջ��Ϊ�µ�Ԫ�ط����㹻�Ŀռ䣬������ָ��ÿռ��ָ�롣
        //memcpy(destination, &e, sizeof(lept_value))������һ���ڴ濽�����������ڽ�e�����ݸ��Ƶ�ָ����Ŀ���ڴ�λ�á�
        //destination��lept_context_push���ص�ָ�룬��ָ���ջ�п��е��ڴ�顣&e��ʾe�����ĵ�ַ��sizeof(lept_value)��ʾҪ���Ƶ��ֽ�����
        //��lept_value���͵Ĵ�С��������memcpy������e�����ݸ��Ƶ���ջ���ʵ�λ�á�
        size++;
        lept_parse_whitespace(c);//�ٴ������κοհ׷�����׼��������һ��Ԫ�ػ������������־��
        if (*c->json == ',') {//�������Ԫ�طָ��� ,�����ƶ� JSON ָ��������������������Ŀհ׷���
            c->json++;
            lept_parse_whitespace(c);
        }
        else if (*c->json == ']') {
            c->json++;
            v->type = LEPT_ARRAY;
            v->u.a.size = size;
            size *= sizeof(lept_value);
            memcpy(v->u.a.e = (lept_value*)malloc(size), lept_context_pop(c, size), size);
            //(lept_value*)malloc(size)��ʹ��malloc������̬�����ڴ�ռ䣬��СΪsize�ֽڡ�
            // �����size��֮ǰѭ���м���õ���������Ԫ�ص���������sizeof(lept_value)����Ҫ������ڴ�ռ���ܴ�С��
            // ǿ������ת��(lept_value*)�ǽ����ص�ָ��ת��Ϊlept_value*���ͣ��Ա���v->u.a.e��������ƥ�䡣
            //lept_context_pop(c, size)���ú����Ӷ�ջ�е���ָ����С��Ԫ�ء�����������ڴӶ�ջ�е���size���ֽڵ��ڴ�飬��Щ�ڴ�����֮ǰ����������Ԫ�ء�

            //memcpy(v->u.a.e, lept_context_pop(c, size), size)������һ���ڴ濽�����������ڽ���ջ�е�Ԫ�ظ��Ƶ�֮ǰ������ڴ�ռ��С�
            // v->u.a.e��ʾ����Ԫ�ص�ָ�룬��ָ��֮ǰʹ��malloc������ڴ�ռ䡣lept_context_pop(c, size)���ڻ�ȡ��ջ�е�Ԫ�أ�size��ʾҪ���Ƶ��ֽ�����
            //������memcpy��������ջ�е�Ԫ�ظ��Ƶ��·�����ڴ�ռ��С�

            return LEPT_PARSE_OK;

        }
        //���������������� ]������ JSON ָ�룬���� v ������Ϊ���飬��ʹ�� malloc ����ǡ�õ��ڴ�ռ�洢����Ԫ�ء�
        //�Ӷ�ջ�е�����ЩԪ�ز����Ƶ��·�����ڴ��У���󷵻ؽ����ɹ�״̬
        else {
            ret = LEPT_PARSE_MISS_COMMA_OR_SQUARE_BRACKET;
            break;
        }
        //����Ȳ��� , Ҳ���� ]����˵�� JSON ��ʽ�������ô������Ͳ��ж�ѭ����
    }
    /* Pop and free values on the stack */
    for (i = 0; i < size; i++)
        lept_free((lept_value*)lept_context_pop(c, sizeof(lept_value)));
    return ret;
    //�����Ϊ������˳�ѭ����������ջ�ϵ�����Ԫ�أ���ÿ��Ԫ�ص��� lept_free ���ͷ�ռ�õ���Դ��
    //���Ӷ�ջ�е�������󷵻ؽ��������ͨ����һ��������룩����һ����ȷ�����κη������Դ�ڴ�����ʱ������ȷ�ͷţ������ڴ�й©��

}

static int lept_parse_object(lept_context* c, lept_value* v) {//ept_parse_object ������������������������ c ��һ�����洢����� JSON ֵ v��
    size_t i, size;
    lept_member m;//lept_member m; ����һ�������Ա��������������ֵ��
    int ret;//int ret; �����洢�������ص�״̬��
    EXPECT(c, '{');
    lept_parse_whitespace(c);
    if (*c->json == '}') {//������� }����ʾ����Ϊ��
        c->json++;
        v->type = LEPT_OBJECT;//���½���λ�ã����� JSON ֵ������Ϊ���󣬳�ʼ����Աָ��ʹ�СΪ 0��
        v->u.o.m = 0;//ָ�븳ֵΪ0��Ҳ����NULL
        v->u.o.size = 0;
        return LEPT_PARSE_OK;//���ؽ����ɹ�״̬��
    }
    m.k = NULL;
    //��ʼ����ָ�� m.k Ϊ NULL���Ա���������ڴ档
    //ʹ������ѭ����ʼ���������еļ�ֵ�ԡ�
    size = 0;
    for (;;){
        char* str;
        lept_init(&m.v);
        /* parse key */
        if (*c->json != '"') {//ȷ���������ſ�ͷ��������ǣ������÷���״̬Ϊ��ȱʧ��
            ret = LEPT_PARSE_MISS_KEY;
            break;
        }
        if ((ret = lept_parse_string_raw(c, &str, &m.klen)) != LEPT_PARSE_OK)//���� lept_parse_string_raw �������ַ�����
            break;
        memcpy(m.k = (char*)malloc(m.klen + 1), str, m.klen); //Ϊ�������ڴ沢���ƽ����õ����ַ��������Ͻ����� \0
        m.k[m.klen] = '\0';
        /* parse ws colon ws */
        lept_parse_whitespace(c);
        if (*c->json != ':') {//�����κοհף���鲢����ð��,���ð��ȱʧ�������÷���״̬
            ret = LEPT_PARSE_MISS_COLON;
            break;
        }
        c->json++;
        lept_parse_whitespace(c);
        /* parse value */
        if ((ret = lept_parse_value(c, &m.v)) != LEPT_PARSE_OK)//���� ����Ӧ��ֵ�����ʧ�ܣ��ж�ѭ����
            break;
        memcpy(lept_context_push(c, sizeof(lept_member)), &m, sizeof(lept_member));//����ֵ�����������ĵĶ�ջ��
        size++;
        m.k = NULL; /* ownership is transferred to member on stack */ 
        /* parse ws [comma | right-curly-brace] ws */
        lept_parse_whitespace(c);//������ֵ��֮��Ŀհ��ַ�
        if (*c->json == ',') {//����������ţ����ʾ���滹��������Ա�����Ķ��Ų�����������
            c->json++;
            lept_parse_whitespace(c);
        }
        else if (*c->json == '}') {
            size_t s = sizeof(lept_member) * size;
            //����������������־ }����������г�Ա�Ľ����������ܴ�С���Ӷ�ջ�е������г�Ա�������ڴ棬
            //���� size ��ʾ��ǰ�洢��Ҫ�洢�ļ�ֵ�ԣ�lept_member �ṹ�壩��������
            //ͨ���� sizeof(lept_member) ���� size�������ڼ���洢 size �� lept_member ʵ����������ڴ�����
            c->json++;
            v->type = LEPT_OBJECT;
            v->u.o.size = size;
            memcpy(v->u.o.m = (lept_member*)malloc(s), lept_context_pop(c, s), s);
            //��ʹ�� malloc �����ƺ��������洢 lept_member ʵ������֮ǰ���С����ṩ�Ĵ����У�һ��֪��������ܴ�С��s����
            //�ͷ�����Ӧ���ڴ沢������ lept_member �ṹ����ʱ�洢��ͨ���ǽ��������ĵ�һ���ֵ�ջ�����Ƶ�����·�����ڴ��С�
            //��һ�����ǽ��� JSON ���󲢴洢�����г�Ա��һ���֣�
            return LEPT_PARSE_OK;
        }
        else {
            ret = LEPT_PARSE_MISS_COMMA_OR_CURLY_BRACKET;
            break;
        }
    }
    /* Pop and free members on the stack */
    
    //��ѭ��������ͨ������Ϊ�����˴��󣩣��ͷ��ѷ���ļ��ڴ棨������ڣ���
    //������ջ���������г�Ա�����ͷ�ÿ����Ա�ļ���ֵ��ռ�õ���Դ��
    //�� v ����������Ϊ LEPT_NULL����ʾ����ʧ�ܣ����ش�����롣
    free(m.k);
    for (i = 0; i < size; i++) {
        lept_member* m = (lept_member*)lept_context_pop(c, sizeof(lept_member));
        free(m->k);
        lept_free(&m->v);
    }
    v->type = LEPT_NULL;
    return ret;
}

static int lept_parse_value(lept_context* c, lept_value* v) {
    switch (*c->json) {
    case 't':  return lept_parse_literal(c, v, "true", LEPT_TRUE);
    case 'f':  return lept_parse_literal(c, v, "false", LEPT_FALSE);
    case 'n':  return lept_parse_literal(c, v, "null", LEPT_NULL);
    default:   return lept_parse_number(c, v);
    case '"':  return lept_parse_string(c, v);
    case '[':  return lept_parse_array(c, v);
    case '{':  return lept_parse_object(c, v);
    case '\0': return LEPT_PARSE_EXPECT_VALUE;
    }
}

int lept_parse(lept_value* v, const char* json) {
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

void lept_free(lept_value* v) {
    size_t i;
    assert(v != NULL);
    switch (v->type) {
    case LEPT_STRING:
        free(v->u.s.s);
        break;
    case LEPT_ARRAY:
        for (i = 0; i < v->u.a.size; i++)
            lept_free(&v->u.a.e[i]);
        free(v->u.a.e);
        break;
    case LEPT_OBJECT:
        for (i = 0; i < v->u.o.size; i++) {
            free(v->u.o.m[i].k);
            lept_free(&v->u.o.m[i].v);
        }
        free(v->u.o.m);
        break;
    default: break;
    }
    v->type = LEPT_NULL;
}

lept_type lept_get_type(const lept_value* v) {
    assert(v != NULL);
    return v->type;
}

int lept_get_boolean(const lept_value* v) {
    assert(v != NULL && (v->type == LEPT_TRUE || v->type == LEPT_FALSE));
    return v->type == LEPT_TRUE;
}

void lept_set_boolean(lept_value* v, int b) {
    lept_free(v);
    v->type = b ? LEPT_TRUE : LEPT_FALSE;
}

double lept_get_number(const lept_value* v) {
    assert(v != NULL && v->type == LEPT_NUMBER);
    return v->u.n;
}

void lept_set_number(lept_value* v, double n) {
    lept_free(v);
    v->u.n = n;
    v->type = LEPT_NUMBER;
}

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

size_t lept_get_array_size(const lept_value* v) {
    assert(v != NULL && v->type == LEPT_ARRAY);
    return v->u.a.size;
}

lept_value* lept_get_array_element(const lept_value* v, size_t index) {
    assert(v != NULL && v->type == LEPT_ARRAY);
    assert(index < v->u.a.size);
    return &v->u.a.e[index];
}

size_t lept_get_object_size(const lept_value* v) {
    assert(v != NULL && v->type == LEPT_OBJECT);
    return v->u.o.size;
}

const char* lept_get_object_key(const lept_value* v, size_t index) {
    assert(v != NULL && v->type == LEPT_OBJECT);
    assert(index < v->u.o.size);
    return v->u.o.m[index].k;
}

size_t lept_get_object_key_length(const lept_value* v, size_t index) {
    assert(v != NULL && v->type == LEPT_OBJECT);
    assert(index < v->u.o.size);
    return v->u.o.m[index].klen;
}

lept_value* lept_get_object_value(const lept_value* v, size_t index) {
    assert(v != NULL && v->type == LEPT_OBJECT);
    assert(index < v->u.o.size);
    return &v->u.o.m[index].v;
}
