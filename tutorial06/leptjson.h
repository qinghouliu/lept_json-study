#ifndef LEPTJSON_H__
#define LEPTJSON_H__
#include <stddef.h> /* size_t */
typedef enum { LEPT_NULL, LEPT_FALSE, LEPT_TRUE, LEPT_NUMBER, LEPT_STRING, LEPT_ARRAY, LEPT_OBJECT } lept_type;
//定义了 JSON 支持的不同数据类型

typedef struct lept_value lept_value;//自定义
typedef struct lept_member lept_member;

struct lept_value {//进行定义
    union {
        struct { lept_member* m; size_t size; }o;   /* object: members, member count */        //o：存储对象类型，包含成员指针和成员数。
        struct { lept_value* e; size_t size; }a;    /* array:  elements, element count */      //a：存储数组类型，包含元素指针和元素数。
        struct { char* s; size_t len; }s; /* string: null-terminated string, string length */ //s：存储字符串类型，包含指向字符串的指针和字符串长度。
        double n;                                   /* number */
    }u;
    lept_type type;//lept_type 枚举值
};

struct lept_member {//新的数据结构
    char* k; size_t klen;   /* member key string, key string length */    //k 和 klen：存储对象成员的键（字符串）及其长度
    lept_value v;           /* member value */  //v：存储对象成员的值    //
};

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
    LEPT_PARSE_INVALID_UNICODE_SURROGATE,
    LEPT_PARSE_MISS_COMMA_OR_SQUARE_BRACKET,
    LEPT_PARSE_MISS_KEY,  //新增加的错误的情况                                   
    LEPT_PARSE_MISS_COLON,//新增加的错误的情况     
    LEPT_PARSE_MISS_COMMA_OR_CURLY_BRACKET//新增加的错误的情况     
};

#define lept_init(v) do { (v)->type = LEPT_NULL; } while(0)

int lept_parse(lept_value* v, const char* json);

void lept_free(lept_value* v);

lept_type lept_get_type(const lept_value* v);

#define lept_set_null(v) lept_free(v)

int lept_get_boolean(const lept_value* v);
void lept_set_boolean(lept_value* v, int b);

double lept_get_number(const lept_value* v);
void lept_set_number(lept_value* v, double n);

const char* lept_get_string(const lept_value* v);
size_t lept_get_string_length(const lept_value* v);
void lept_set_string(lept_value* v, const char* s, size_t len);

size_t lept_get_array_size(const lept_value* v);
lept_value* lept_get_array_element(const lept_value* v, size_t index);

//新增加的对json对象的操作
size_t lept_get_object_size(const lept_value* v);
const char* lept_get_object_key(const lept_value* v, size_t index);
size_t lept_get_object_key_length(const lept_value* v, size_t index);
lept_value* lept_get_object_value(const lept_value* v, size_t index);
#endif /* LEPTJSON_H__ */
