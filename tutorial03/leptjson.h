#ifndef LEPTJSON_H__
#define LEPTJSON_H__
//包含了标准库头文件 <stddef.h>，以便使用 size_t 类型。
#include <stddef.h> /* size_t */
//新增加3个数据类型 LEPT_STRING, LEPT_ARRAY, LEPT_OBJECT，可以做更加抽象的JSON解析
typedef enum { LEPT_NULL, LEPT_FALSE, LEPT_TRUE, LEPT_NUMBER, LEPT_STRING, LEPT_ARRAY, LEPT_OBJECT } lept_type;
//lept_value 结构体用于表示 JSON 值，包括一个联合体 u 和一个表示值类型的枚举 type。
//联合体 u 可以存储两种类型的值：字符串（通过 s 成员表示，包括字符串指针 s 和字符串长度 len）和双精度浮点数（通过 n 成员表示）。
//type 是一个 lept_type 枚举类型的变量，用于标识 u 中存储的值类型。
typedef struct {
    union {
        struct { char* s; size_t len; }s;  /* string: null-terminated string, string length */
        double n;                          /* number */
    }u;
    lept_type type;
}lept_value;

//新增加的LEPT_PARSE_MISS_QUOTATION_MARK,
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
//lept_init(v): 定义了一个宏，用于初始化 lept_value 结构体。展开后的效果是将 v 的 type 成员设置为 LEPT_NULL
#define lept_init(v) do { (v)->type = LEPT_NULL; } while(0)
//解析输入的 JSON 字符串 json，并将解析结果存储在 lept_value 结构体 v 中。
int lept_parse(lept_value* v, const char* json);
//释放 lept_value 结构体 v 中的资源，包括字符串内存等
void lept_free(lept_value* v);
//获取 lept_value 结构体 v 中存储的值类型
lept_type lept_get_type(const lept_value* v);
//宏定义，用于将 lept_value 结构体 v 中的值设为 LEPT_NULL 并释放相关资源
#define lept_set_null(v) lept_free(v)
//获取 lept_value 结构体 v 中存储的布尔值
int lept_get_boolean(const lept_value* v);
//设置 lept_value 结构体 v 中的值为布尔类型。
void lept_set_boolean(lept_value* v, int b);
//获取 lept_value 结构体 v 中存储的数值。
double lept_get_number(const lept_value* v);
//设置 lept_value 结构体 v 中的值为数值类型。
void lept_set_number(lept_value* v, double n);
//获取 lept_value 结构体 v 中存储的字符串
const char* lept_get_string(const lept_value* v);
//获取 lept_value 结构体 v 中存储的字符串长度
size_t lept_get_string_length(const lept_value* v);
//设置 lept_value 结构体 v 中的值为字符串类型
void lept_set_string(lept_value* v, const char* s, size_t len);
#endif /* LEPTJSON_H__ */
