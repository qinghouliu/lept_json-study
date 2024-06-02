#ifndef LEPTJSON_H__
#define LEPTJSON_H__

#include <stddef.h> /* size_t */

typedef enum { LEPT_NULL, LEPT_FALSE, LEPT_TRUE, LEPT_NUMBER, LEPT_STRING, LEPT_ARRAY, LEPT_OBJECT } lept_type;

#define LEPT_KEY_NOT_EXIST ((size_t)-1)

typedef struct lept_value lept_value;
typedef struct lept_member lept_member;

struct lept_value {
    union {
        struct { lept_member* m; size_t size, capacity; }o; /* object: members, member count, capacity */
        struct { lept_value*  e; size_t size, capacity; }a; /* array:  elements, element count, capacity */
        struct { char* s; size_t len; }s;                   /* string: null-terminated string, string length */
        double n;                                           /* number */
    }u;
    lept_type type;
};

struct lept_member {
    char* k; size_t klen;   /* member key string, key string length */
    lept_value v;           /* member value */
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
    LEPT_PARSE_MISS_KEY,
    LEPT_PARSE_MISS_COLON,
    LEPT_PARSE_MISS_COMMA_OR_CURLY_BRACKET
};

#define lept_init(v) do { (v)->type = LEPT_NULL; } while(0)
//1个宏，用于初始化一个 lept_value 实例，将其类型设置为 LEPT_NULL

int lept_parse(lept_value* v, const char* json);
//解析 JSON 文本，将结果存储在 lept_value 结构体中。
char* lept_stringify(const lept_value* v, size_t* length);
//将 lept_value 结构体转换为 JSON 文本字符串。
void lept_copy(lept_value* dst, const lept_value* src);//lept_value 的内容复制到另一个

void lept_move(lept_value* dst, lept_value* src);//移动一个 lept_value 的内容到另一个，移动后源对象将不再包含任何数据

void lept_swap(lept_value* lhs, lept_value* rhs);//交换两个 lept_value 的内容

void lept_free(lept_value* v);//释放 lept_value 占用的资源，如字符串或数组成员。

lept_type lept_get_type(const lept_value* v);//获取 lept_value 的类型。
int lept_is_equal(const lept_value* lhs, const lept_value* rhs);//比较两个 lept_value 是否相等。

#define lept_set_null(v) lept_free(v)//将 lept_value 设置为 null 类型。

int lept_get_boolean(const lept_value* v);//获取和设置布尔值。
void lept_set_boolean(lept_value* v, int b);

double lept_get_number(const lept_value* v);//获取和设置数值
void lept_set_number(lept_value* v, double n);

const char* lept_get_string(const lept_value* v);//获取和设置字符串。
size_t lept_get_string_length(const lept_value* v);
void lept_set_string(lept_value* v, const char* s, size_t len);

void lept_set_array(lept_value* v, size_t capacity);//于设置一个 JSON 值为数组类型，并预留一定的容量
size_t lept_get_array_size(const lept_value* v);//用于获取数组类型 JSON 值中当前包含的元素数量。
size_t lept_get_array_capacity(const lept_value* v);//用于获取数组类型 JSON 值中当前预留的容量大小
void lept_reserve_array(lept_value* v, size_t capacity);//

void lept_shrink_array(lept_value* v);//用于收缩数组类型 JSON 值的容量，使其适应实际元素数量

void lept_clear_array(lept_value* v);//这是一个函数声明，用于清空数组类型 JSON 值中的所有元素

lept_value* lept_get_array_element(lept_value* v, size_t index);//用于获取数组类型 JSON 值中指定索引处的元素。
lept_value* lept_pushback_array_element(lept_value* v);//用于向数组类型 JSON 值末尾添加一个元素，并返回新添加元素的指针。
void lept_popback_array_element(lept_value* v);//用于删除数组类型 JSON 值末尾的一个元素。
lept_value* lept_insert_array_element(lept_value* v, size_t index);//用于向数组类型 JSON 值中指定索引处插入一个元素，并返回新插入元素的指针。
void lept_erase_array_element(lept_value* v, size_t index, size_t count);//用于删除数组类型 JSON 值中指定索引范围内的元素

void lept_set_object(lept_value* v, size_t capacity);//设置一个 JSON 值为对象类型，并预留一定的容量

size_t lept_get_object_size(const lept_value* v);//获取对象类型 JSON 值中当前包含的键值对数量

size_t lept_get_object_capacity(const lept_value* v);//获取对象类型 JSON 值中当前预留的容量大小。

void lept_reserve_object(lept_value* v, size_t capacity);//为对象类型 JSON 值预留一定的容量。

void lept_shrink_object(lept_value* v);//收缩对象类型 JSON 值的容量，使其适应实际键值对数量。

void lept_clear_object(lept_value* v);//清空对象类型 JSON 值中的所有键值对。

const char* lept_get_object_key(const lept_value* v, size_t index);//获取对象类型 JSON 值中指定索引处键的指针。

size_t lept_get_object_key_length(const lept_value* v, size_t index);//获取对象类型 JSON 值中指定索引处键的长度

lept_value* lept_get_object_value(lept_value* v, size_t index);//获取对象类型 JSON 值中指定索引处值的指针

size_t lept_find_object_index(const lept_value* v, const char* key, size_t klen);//在对象类型 JSON 值中查找指定键的索引

lept_value* lept_find_object_value(lept_value* v, const char* key, size_t klen);//对象类型 JSON 值中查找指定键的值。
lept_value* lept_set_object_value(lept_value* v, const char* key, size_t klen);//设置对象类型 JSON 值中指定键的值。
void lept_remove_object_value(lept_value* v, size_t index);//移除对象类型 JSON 值中指定索引处的键值对。

#endif /* LEPTJSON_H__ */
