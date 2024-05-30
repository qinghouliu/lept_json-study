//leptjson.h���Զ���� JSON ������ͷ�ļ���
//<assert.h>�����ڶ�����ԣ�assert���꣬���ڵ���ʱ��������顣
//<errno.h>�������˴����� errno �ʹ����� ERANGE��
//<math.h>���ṩ����ѧ���㺯������ HUGE_VAL��
//<stdlib.h>���������ڴ���亯�� malloc(), realloc(), free()���Լ��ַ���ת������ strtod()��
//<string.h>���ṩ���ַ��������������� memcpy()��
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
//���������ʹ�õĶ�ջ��ʼ��С��Ĭ��Ϊ 256
#ifndef LEPT_PARSE_STACK_INIT_SIZE
#define LEPT_PARSE_STACK_INIT_SIZE 256
#endif

#define EXPECT(c, ch)       do { assert(*c->json == (ch)); c->json++; } while(0)
#define ISDIGIT(ch)         ((ch) >= '0' && (ch) <= '9')
#define ISDIGIT1TO9(ch)     ((ch) >= '1' && (ch) <= '9')
//�����ӵ�
#define PUTC(c, ch)         do { *(char*)lept_context_push(c, sizeof(char)) = (ch); } while(0)
//*(char*)lept_context_push(c, sizeof(char)) = (ch);
//lept_context_push(c, sizeof(char))������ lept_context_push �������ڶ�ջ�з����㹻�Ŀռ��Դ洢һ���ַ�������������ռ��ָ�롣
//(char*)�������ص�ָ��ת��Ϊ char* ���͡�
//* (char*)... = (ch); ��ͨ�������ã����ַ� ch �洢���·���Ķ�ջ�ռ�λ���С�
//*(char*)�������ص�ָ��ת��Ϊ char ����ָ�룬���������Է��ʻ�洢�ַ���
//(ch)���� ch ֵ����Ԥ���Ŀռ��С�



//��lept_contex������չ��������ջ
typedef struct {
    const char* json;
    char* stack;
    size_t size, top;
}lept_context;


//lept_context_push �� lept_context_pop����������ջ��ѹ�롣
//static�����Ƹú����������򣬽��ڵ�ǰ�ļ��ɼ���
//void* ������ֵ���ͣ�ָ���ڶ�ջ�з���Ŀռ䡣
//lept_context* c��ָ���ջ����ṹ lept_context ��ָ�롣
//size_t size��ָ��Ҫ������ֽ���


//����һ������ void* ָ��ľ�̬���� lept_context_push�������� lept_context ��ջ�ṹ�з���ռ䡣
//��δ���ͨ�� lept_context_push ������̬������ջ������ȷ������ʼ��Ϊ�µ����ݷ����㹻�Ŀռ䣬��ͨ�� PUTC �꽫�ַ�ֱ�Ӵ洢����ջ�С�
static void* lept_context_push(lept_context* c, size_t size) {
    void* ret;//����һ��ͨ��ָ�� ret�����ڷ����·���Ķ�ջ�ռ��ַ��
    assert(size > 0);//ȷ������� size ����0�����򴥷����Զ��Դ���
    if (c->top + size >= c->size) {//��鵱ǰջ��λ�����������Ĵ�С����һ���Ƿ񳬳���ǰ��ջ������
        if (c->size == 0)//�����ջ��ǰ����Ϊ�㣬��ʼ��ΪĬ�ϵĶ�ջ����
            c->size = LEPT_PARSE_STACK_INIT_SIZE;
        while (c->top + size >= c->size)//���ϵ�����ջ����ֱ�����㵱ǰ��������Ŀռ䡣
            c->size += c->size >> 1;  /* c->size * 1.5 */
        c->stack = (char*)realloc(c->stack, c->size);
        //ʹ�� realloc ���·����ڴ棬���µ�ָ�븳ֵ�� c->stack�������ջ��û�з������realloc ���Զ����䡣
    }
    ret = c->stack + c->top;//��ȡ�µ����ݿ����ʼ��ַ�����洢�� ret ������
    c->top += size;//���� c->top ָ��λ�ã�ָ���µ�ջ��
    return ret;//���ط���ռ����ʼ��ַ
}


