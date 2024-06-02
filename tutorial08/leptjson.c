#ifdef _WINDOWS
#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#endif
#include "leptjson.h"
#include <assert.h>  // assert()
#include <errno.h> // errno, ERANGE
#include <math.h>  // HUGE_VAL
#include <stdio.h>   /* sprintf() */
#include <stdlib.h>  // NULL strtod() malloc(), realloc(), free()
#include <string.h> //memcpy()


#ifndef LEPT_PARSE_STACK_INIT_SIZE
#define LEPT_PARSE_STACK_INIT_SIZE 256
#endif

#ifndef LEPT_PARSE_STRINGIFY_INIT_SIZE
#define LEPT_PARSE_STRINGIFY_INIT_SIZE 256
#endif

#define EXPECT(c, ch) do{assert(*c->json==(ch));c->json++;}while(0)
#define ISDIGIT(ch) ((ch)>='0'&&(ch)<='9')
#define ISDIGIT1TO9(ch) ((ch)>='1'&&(ch)<='9')
#define PUTC(c,ch) do{*(char*)lept_context_push(c,sizeof(char))=(ch);}while(0)
#define PUTS(c, s, len)     memcpy(lept_context_push(c, len), s, len)

typedef struct {
	const char* json;
	char* stack;
	size_t size, top;
}lept_context;

static void* lept_context_push(lept_context* c, size_t size) {//添加元素
	void* ret;
	assert(size > 0);
	if (c->top + size >= c->size) {
		if (c->size == 0) {
			c->size = LEPT_PARSE_STACK_INIT_SIZE;
		}
		while (c->top + size >= c->size) {
			c->size += c->size >> 1;
		}
		c->stack = (char*)realloc(c->stack, c->size);
	}
	ret = c->stack + c->top;
	c->top += size;
	return ret;
}

static void* lept_context_pop(lept_context* c, size_t size) {//弹出元素
	assert(c->top >= size);
	return c->stack + (c->top -= size);
}

static void lept_parse_whitespace(lept_context* c) {//跳过无意义的字符
	const char* p = c->json;
	while (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r') {
		p++;
	}
	c->json = p;
}

static int lept_parse_literal(lept_context* c, lept_value* v, const char* literal, lept_type type) {
	////对 null false true进行重构直接进行解析
	size_t i;
	EXPECT(c, literal[0]);
	for (i = 0; literal[i + 1]; i++) {
		if (c->json[i] != literal[i + 1]) {
			return LEPT_PARSE_INVALID_VALUE;
		}
	}
	c->json += i;
	v->type = type;
	return LEPT_PARSE_OK;
}

static int lept_parse_number(lept_context* c, lept_value* v) {
	//进行数字的解析
	const char* p = c->json;
	if (*p == '-')p++;
	if (*p == '0') {
		p++;
	}
	else {
		if (!ISDIGIT1TO9(*p)) {
			return LEPT_PARSE_INVALID_VALUE;
		}
		for (p++; ISDIGIT(*p); p++);
	}
	if (*p == '.') {
		p++;
		if (!ISDIGIT(*p)) {
			return LEPT_PARSE_INVALID_VALUE;
		}
		for (p++; ISDIGIT(*p); p++);
	}
	if (*p == 'e' || *p == 'E') {
		p++;
		if (*p == '+' || *p == '-') {
			p++;
		}
		if (!ISDIGIT(*p))return LEPT_PARSE_INVALID_VALUE;
		for (p++; ISDIGIT(*p); p++);
	}
	errno = 0;
	v->u.n = strtod(c->json, NULL);
	if (errno == ERANGE && (v->u.n == HUGE_VAL || v->u.n == -HUGE_VAL)) {
		return LEPT_PARSE_NUMBER_TOO_BIG;
	}
	v->type = LEPT_NUMBER;
	c->json = p;
	return LEPT_PARSE_OK;
}
////这个函数的目的是从一个字符串中解析四个十六进制字符，并将其转换成一个无符号整数。
static const char* lept_parse_hex4(const char* p, unsigned* u) {
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
////这个函数将 Unicode 码点 u 转换成 UTF-8 编码形式，并将每个字节推入管理在上下文 c 中的堆栈。
static void lept_encode_utf8(lept_context* c, unsigned u) {
	if (u <= 0x7F)
		PUTC(c, u & 0xFF);
	else if (u <= 0x7FF) {
		PUTC(c, 0xC0 | ((u >> 6) & 0xFF));
		PUTC(c, 0x80 | (u & 0x3F));
	}
	else if (u <= 0xFFFF) {
		PUTC(c, 0xE0 | ((u >> 12) & 0xFF));
		PUTC(c, 0x80 | ((u >> 6) & 0x3F));
		PUTC(c, 0x80 | (u & 0x3F));
	}
	else {
		assert(u <= 0x10FFFF);
		PUTC(c, 0xF0 | ((u >> 18) & 0xFF));
		PUTC(c, 0x80 | ((u >> 12) & 0x3F));
		PUTC(c, 0x80 | ((u >> 6) & 0x3F));
		PUTC(c, 0x80 | (u & 0x3F));
	}
}

#define STRING_ERROR(ret) do { c->top = head; return ret; } while(0)
// 此宏定义用于处理字符串解析中出现的错误。当调用此宏时，它将栈顶指针(c->top) 重置为 head（即字符串解析开始时的位置），
//然后返回错误代码 ret。这样做是为了丢弃在解析过程中可能已经加入到堆栈的部分数据，保持数据一致性。
static int lept_parse_string_raw(lept_context* c, char** str, size_t* len) {
	size_t head = c->top;
	unsigned u, u2;
	const char* p;
	EXPECT(c, '\"');
	p = c->json;
	for (;;) {
		char ch = *p++;
		switch (ch) {
		case '\"':
			*len = c->top - head;
			*str = (char*)lept_context_pop(c, *len);
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
				break;
			default:
				STRING_ERROR(LEPT_PARSE_INVALID_STRING_ESCAPE);
			}
			break;
		case '\0':
			STRING_ERROR(LEPT_PARSE_MISS_QUOTATION_MARK);
		default:
			if ((unsigned char)ch < 0x20) {
				STRING_ERROR(LEPT_PARSE_INVALID_STRING_CHAR);
			}
			PUTC(c, ch);
		}
	}
}

