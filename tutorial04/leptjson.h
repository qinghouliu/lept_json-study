#ifndef LEPTJSON_H__
#define LEPTJSON_H__
//防止同一个文件或在同一翻译单元内包含的不同文件中多次包含同一个头文件。
// 如果LEPTJSON_H__未被定义，预处理器会定义它并包含文件内容。如果已定义，则跳过内容
#include <stddef.h> /* size_t */
//标准头文件<stddef.h>，它定义了多种类型和宏，特别是size_t，用于表示内存大小。
typedef enum { LEPT_NULL, LEPT_FALSE, LEPT_TRUE, LEPT_NUMBER, LEPT_STRING, LEPT_ARRAY, LEPT_OBJECT } lept_type;
//lept_type的枚举，列出了一个 JSON 实体可能具有的值类型，类似于 JSON 数据中的类型（null、boolean、number、string、array、object）
typedef struct {
    union {
        struct { char* s; size_t len; }s;  /* string: null-terminated string, string length */
        double n;                          /* number */
    }u;
    lept_type type;
}lept_value;
//lept_value来表示一个 JSON 值。使用一个union来存储字符串（由字符指针和长度size_t表示）或数字（double）。
//lept_type成员指定联合体当前存储的值的类型。
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
    LEPT_PARSE_INVALID_UNICODE_SURROGATE
};
//列出了 JSON 解析器可能返回的各种错误代码，解释了解析过程中发生的错误类型。
#define lept_init(v) do { (v)->type = LEPT_NULL; } while(0)
//用于初始化一个lept_value实例，将其类型设置为LEPT_NULL。它使用 do-while 循环使其成为一个复合语句，从语法上表现得像一个普通的函数调用

int lept_parse(lept_value* v, const char* json);
//接受一个指向lept_value的指针和一个 JSON 数据字符串。它解析 JSON 字符串并将结果存储在lept_value结构中。

void lept_free(lept_value* v);
//函数lept_free，用于释放和清理lept_value实例中的资源。

lept_type lept_get_type(const lept_value* v);//返回lept_value的类型。

#define lept_set_null(v) lept_free(v)//通过释放它来将lept_value设置为 null，假设lept_free处理设置类型为LEPT_NULL。

int lept_get_boolean(const lept_value* v);//获取和设置lept_value布尔值的函数
void lept_set_boolean(lept_value* v, int b);

//获取和设置lept_value数值的函数
double lept_get_number(const lept_value* v);
void lept_set_number(lept_value* v, double n);

//lept_value获取字符串值及其长度的函数，以及设置字符串值的函数。
const char* lept_get_string(const lept_value* v);
size_t lept_get_string_length(const lept_value* v);
void lept_set_string(lept_value* v, const char* s, size_t len);
#endif /* LEPTJSON_H__ */