//��������
static void* lept_context_pop(lept_context* c, size_t size) {
    assert(c->top >= size);//c->top >= size ȷ����ǰջ��λ�� c->top ������ size ���ֽڣ��Է�ֹջ�������
    return c->stack + (c->top -= size);//����ջ��ָ�� c->top �м�ȥ size������ջ��λ����ʵ�ֵ���Ч����
    //c->stack + ...�����㵯����ջ���ĵ�ַ��
}
//�����հ��ַ�
static void lept_parse_whitespace(lept_context* c) {
    const char *p = c->json;
    while (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r')
        p++;
    c->json = p;
}
//����true ��false��NULL�Ĳ���
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
//�����ֽ��в���
static int lept_parse_number(lept_context* c, lept_value* v) {
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
//���� JSON �ַ������͡�
//�����������ַ����洢�� lept_value �ṹ�С�
//head����¼��ǰ������ջ����ʼλ�á�
//len�����ڴ洢���������ַ����ĳ��ȡ�
//EXPECT(c, '\"')����鵱ǰλ���Ƿ�Ϊ˫���ſ�ͷ��
//for (;;) { ... }������ѭ�������ڱ����ַ����е�ÿ���ַ���
//char ch = *p++����ȡ��ǰ�ַ�����ָ�� p ��ǰ�ƶ���
//switch (ch) { ... }�����ݵ�ǰ�ַ����з�֧�жϡ�
//case '\"':������˫���ű�ʾ�ַ��������������ַ������Ȳ����ַ����洢�� lept_value �ṹ�У�Ȼ����½�����λ�ò����ؽ����ɹ���
//case '\0':�������ַ�����β��δ�ҵ�˫���ţ��ָ�������ջ������ȱ��˫���ŵĴ���
//default:��������ͨ�ַ������ַ�ѹ�������ջ�С�
static int lept_parse_string(lept_context* c, lept_value* v) {
    size_t head = c->top, len;
    const char* p;
    EXPECT(c, '\"');//�꣨������ȷ����һ���ַ���˫����"����ʾ�ַ����Ŀ�ͷ����֤���ƶ�ָ��λ�á�
    p = c->json;//ָ��p��ʼ��ΪJSON�ַ����еĵ�ǰλ��
    for (;;) {//����ѭ��������������ֱ���ҵ��պ����Ż�������
        char ch = *p++;//��ȡ��ǰ�ַ�����ָ��p�ƶ�����һ��λ��
        switch (ch) {
        case '\"'://ʶ���ַ����Ľ�β��
            len = c->top - head;//��ȡ�ַ��ĳ���
            lept_set_string(v, (const char*)lept_context_pop(c, len), len);
            //ԭ���ַ�����c�У�����lept_set_string��c�еķ��õ�v��
            c->json = p;
            return LEPT_PARSE_OK;
        case '\\':
            switch (*p++) {
            case '\"': PUTC(c, '\"'); break;//����ת���˫����
            case '\\': PUTC(c, '\\'); break;//����ת��ķ�б��
            case '/':  PUTC(c, '/'); break;//����ת�����б��
            case 'b':  PUTC(c, '\b'); break;//�����˸����\b��
            case 'f':  PUTC(c, '\f'); break;//����ҳ����\f��
            case 'n':  PUTC(c, '\n'); break;//�����з���\n��
            case 'r':  PUTC(c, '\r'); break;//����س�����\r��
            case 't':  PUTC(c, '\t'); break;//�����Ʊ����\t��
            default://���ת��������Ч������ջ���ã�c->top = head����������LEPT_PARSE_INVALID_STRING_ESCAPE��
                c->top = head;
                return LEPT_PARSE_INVALID_STRING_ESCAPE;
            }
            break;
        case '\0'://�������ַ���ζ�������ѵ���ĩβ��û���ҵ��պ����š�
            c->top = head;
            return LEPT_PARSE_MISS_QUOTATION_MARK;
        default://�����ַ��Ƿ�Ϊ�����ַ���ASCII��С��0x20��
            //����ǣ�����ջ���ã�c->top = head����������LEPT_PARSE_INVALID_STRING_CHAR��
            //����ʹ��PUTC�����ַ��洢��������ջ��
            if ((unsigned char)ch < 0x20) {
             //�����ַ���ָ ASCII ����ֵС�� 32��0x20�����ַ���ͨ���ǲ��ɴ�ӡ�ַ��������س������еȡ������� JSON �ַ����б���Ϊ����Ч�ġ�
                c->top = head;//�ڶ�ջ�У����� c->top ָ�룬����ָ�� head λ�á�
                //head �ǵ�ǰ������ջ��ʼ�����ַ���ʱ�Ķ�ջλ�á����������Ч�ַ�����Ҫ���õ�������ʼʱ��״̬��
                return LEPT_PARSE_INVALID_STRING_CHAR;//��ʾ������������������Ч���ַ����ַ��������ַ�����
            }
            PUTC(c, ch);//һ���꣨���������ڽ��ַ�ch������c����Ľ�����ջ
        }
    }
}

static int lept_parse_value(lept_context* c, lept_value* v) {
    switch (*c->json) {
        case 't':  return lept_parse_literal(c, v, "true", LEPT_TRUE);
        case 'f':  return lept_parse_literal(c, v, "false", LEPT_FALSE);
        case 'n':  return lept_parse_literal(c, v, "null", LEPT_NULL);
        case '"':  return lept_parse_string(c, v);
        case '\0': return LEPT_PARSE_EXPECT_VALUE;
        default:   return lept_parse_number(c, v);
    }
}
//����Ҫ�ĺ��� lept_parse
int lept_parse(lept_value* v, const char* json) {
    lept_context c;
    int ret;
    assert(v != NULL);
    //��ʼ����lept_context�е���������
    c.json = json;//�����ｫ�ַ�����ֵ�ŵ�C��
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
    assert(c.top == 0);//�ڽ�������ʱ��ջ��û��ʣ���Ԫ�أ��� c.top Ӧ��Ϊ 0��
    //�����ջ�л���Ԫ�أ���ζ�Ž��������г����˴��󣬶�ջû�б���ȷ����
    // ������Ե��������ڵ��Խ׶β������ֲ�����Ԥ�ڵ�����������Ų����
    free(c.stack);
   // �ͷŽ��������з���Ķ�ջ�ռ䣬��ֹ�ڴ�й©������������������ͨ����̬�����ڴ���ά����ջ�ṹ�������ڽ�����������Ҫ�ֶ��ͷŷ�����ڴ档
    return ret;
}
//�ͷ� lept_value �ṹ�е���Դ��
void lept_free(lept_value* v) {
    assert(v != NULL);
    if (v->type == LEPT_STRING)
        free(v->u.s.s);
    v->type = LEPT_NULL;
}
//��ȡ lept_value �ṹ�е�����
lept_type lept_get_type(const lept_value* v) {
    assert(v != NULL);
    return v->type;
}

//����һ����������Ϊ int �ĺ��� lept_get_boolean��������һ��ָ�� lept_value ���͵�ָ�� v��
int lept_get_boolean(const lept_value* v) {
    /* \TODO */
    assert(v != NULL && (v->type == LEPT_TRUE || v->type == LEPT_FALSE));
    //ͨ������ȷ�� v ��Ϊ�գ����� v �������ǲ���ֵ LEPT_TRUE �� LEPT_FALSE��
    return v->type == LEPT_TRUE;
    return 0;
}

void lept_set_boolean(lept_value* v, int b) {//����һ���޷���ֵ�ĺ��� lept_set_boolean��������ָ�� lept_value ���͵�ָ�� v ��һ������ֵ b
    /* \TODO */
    lept_free(v);
    v->type = b ? LEPT_TRUE : LEPT_FALSE;//b Ϊ���㣬���ʽ���Ϊ LEPT_TRUE
    //b Ϊ 0�����ʽ���Ϊ LEPT_FALSE
}

double lept_get_number(const lept_value* v) {//����һ����������Ϊ double �ĺ��� lept_get_number��������һ��ָ�� lept_value ���͵�ָ�� v��
    assert(v != NULL && v->type == LEPT_NUMBER);
    return v->u.n;
}

void lept_set_number(lept_value* v, double n) {
    //����һ���޷���ֵ�ĺ��� lept_set_number��������ָ�� lept_value ���͵�ָ�� v ��һ�� double ���͵���ֵ n
    /* \TODO */
    lept_free(v);
    v->u.n = n;
    v->type = LEPT_NUMBER;
}

const char* lept_get_string(const lept_value* v) {
    //����һ����������Ϊָ�� char ���͵ĳ���ָ��ĺ��� lept_get_string��������һ��ָ�� lept_value ���͵�ָ�� v��
    assert(v != NULL && v->type == LEPT_STRING);
    return v->u.s.s;
}

size_t lept_get_string_length(const lept_value* v) {
    //����һ����������Ϊ size_t �ĺ��� lept_get_string_length��������һ��ָ�� lept_value ���͵�ָ�� v��
    assert(v != NULL && v->type == LEPT_STRING);
    return v->u.s.len;
}

void lept_set_string(lept_value* v, const char* s, size_t len) {
    //����һ���޷���ֵ�ĺ��� lept_set_string��������ָ�� lept_value ���͵�ָ�� v���Լ�һ��ָ�� char ���͵ĳ���ָ�� s ��һ���ַ������� len��
    assert(v != NULL && (s != NULL || len == 0));
    //ͨ������ȷ�� v ��Ϊ�գ����� s Ϊ�ǿգ����� len Ϊ 0��
    lept_free(v);//���� lept_free �����ͷ� v ��ռ�õ���Դ��
    v->u.s.s = (char*)malloc(len + 1);//Ϊ v ���ַ����ֶ� u.s.s �����ڴ棬����Ϊ len + 1 �ֽڡ�
    memcpy(v->u.s.s, s, len);//�������ַ��� s �����ݸ��Ƶ� v ���ַ����ֶ� u.s.s �У����Ƶĳ���Ϊ len��
    v->u.s.s[len] = '\0';//�� u.s.s �����һλ����ַ�����ֹ�� \0
    v->u.s.len = len;//���� v ���ַ��������ֶ� u.s.len Ϊ len
    v->type = LEPT_STRING;//�� v ����������Ϊ�ַ��� LEPT_STRING��
}
