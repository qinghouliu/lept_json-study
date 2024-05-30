#ifndef LEPTJSON_H__
#define LEPTJSON_H__
//#ifndef 检查是否已定义了 LEPTJSON_H__ 宏，如果未定义，则继续执行下面的代码，
//否则跳过整个文件。一般来说，保护宏的名称会和头文件的名称一样，只是加上了一个 _H__ 后缀，以确保唯一性
// 
//在教程1的基础之上，新添加了几个类型
typedef enum { LEPT_NULL, LEPT_FALSE, LEPT_TRUE, LEPT_NUMBER, LEPT_STRING, LEPT_ARRAY, LEPT_OBJECT } lept_type;

//用于存储 JSON 值的具体信息，包括值的数值（如果是数字类型）和类型。
typedef struct {
    double n;
    lept_type type;
}lept_value;

//对于解析 新增加的当数字过大时会失败的情况
enum {
    LEPT_PARSE_OK = 0,
    LEPT_PARSE_EXPECT_VALUE,
    LEPT_PARSE_INVALID_VALUE,
    LEPT_PARSE_ROOT_NOT_SINGULAR,
    LEPT_PARSE_NUMBER_TOO_BIG//新增加的，当数字转换，转换不了，会出现溢出的情况
};
int lept_parse(lept_value* v, const char* json);//解析
lept_type lept_get_type(const lept_value* v);//类型
double lept_get_number(const lept_value* v);//数字
//声明了获取 JSON 数字值的函数 lept_get_number，用于返回 lept_value 结构体中存储的 JSON 数字值。

#endif /* LEPTJSON_H__ */
