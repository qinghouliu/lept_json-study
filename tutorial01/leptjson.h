#ifndef LEPTJSON_H__
#define LEPTJSON_H__
//#ifndef 检查是否已定义了 LEPTJSON_H__ 宏，如果未定义，则继续执行下面的代码，
//否则跳过整个文件。一般来说，保护宏的名称会和头文件的名称一样，只是加上了一个 _H__ 后缀，以确保唯一性
typedef enum { LEPT_NULL, LEPT_FALSE, LEPT_TRUE, LEPT_NUMBER, LEPT_STRING, LEPT_ARRAY, LEPT_OBJECT } lept_type;
//枚举类型的声明，用于定义 JSON 值的类型。
// 它包括了 null、false、true、number、string、array 和 object 这几种类型。
typedef struct {
    lept_type type;//枚举数据类型
    //表示 JSON 值。它包含了一个名为 type 的成员，用于存储 JSON 值的类型。
}lept_value;
enum {
    LEPT_PARSE_OK = 0,//解析成功
    LEPT_PARSE_EXPECT_VALUE,//期望值
    //若一个 JSON 只含有空白，传回 LEPT_PARSE_EXPECT_VALUE
    LEPT_PARSE_INVALID_VALUE,//无效值
    //若一个值之后，在空白之后还有其他字符，传回 LEPT_PARSE_ROOT_NOT_SINGULAR
    LEPT_PARSE_ROOT_NOT_SINGULAR//根节点不唯一等
    //若值不是那三种字面值，传回 LEPT_PARSE_INVALID_VALUE
};

int lept_parse(lept_value* v, const char* json);
//解析 JSON 字符串并将解析结果存储在 lept_value 结构体中。
//它接受一个指向 lept_value 结构体的指针 v 和一个指向 JSON 字符串的指针 json
lept_type lept_get_type(const lept_value* v);
//用于获取给定 JSON 值的类型。它接受一个指向 lept_value 结构体的常量指针 v，并返回该 JSON 值的类型。

#endif /* LEPTJSON_H__ */
//是头文件的结束标志，用于结束 #ifndef 块。
// 在这里，#endif 后面的注释 LEPTJSON_H__ 是为了确保该标志与 #ifndef 的保护宏名称匹配。
