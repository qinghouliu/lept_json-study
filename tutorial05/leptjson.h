#ifndef LEPTJSON_H__           // 如果没有定义宏LEPTJSON_H__
#define LEPTJSON_H__           // 则定义宏LEPTJSON_H__

#include <stddef.h>           // 包含头文件stddef.h，提供size_t等类型

typedef enum {                 // 定义一个枚举类型lept_type，用于表示JSON值的类型
    LEPT_NULL,                 // JSON的null类型
    LEPT_FALSE,                // JSON的false类型
    LEPT_TRUE,                 // JSON的true类型
    LEPT_NUMBER,               // JSON的number类型
    LEPT_STRING,               // JSON的string类型
    LEPT_ARRAY,                // JSON的array类型
    LEPT_OBJECT                // JSON的object类型
} lept_type;

typedef struct lept_value lept_value; // 前向声明一个结构体lept_value

struct lept_value {                 // 定义结构体lept_value，用于表示一个JSON值
    union {                         // 使用联合体来节省内存，只存储当前值的类型所需的数据
        struct { lept_value* e; size_t size; }a;    // array类型：指向元素的指针和元素数量
        struct { char* s; size_t len; }s;           // string类型：指向字符串的指针和字符串长度
        double n;                                   // number类型：用double表示
    }u;
    lept_type type;                 // JSON值的类型
};

enum {                             // 定义一个枚举，用于表示解析JSON时可能遇到的错误
    LEPT_PARSE_OK = 0,             // 解析成功
    LEPT_PARSE_EXPECT_VALUE,       // 期望一个值但没有得到
    LEPT_PARSE_INVALID_VALUE,      // 无效的值
    LEPT_PARSE_ROOT_NOT_SINGULAR,  // 根节点后面还有其他字符
    LEPT_PARSE_NUMBER_TOO_BIG,     // 数字太大
    LEPT_PARSE_MISS_QUOTATION_MARK,  // 缺失引号
    LEPT_PARSE_INVALID_STRING_ESCAPE,  // 字符串中有无效的转义字符
    LEPT_PARSE_INVALID_STRING_CHAR,  // 字符串中有无效的字符
    LEPT_PARSE_INVALID_UNICODE_HEX,  // 无效的Unicode十六进制字符
    LEPT_PARSE_INVALID_UNICODE_SURROGATE,  // 无效的Unicode代理对
    LEPT_PARSE_MISS_COMMA_OR_SQUARE_BRACKET  // 缺失逗号或方括号
};

#define lept_init(v) do { (v)->type = LEPT_NULL; } while(0)  
// 宏，用于初始化lept_value类型的变量，将类型设置为LEPT_NULL

int lept_parse(lept_value* v, const char* json); // 函数声明，用于解析JSON字符串

void lept_free(lept_value* v);  // 函数声明，用于释放lept_value结构体占用的资源

lept_type lept_get_type(const lept_value* v);  // 函数声明，用于获取JSON值的类型

#define lept_set_null(v) lept_free(v)  // 宏，将lept_value设置为NULL类型

int lept_get_boolean(const lept_value* v);  // 函数声明，用于获取JSON值的布尔值
void lept_set_boolean(lept_value* v, int b);  // 函数声明，用于设置JSON值的布尔值

double lept_get_number(const lept_value* v);  // 函数声明，用于获取JSON值的数字
void lept_set_number(lept_value* v, double n);  // 函数声明，用于设置JSON值的数字

const char* lept_get_string(const lept_value* v);  // 函数声明，用于获取JSON值的字符串
size_t lept_get_string_length(const lept_value* v);  // 函数声明，用于获取JSON字符串的长度
void lept_set_string(lept_value* v, const char* s, size_t len);  // 函数声明，用于设置JSON值的字符串


size_t lept_get_array_size(const lept_value* v);  // 函数声明，用于获取JSON数组的大小
lept_value* lept_get_array_element(const lept_value* v, size_t index);  // 函数声明，用于获取JSON数组的元素

#endif /* LEPTJSON_H__ */  // 结束宏定义
