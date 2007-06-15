#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <stdarg.h>
#include <errno.h>
#include <fcntl.h>
#include <assert.h>
#include <sys/types.h>

#ifndef MYSTRING_H
#define MYSTRING_H

#ifndef TYPEDEF_MYSTRING
#define TYPEDEF_MYSTRING
typedef struct _mystring{
				
}mystring;
#endif

#define MEM_CPY(dest, src, len){\
	char *_ct = (char *)dest;\
	char *_cs = (char *)src;\
	int _cnt = len;\
	while(_cnt--)\
		*_ct++ = *_cs++;\
}

#define MEM_SET(dest, chr, len){\
	int _cnt = len;\
	char *_ct = dest;\
	while(_cnt--){\
		*_ct++ = chr;\
	}\
}

#define MEM_MOVE(dest, src, len){\
	char *_ct 	= dest;\
	char *_cs 	= src;\
	int _cnt 	= len;\
	if(dest <= src){\
		while(_cnt--)\
			*_ct++ = *_cs++;\
	}else{\
		_ct += _cnt;\
		_cs += _cnt;\
		while(_cnt--)\
			*--_ct = *--_cs;\
	}\
}

#define MEM_CMP(ret, str1, str2, len){\
	char *_cs = str1;\
	char *_ct = str2;\
	int _cnt = len;\
	for(; _cnt > 0; _cs++, _ct++, _cnt--){\
		if((ret = *_cs - *_ct ) != 0)\
			break;\
	}\
}

#define MEM_DUP(dest, src, len){\
	dest = (char *)calloc(1, (len + 1));\
	char *_ct = dest;\
	char *_cs = src;\
	int _cnt = len;\
	while(_cnt--){\
		*_ct++ = *_cs++;\
	}\
}

#define MEM_DUMP(src, len){\
	char *_chrs 	= src;\
	int  _cnt 	= len;\
	while(_cnt--){\
		if(*_chrs != 0)\
			printf("%c", *_chrs++);\
		else\
			printf("\\0", *_chrs++);\
	}\
}

#define MEM_SEARCH(index, from, to, search, len){\
	char *_from 	= from;\
	int  _ret   	= -1;\
	char *_search	= search;\
	int   _cnt  	= len;\
	while(_from < to){\
		for(_search = search, _cnt = len; _cnt > 0; _cnt--){\
			if((_ret = *_search++ - *_from++) != 0)\
				break;\
		}\
		if(_ret == 0){\
			index = _from - len;\
				break;\
		}\
	}\
}

#ifndef DEF_STR_RTRIM
#define DEF_STR_RTRIM
#define STR_RTRIM(_str){\
	char *_s = _str;\
        while(*_s != 0 )_s++;\
	while(*_s == 0x20 && *_s >= _str)*_s-- = 0;\
}
#endif

#ifndef DEF_STR_LTRIM
#define DEF_STR_LTRIM
#define STR_LTRIM(_str){\
        while(*_str == 0x20){*_str++ = 0;\
}
#endif

#define  N2NULL(_src, _src_len, _dst, _dst_len){\
        char *_s = _src;\
        char *_s1 = _dst;\
        char *_end = _src + _src_len;\
        while(_s < _end){\
                if(*_s == '\r')_s++;\
                else if(*_s == '\n'){*_s1++ = '\0';_s++;}\
                else *_s1++ = *_s++;\
        }\
        _dst_len = _s1 - _dst;\
}

#define N2PRINT(_src, _len){ \
	char *_p = _src; \
	char *_end = _src + _len;\
	while(_p < _end) printf("%c", *_p++);\
	printf("%c", '\0');\
}
/**
 * memory block duplicate
 * @src: memory source pointer
 * @len: length of source block
 *      return new memory block pointer
 */
char *mem_dup(char *src, int len);

/**
 * search @s in @src and return the pointer of first @s in @src
 * @src: source memory pointer
 * @src_len: length of source
 * @s: searched momery block
 * @s_len: length of searched block
 *      return pointer of first @s in @src
 */
char *mem_search(char *src, int src_len, char *s, int s_len);

/**
 * split string to array , the string maybe containt '\0'
 * @src: string source
 * @src_len: length of src
 * @sep: seperator
 * @sep_len: length of seperator
 * @out: out Array
 *   return the array length
 */
int str_split(char *src, int src_len, char *sep, int sep_len, char ***out);

/**
 * replace @search in @src with @replace all the @ could contain '\0'
 * @src: The source string
 * @src_len: The length of @src
 * @search: The search string
 * @s_len: The length of search
 * @replace: The replace string
 * @r_len: The length of replace
 * @out: The out pointer
 *      return the length of result string porinter after being replaced
 */
int str_replace(char *src, int src_len, char *search, int s_len, char *replace, int r_len, char **outp);

/**
 * str_sub
 *
 */

/**
 * free memory allocated  by  str_split and so on
 * @arr: array pointer
 * @size: array size
 *
 */
int str_free(char **arr, int size);

/**
 * print array(string) list
 * @arr: array pointer
 * @size: array size
 */
int str_print(char **arr, int size);


#endif