static int lept_parse_string(lept_context* c, lept_value* v) {
	// 对其进行重构，和之前的不一样
	int ret;
	char* s;
	size_t len;
	if ((ret = lept_parse_string_raw(c, &s, &len)) == LEPT_PARSE_OK) {
		lept_set_string(v, s, len);
	}
	return ret;
}

static int lept_parse_value(lept_context* c, lept_value* v);

static int lept_parse_array(lept_context* c, lept_value* v) {//解析数组元素
	size_t i, size = 0;
	int ret;
	EXPECT(c, '[');
	lept_parse_whitespace(c);
	if (*c->json == ']') {
		c->json++;
		v->type = LEPT_ARRAY;
		v->u.a.size = 0;
		v->u.a.e = NULL;
		return LEPT_PARSE_OK;
	}
	for (;;) {
		lept_value e;
		lept_init(&e);
		if ((ret = lept_parse_value(c, &e)) != LEPT_PARSE_OK) {
			break;
		}
		memcpy(lept_context_push(c, sizeof(lept_value)), &e, sizeof(lept_value));
		size++;
		lept_parse_whitespace(c);
		if (*c->json == ',') {
			c->json++;
			lept_parse_whitespace(c);
		}
		else if (*c->json == ']') {
			c->json++;
			v->type = LEPT_ARRAY;
			v->u.a.size = size;
			size *= sizeof(lept_value);
			memcpy(v->u.a.e = (lept_value*)malloc(size), lept_context_pop(c, size), size);
			return LEPT_PARSE_OK;
		}
		else {
			ret = LEPT_PARSE_MISS_COMMA_OR_SQUARE_BRACKET;
			break;
		}
	}
	/* Pop and free values on the stack */
	for (i = 0; i < size; i++)
		lept_free((lept_value*)lept_context_pop(c, sizeof(lept_value)));
	return ret;
}

static int lept_parse_object(lept_context* c, lept_value* v) {//解析对象元素
	size_t i, size;
	lept_member m;
	int ret;
	EXPECT(c, '{');
	lept_parse_whitespace(c);
	if (*c->json == '}') {
		c->json++;
		v->type = LEPT_OBJECT;
		v->u.o.m = 0;
		v->u.o.size = 0;
		return LEPT_PARSE_OK;
	}
	m.k = NULL;
	size = 0;
	for (;;) {
		char* str;
		lept_init(&m.v);
		/* parse key to m.k, m.klen */
		if (*c->json != '"') {
			ret = LEPT_PARSE_MISS_KEY;
			break;
		}
		if ((ret = lept_parse_string_raw(c, &str, &m.klen)) != LEPT_PARSE_OK) {
			break;
		}
		memcpy(m.k = (char*)malloc(m.klen + 1), str, m.klen);
		m.k[m.klen] = '\0';
		/* parse ws colon ws */
		lept_parse_whitespace(c);
		if (*c->json != ':') {
			ret = LEPT_PARSE_MISS_COLON;
			break;
		}
		c->json++;
		lept_parse_whitespace(c);
		/* parse value */
		if ((ret = lept_parse_value(c, &m.v)) != LEPT_PARSE_OK) {
			break;
		}
		memcpy(lept_context_push(c, sizeof(lept_member)), &m, sizeof(lept_member));
		size++;
		m.k = NULL;/* ownership is transferred to member on stack */
		/* parse ws [comma | right-curly-brace] ws */
		lept_parse_whitespace(c);
		if (*c->json == ',') {
			c->json++;
			lept_parse_whitespace(c);
		}
		else if (*c->json == '}') {
			size_t s = sizeof(lept_member) * size;
			c->json++;
			v->type = LEPT_OBJECT;
			v->u.o.size = size;
			memcpy(v->u.o.m = (lept_member*)malloc(s), lept_context_pop(c, s), s);
			return LEPT_PARSE_OK;
		}
		else {
			ret = LEPT_PARSE_MISS_COMMA_OR_CURLY_BRACKET;
			break;
		}
	}
	/* Pop and free members on the stack */
	free(m.k);
	for (i = 0; i < size; i++) {
		lept_member* m = (lept_member*)lept_context_pop(c, sizeof(lept_member));
		free(m->k);
		lept_free(&m->v);
	}
	v->type = LEPT_NULL;
	return ret;
}

static int lept_parse_value(lept_context* c, lept_value* v) {//解析的主要框架
	switch (*c->json) {
	case 'n': return lept_parse_literal(c, v, "null", LEPT_NULL);
	case 't': return lept_parse_literal(c, v, "true", LEPT_TRUE);
	case 'f': return lept_parse_literal(c, v, "false", LEPT_FALSE);
	default:  return lept_parse_number(c, v);
	case '"': return lept_parse_string(c, v);
	case '[': return lept_parse_array(c, v);
	case '{': return lept_parse_object(c, v);
	case '\0': return LEPT_PARSE_EXPECT_VALUE;
	}
}

int lept_parse(lept_value* v, const char* json) {//解析
	lept_context c;
	int ret;
	assert(v != NULL);
	c.json = json;
	c.stack = NULL;
	c.size = c.top = 0;
	lept_init(v);
	lept_parse_whitespace(&c);
	ret = lept_parse_value(&c, v);
	if (ret == LEPT_PARSE_OK) {
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
//lept_stringify_string 主要用于在 JSON 字符串化过程中处理和转换字符串，包括转义特殊字符
static void lept_stringify_string(lept_context* c, const char* s, size_t len) {
	static const char hex_digits[] = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F' };
	size_t i, size;
	char* head, * p;
	assert(s != NULL);
	p = head = (char*)lept_context_push(c, size = len * 6 + 2); /* "\u00xx..." */
	*p++ = '"';
	for (i = 0; i < len; i++) {
		unsigned char ch = (unsigned char)s[i];
		switch (ch) {
		case '\"': *p++ = '\\'; *p++ = '\"'; break;
		case '\\': *p++ = '\\'; *p++ = '\\'; break;
		case '\b': *p++ = '\\'; *p++ = 'b';  break;
		case '\f': *p++ = '\\'; *p++ = 'f';  break;
		case '\n': *p++ = '\\'; *p++ = 'n';  break;
		case '\r': *p++ = '\\'; *p++ = 'r';  break;
		case '\t': *p++ = '\\'; *p++ = 't';  break;
		default:
			if (ch < 0x20) {
				*p++ = '\\'; *p++ = 'u'; *p++ = '0'; *p++ = '0';
				*p++ = hex_digits[ch >> 4];
				*p++ = hex_digits[ch & 15];
			}
			else
				*p++ = s[i];
		}
	}
	*p++ = '"';
	c->top -= size - (p - head);
}

static void lept_stringify_value(lept_context* c, const lept_value* v) {
	size_t i;
	switch (v->type) {
	case LEPT_NULL:   PUTS(c, "null", 4); break;
	case LEPT_FALSE:  PUTS(c, "false", 5); break;
	case LEPT_TRUE:   PUTS(c, "true", 4); break;
	case LEPT_NUMBER: c->top -= 32 - sprintf((char*)lept_context_push(c, 32), "%.17g", v->u.n); break;
	case LEPT_STRING: lept_stringify_string(c, v->u.s.s, v->u.s.len); break;
	case LEPT_ARRAY:
		PUTC(c, '[');
		for (i = 0; i < v->u.a.size; i++) {
			if (i > 0)
				PUTC(c, ',');
			lept_stringify_value(c, &v->u.a.e[i]);
		}
		PUTC(c, ']');
		break;
	case LEPT_OBJECT:
		PUTC(c, '{');
		for (i = 0; i < v->u.o.size; i++) {
			if (i > 0)
				PUTC(c, ',');
			lept_stringify_string(c, v->u.o.m[i].k, v->u.o.m[i].klen);
			PUTC(c, ':');
			lept_stringify_value(c, &v->u.o.m[i].v);
		}
		PUTC(c, '}');
		break;
	default: assert(0 && "invalid type");
	}
}
//lept_stringify 是一个用于将 lept_value 结构（代表 JSON 数据）转换成一个字符串的顶层函数
char* lept_stringify(const lept_value* v, size_t* length) {
	lept_context c;
	assert(v != NULL);
	c.stack = (char*)malloc(c.size = LEPT_PARSE_STRINGIFY_INIT_SIZE);
	c.top = 0;
	lept_stringify_value(&c, v);
	if (length) {
		*length = c.top;
	}
	PUTC(&c, '\0');
	return c.stack;
}

void lept_copy(lept_value* dst, const lept_value* src) {//这是一个函数声明，表示将源 JSON 值（src）复制到目标 JSON 值（dst）中。
	assert(src != NULL && dst != NULL && src != dst);//执行源和目标是否为空的判断
	size_t i;
	switch (src->type) {
	case LEPT_STRING:
		lept_set_string(dst, src->u.s.s, src->u.s.len);//把源的字符串和字符串的长度进行赋值到目的
		break;
	case LEPT_ARRAY:
		/* \todo */
		//数组
		// 先设置大小
		lept_set_array(dst, src->u.a.size);//先通过源的数组的大小，给目的数组进行分配大小
		// 逐个拷贝
		for (i = 0; i < src->u.a.size; i++) {
			lept_copy(&dst->u.a.e[i], &src->u.a.e[i]);//递归对每个数组的元素进行赋值
		}
		dst->u.a.size = src->u.a.size;//设置目标数组的大小与源数组相同
		break;
	case LEPT_OBJECT:
		/* \todo */
		//对象
		// 先设置大小
		lept_set_object(dst, src->u.o.size);//先调用 lept_set_object 函数，为目标 JSON 值分配合适大小的对象空间
		// 逐个拷贝, 先key后value
		for (i = 0; i < src->u.o.size; i++) {
			//k
			// 设置k字段为key的对象的值，如果在查找过程中找到了已经存在key，则返回；否则新申请一块空间并初始化，然后返回
			lept_value* val = lept_set_object_value(dst, src->u.o.m[i].k, src->u.o.m[i].klen);
			//然后使用循环逐个复制源对象中的键值对到目标对象中
			lept_copy(val, &src->u.o.m[i].v);
		}
		dst->u.o.size = src->u.o.size;
		break;
	default://如果是其他类型的，就直接进行赋值，不会出现嵌套的情况
		lept_free(dst);
		memcpy(dst, src, sizeof(lept_value));
		break;
	}
}

void lept_move(lept_value* dst, lept_value* src) {
	//定义函数 lept_move，该函数接受两个参数：目标指针 dst 和源指针 src，都指向 lept_value 结构体
	assert(dst != NULL && src != NULL && src != dst);
	lept_free(dst);
	//memcpy 函数将 src 指向的对象的内容复制到 dst 指向的对象中。复制的大小是一个 lept_value 结构体的大小。
	memcpy(dst, src, sizeof(lept_value));
	lept_init(src);
	//初始化 src 指向的对象，使其回到一个安全的默认状态。
}

void lept_swap(lept_value* lhs, lept_value* rhs) {
	assert(lhs != NULL && rhs != NULL);
	if (lhs != rhs) {//如果两个指针不指向同一个对象，则执行交换。
		//定义函数 lept_swap，用于交换两个 lept_value 结构体的内容。参数 lhs 和 rhs 分别表示左侧和右侧的对象。
		lept_value temp;//定义一个临时变量 temp 用于辅助交换。
		memcpy(&temp, lhs, sizeof(lept_value));
		memcpy(lhs, rhs, sizeof(lept_value));
		memcpy(rhs, &temp, sizeof(lept_value));
		//使用 memcpy 函数来实现三次复制，从而交换 lhs 和 rhs 的内容。
	}
}

void lept_free(lept_value* v) {//定义函数 lept_free，用来释放一个 lept_value 对象占用的资源
	size_t i;//声明变量 i 用于循环，使用 assert 确保 v 不为 NULL。
	assert(v != NULL);
	switch (v->type) {
	case LEPT_STRING://如果类型是字符串，则释放字符串所占用的内存
		free(v->u.s.s);
		break;
	case LEPT_ARRAY://如果类型是数组，则递归地释放数组中每个元素占用的资源，最后释放数组本身的内存。
		for (i = 0; i < v->u.a.size; i++)
			lept_free(&v->u.a.e[i]);
		free(v->u.a.e);
		break;
	case LEPT_OBJECT:
	//如果类型是对象，则递归地释放对象中每个键值对的资源，包括键的内存和值所占的资源，最后释放键值对数组的内存。
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

lept_type lept_get_type(const lept_value* v) {//此函数返回JSON值的类型
	assert(v != NULL);
	return v->type;
}

int lept_is_equal(const lept_value* lhs, const lept_value* rhs) {
	size_t i;
	assert(lhs != NULL && rhs != NULL);// 断言两个JSON值指针均非空。
	if (lhs->type != rhs->type)
		return 0;
	switch (lhs->type) {
	case LEPT_STRING://// 如果JSON值是字符串。
		return lhs->u.s.len == rhs->u.s.len &&
			memcmp(lhs->u.s.s, rhs->u.s.s, lhs->u.s.len) == 0;/// 比较字符串内容是否相同。
	case LEPT_NUMBER://如果JSON值是数字。
		return lhs->u.n == rhs->u.n;
	case LEPT_ARRAY:
		if (lhs->u.a.size != rhs->u.a.size)// 首先检查数组大小是否不同。
			return 0;
		for (i = 0; i < lhs->u.a.size; i++)
			if (!lept_is_equal(&lhs->u.a.e[i], &rhs->u.a.e[i]))//// 递归检查数组元素是否相等
				return 0;
		return 1;//// 如果所有元素都匹配，返回1（真）。
	case LEPT_OBJECT:
		/* \todo */
		// 对于对象，先比较键值对个数是否一样
		// 一样的话，对左边的键值对，依次在右边进行寻找
		if (lhs->u.o.size != rhs->u.o.size)
			return 0;
		/* key-value comp */
		for (i = 0; i < lhs->u.o.size; i++) {
			size_t index = lept_find_object_index(rhs, lhs->u.o.m[i].k, lhs->u.o.m[i].klen);
			if (index == LEPT_KEY_NOT_EXIST) {
				return 0;
			}//key not exist
			if (!lept_is_equal(&lhs->u.o.m[i].v, &rhs->u.o.m[index].v)) return 0; //value not match
		}
		return 1;// // 对于不需要特别比较的类型，返回真
	default:
		return 1;// 如果类型比较通过，则返回真。
	}
}
//从JSON值中获取布尔值，确保它确实是布尔类型。
int lept_get_boolean(const lept_value* v) {
	assert(v != NULL && (v->type == LEPT_TRUE || v->type == LEPT_FALSE));
	return v->type == LEPT_TRUE;
}
//设置JSON值的布尔类型，先清理任何现有的值。
void lept_set_boolean(lept_value* v, int b) {
	lept_free(v);
	v->type = b ? LEPT_TRUE : LEPT_FALSE;
}
//从JSON值中检索数值，确保它确实是数字。
double lept_get_number(const lept_value* v) {
	assert(v != NULL && v->type == LEPT_NUMBER);
	return v->u.n;
}
//设置JSON值的数值，先清理任何现有的值。
void lept_set_number(lept_value* v, double n) {
	lept_free(v);
	v->u.n = n;
	v->type = LEPT_NUMBER;
}

const char* lept_get_string(const lept_value* v) {//该函数返回存储在JSON值中的字符串。
	assert(v != NULL && v->type == LEPT_STRING);
	return v->u.s.s;
}

size_t lept_get_string_length(const lept_value* v) {//获取并返回JSON字符串的长度。
	assert(v != NULL && v->type == LEPT_STRING);
	return v->u.s.len;
}

void lept_set_string(lept_value* v, const char* s, size_t len) {
	//设置JSON值为指定的字符串，复制字符串内容，并确保其以空字符结尾。
	assert(v != NULL && (s != NULL || len == 0));
	lept_free(v);
	v->u.s.s = (char*)malloc(len + 1);
	memcpy(v->u.s.s, s, len);
	v->u.s.s[len] = '\0';
	v->u.s.len = len;
	v->type = LEPT_STRING;
}

void lept_set_array(lept_value* v, size_t capacity) {//设置JSON值为数组类型，分配内存以存储指定容量的元素。
	// 给数组分配内存
	assert(v != NULL);
	lept_free(v);
	v->type = LEPT_ARRAY;
	v->u.a.size = 0;
	v->u.a.capacity = capacity;
	v->u.a.e = capacity > 0 ? (lept_value*)malloc(capacity * sizeof(lept_value)) : NULL;
}

size_t lept_get_array_size(const lept_value* v) {//获取数组中当前的元素数量
	assert(v != NULL && v->type == LEPT_ARRAY);
	return v->u.a.size;
}

size_t lept_get_array_capacity(const lept_value* v) {//获取数组的容量，即最多可以存储的元素数量
	assert(v != NULL && v->type == LEPT_ARRAY);
	return v->u.a.capacity;
}

// 将数组容量扩充到capacity，如果原来数组容量比capacity大，则无操作
//如果需要，扩展数组的容量，保证可以存放更多的元素。
void lept_reserve_array(lept_value* v, size_t capacity) {
	assert(v != NULL && v->type == LEPT_ARRAY);
	if (v->u.a.capacity < capacity) {
		v->u.a.capacity = capacity;
		v->u.a.e = (lept_value*)realloc(v->u.a.e, capacity * sizeof(lept_value));
	}
}

// 收缩数组容量
void lept_shrink_array(lept_value* v) {//收缩数组的容量，使其等于当前存储的元素数量，减少内存占用。
	assert(v != NULL && v->type == LEPT_ARRAY);
	if (v->u.a.capacity > v->u.a.size) {
		v->u.a.capacity = v->u.a.size;
		v->u.a.e = (lept_value*)realloc(v->u.a.e, v->u.a.capacity * sizeof(lept_value));
	}
}

void lept_clear_array(lept_value* v) {
	//清空数组中的所有元素，基本上是通过删除从开始到数组结束的所有元素实现的。
	assert(v != NULL && v->type == LEPT_ARRAY);
	lept_erase_array_element(v, 0, v->u.a.size);
}

lept_value* lept_get_array_element(const lept_value* v, size_t index) {
	//获取数组中指定索引位置的元素的指针。这使得可以直接访问或修改该元素
	assert(v != NULL && v->type == LEPT_ARRAY);
	assert(index < v->u.a.size);
	return &v->u.a.e[index];
}

lept_value* lept_pushback_array_element(lept_value* v) {
	//在数组末尾添加一个新元素，如果数组已满，则首先扩大容量。返回新元素的指针
	assert(v != NULL && v->type == LEPT_ARRAY);
	if (v->u.a.size == v->u.a.capacity)
		lept_reserve_array(v, v->u.a.capacity == 0 ? 1 : v->u.a.capacity * 2);
	lept_init(&v->u.a.e[v->u.a.size]);
	return &v->u.a.e[v->u.a.size++];
}

// 释放最后一个元素
void lept_popback_array_element(lept_value* v) {
	//从数组末尾移除一个元素，并释放其占用的资源。
	assert(v != NULL && v->type == LEPT_ARRAY && v->u.a.size > 0);
	lept_free(&v->u.a.e[--v->u.a.size]);
	//从数组末尾移除一个元素，并释放其占用的资源。
}

// index不可以超过size（因为是插入）（等于的话，相当于插在末尾）
lept_value* lept_insert_array_element(lept_value* v, size_t index) {
	assert(v != NULL && v->type == LEPT_ARRAY && index <= v->u.a.size);
	// // 确保 v 非空，类型为数组，且索引有效
	/* \todo */
	if (v->u.a.size == v->u.a.capacity) {//// 如果数组已满
		lept_reserve_array(v, v->u.a.capacity == 0 ? 1 : (v->u.a.size << 1)); //扩容为原来一倍
	}
	memcpy(&v->u.a.e[index + 1], &v->u.a.e[index], (v->u.a.size - index) * sizeof(lept_value));
	//  将 index 后的元素向后移动
	lept_init(&v->u.a.e[index]);//初始化新插入的元素位置。
	v->u.a.size++;// 增加数组大小。
	return &v->u.a.e[index];// 返回新元素的位置。
}
// 回收完空间，然后将index 后面count个元素移到index处，最后将空闲的count个元素重新初始化

void lept_erase_array_element(lept_value* v, size_t index, size_t count) {
	//  确保输入有效，且删除的范围在数组大小内。
	assert(v != NULL && v->type == LEPT_ARRAY && index + count <= v->u.a.size);
	/* \todo */
	size_t i;
	for (i = index; i < index + count; i++) {//// 循环释放指定范围内的元素。
		lept_free(&v->u.a.e[i]);
	}
	memcpy(v->u.a.e + index, v->u.a.e + index + count, (v->u.a.size - index - count) * sizeof(lept_value));
	//// 将删除元素后的部分前移
	for (i = v->u.a.size - count; i < v->u.a.size; i++)//初始化被删除后空出来的位置。
		lept_init(&v->u.a.e[i]);
	v->u.a.size -= count;//// 减小数组大小
}

void lept_set_object(lept_value* v, size_t capacity) {
	assert(v != NULL);  // 确保 v 非空。
	lept_free(v);  // 清理 v 指向的值。
	v->type = LEPT_OBJECT;  // 设置 v 的类型为对象。
	v->u.o.size = 0;  // 初始化对象中的成员数量为0。
	v->u.o.capacity = capacity;  // 设置对象的容量。
	v->u.o.m = capacity > 0 ? (lept_member*)malloc(capacity * sizeof(lept_member)) : NULL;  // 为对象成员分配内存。
}

size_t lept_get_object_size(const lept_value* v) {
	assert(v != NULL && v->type == LEPT_OBJECT);  // 确保 v 非空且类型为对象。
	return v->u.o.size;  // 返回对象中的成员数量。
}

size_t lept_get_object_capacity(const lept_value* v) {
	assert(v != NULL && v->type == LEPT_OBJECT);
	/* \todo */
	// 返回对象容量大小
	return v->u.o.capacity;
}

void lept_reserve_object(lept_value* v, size_t capacity) {
	assert(v != NULL && v->type == LEPT_OBJECT);  // 确保 v 非空且类型为对象。
	if (v->u.o.capacity < capacity) {  // 如果当前容量小于新的容量。
		v->u.o.capacity = capacity;  // 更新对象的容量。
		v->u.o.m = (lept_member*)realloc(v->u.o.m, capacity * sizeof(lept_member));  
		// 重新分配内存以匹配新的容量。
	}
}

void lept_shrink_object(lept_value* v) {//和对数组的操作类似，直接进行收缩
	assert(v != NULL && v->type == LEPT_OBJECT);
	/* \todo */
	// 收缩容量到刚好符合大小
	if (v->u.o.capacity > v->u.o.size) {
		v->u.o.capacity = v->u.o.size;
		v->u.o.m = (lept_member*)realloc(v->u.o.m, v->u.o.capacity * sizeof(lept_member));
	}
}

void lept_clear_object(lept_value* v) {
	assert(v != NULL && v->type == LEPT_OBJECT);
	/* \todo */
	// 清空对象
	size_t i;
	for (i = 0; i < v->u.o.size; i++) {
		//回收k和v空间
		free(v->u.o.m[i].k);
		v->u.o.m[i].k = NULL;
		v->u.o.m[i].klen = 0;
		lept_free(&v->u.o.m[i].v);
	}
	v->u.o.size = 0;
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

lept_value* lept_get_object_value(lept_value* v, size_t index) {
	assert(v != NULL && v->type == LEPT_OBJECT);
	assert(index < v->u.o.size);
	return &v->u.o.m[index].v;
}

size_t lept_find_object_index(const lept_value* v, const char* key, size_t klen) {
	//lept_find_object_index，它接受三个参数：一个指向 lept_value 结构体的常量指针 v（表示 JSON 对象），
	// 一个常量字符指针 key（表示要查找的键），和一个 size_t 类型的 klen（表示键的长度)
	size_t i;
	assert(v != NULL && v->type == LEPT_OBJECT && key != NULL);
	//使用断言确保传入的 v 不为 NULL，
	// 其类型为 LEPT_OBJECT（确保这是一个 JSON 对象），并且 key 也不为 NULL。这是为了防止程序运行时错误或者无效输入。

	for (i = 0; i < v->u.o.size; i++)
	//一个变量 i 从 0 开始，用于遍历 JSON 对象中的所有键值对。循环继续，直到遍历完对象中的所有元素（v->u.o.size 是对象中的元素个数）。

		if (v->u.o.m[i].klen == klen && memcmp(v->u.o.m[i].k, key, klen) == 0)

			//对每个元素，首先检查当前元素的键的长度 (v->u.o.m[i].klen) 是否与搜索的键的长度 (klen) 相同。
			// 如果长度相同，再使用 memcmp 函数比较两个键的内容是否相同。如果这两个条件都满足，
			// 说明找到了键，函数返回当前的索引 i
			//memcmp 函数用于比较两块内存区域的内容。在这里，它比较的是两个字符串（键名）：一个是 JSON 对象中已存储的键 v->u.o.m[i].k，
			//另一个是我们正在查找的键 key。这两个字符串的长度都是 klen。
			// memcmp 函数会逐字节比较这两个内存区域，如果所有的字节都相等，函数返回 0。

			return i;
	return LEPT_KEY_NOT_EXIST;
}

lept_value* lept_find_object_value(lept_value* v, const char* key, size_t klen) {//寻找对象键对应的值
	size_t index = lept_find_object_index(v, key, klen);
	return index != LEPT_KEY_NOT_EXIST ? &v->u.o.m[index].v : NULL;
}

// 设置k字段为key的对象的值，如果在查找过程中找到了已经存在key，则返回；否则新申请一块空间并初始化，然后返回
lept_value* lept_set_object_value(lept_value* v, const char* key, size_t klen) {
	//lept_set_object_value，接收三个参数：一个指向 lept_value 结构体的指针 v，一个常量字符指针 key 表示要设置或添加的键，

	assert(v != NULL && v->type == LEPT_OBJECT && key != NULL);
	//断言确保传入的 v 不为 NULL，v 的类型必须是 LEPT_OBJECT（表明它是一个 JSON 对象），并且键 key 也不为 NULL。
	// 这是为了防止无效的输入导致程序错误。
	/* \todo */
	
	size_t i, index;
	index = lept_find_object_index(v, key, klen);
	//声明两个变量 i 和 index，然后调用 lept_find_object_index 函数查找键在对象中的索引。
	// 如果键已存在，lept_find_object_index 返回该键的位置索引；如果不存在，返回一个特殊值（如 LEPT_KEY_NOT_EXIST）。

	if (index != LEPT_KEY_NOT_EXIST)//键已存在（索引不是 LEPT_KEY_NOT_EXIST），则直接返回这个键对应值的地址。
		return &v->u.o.m[index].v;
	
	//key not exist, then we make room and init
	if (v->u.o.size == v->u.o.capacity) {
		//如果键不存在，首先检查当前对象的大小是否已经达到其容量，如果是，则需要扩展容量。
		// 这里使用 lept_reserve_object 函数扩展容量，如果当前容量为 0，则扩展为 1，否则扩展为当前容量的两倍。
		lept_reserve_object(v, v->u.o.capacity == 0 ? 1 : (v->u.o.capacity << 1));
	}
	i = v->u.o.size;
	v->u.o.m[i].k = (char*)malloc((klen + 1));
	memcpy(v->u.o.m[i].k, key, klen);
	v->u.o.m[i].k[klen] = '\0';
	v->u.o.m[i].klen = klen;
	//为新键分配内存，并拷贝键名。malloc((klen + 1)) 分配足够的内存以存储键及其结尾的空字符。
	// 使用 memcpy 将键复制到分配的内存中，然后设置字符串的结束符 \0。同时设置键的长度。
	lept_init(&v->u.o.m[i].v);
	v->u.o.size++;
	// 初始化新键对应的值（通过 lept_init），并增加对象的大小计数。
	return &v->u.o.m[i].v;
	//返回新添加的键值对中值的地址
}

void lept_remove_object_value(lept_value* v, size_t index) {
	assert(v != NULL && v->type == LEPT_OBJECT && index < v->u.o.size);
	//确保传入的 v 不为 NULL，且 v 的类型必须是 LEPT_OBJECT（表明它是一个 JSON 对象），
	//并且 index 小于对象的大小 v->u.o.size，以防止无效输入导致程序错误
	/* \todo */
	free(v->u.o.m[index].k);
	lept_free(&v->u.o.m[index].v);
	//think like a list
	memcpy(v->u.o.m + index, v->u.o.m + index + 1, (v->u.o.size - index - 1) * sizeof(lept_member));   // 这里原来有错误
	// 原来的size比如是10，最多其实只能访问下标为9
	// 删除一个元素，再进行挪移，原来为9的地方要清空
	// 现在先将size--，则size就是9
	    //v->u.o.m + index
		//v->u.o.m 是指向 lept_member 数组的指针，index 是要删除的元素的位置。
		//v->u.o.m + index 表示从 index 位置开始的数组部分，也就是将要被覆盖的开始位置。
		//v->u.o.m + index + 1
		//v->u.o.m + index + 1 表示从 index + 1 位置开始的数组部分，也就是需要移动的开始位置。
		//(v->u.o.size - index - 1) * sizeof(lept_member)
		//v->u.o.size 是当前对象的大小（元素个数）。
		//v->u.o.size - index - 1 计算出从 index + 1 到数组末尾的元素个数。
		//sizeof(lept_member) 是每个元素（成员）的大小。
		//(v->u.o.size - index - 1) * sizeof(lept_member) 计算出需要移动的字节数。
	v->u.o.m[--v->u.o.size].k = NULL;
	v->u.o.m[v->u.o.size].klen = 0;
	lept_init(&v->u.o.m[v->u.o.size].v);
	// 
	// 错误，因为无法访问原来的size下标
	//v->u.o.m[v->u.o.size].k = NULL;
	//printf("v->u.o.size：  %d\n", v->u.o.size);
	//printf("v->u.o.m[v->u.o.size]：  %s\n", v->u.o.m[v->u.o.size]);
	//printf("v->u.o.m[v->u.o.size].klen： %d\n", v->u.o.m[v->u.o.size].klen);
	//v->u.o.m[v->u.o.size].klen = 0;
	//lept_init(&v->u.o.m[--v->u.o.size].v);

}
