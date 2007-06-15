#include "mystring.h"

/**
 * memory block duplicate
 * @src: memory source pointer
 * @len: length of source block
 *  	return new memory block pointer
 * 	NOTE: Free @return once @return  is NOT NULL after using is ,
	Or there will cause memory leaking
 */
char *mem_dup(char *src, int len){
        char *dest = (char *)calloc(1, (len + 1));
        char *a = dest;
        char *b = src;
        int cnt = len;
        while(cnt--){
                *a++ = *b++;
        }
	return dest;
}

/** 
 * search @s in @src and return the pointer of first @s in @src
 * @src: source memory pointer
 * @src_len: length of source 
 * @s: searched momery block  
 * @s_len: length of searched block
 *	return pointer of first @s in @src 
 */
char *mem_search(char *src, int src_len, char *s, int s_len){
	char *index = NULL;
        char *pos  = (char *)src;
        char *last = ((char *)src + src_len);
        char *chr  = s;
        int  ret   = -1;
        int   cnt  = s_len;
        while(pos < last){
                for(chr = s, cnt = s_len; cnt > 0; cnt--){
                        if((ret = *chr++ - *pos++) != 0)
                                break;
                }
                if(ret == 0){
                        index = pos - s_len;
                        break;
                }
        }
	return index;
}


/**
 * split string to array , the string maybe containt '\0'
 * @src: string source
 * @src_len: length of src
 * @sep: seperator
 * @sep_len: length of seperator
 * @out: out Array
 *   return the array length
 *	NOTE: Free @out with @str_free(@out, @out_len) once @out is NOT NULL after using it,
	Or there will cause memory leaking
 */
int str_split(char *src, int src_len, char *sep, int sep_len, char ***out){
	if(src == NULL || src_len == 0 || sep == NULL 
			|| sep_len == 0  || out == NULL ) return -1;

	char *pos   = (char *)src;
	char *last  = (char *)src + src_len; 
	
	int  i = 0;
	char **arr = (char **)calloc(1, sizeof(char *));
	int e_len = 0;
	while(pos  < last){
		char *idx  = NULL;
		MEM_SEARCH(idx, pos, last, sep, sep_len);
		if(idx != NULL){
			e_len = idx - pos;
			if(e_len > 0){
				arr = (char **) realloc(arr, sizeof(char *) * (i +1));
				MEM_DUP(arr[i], pos, e_len);
				pos = idx + sep_len ;
				i++;
			}else{
				pos = idx + sep_len;
			}
		}else{
 			break; 
		}	
	}	
	if(pos < last){
		arr = (char **) realloc(arr, sizeof(char *) * (i +1));
		e_len = last - pos;
		MEM_DUP(arr[i], pos, e_len);
		i++;
	}		
	*out = arr;
	return i;
}

/**
 * replace @search in @src with @replace all the @ could contain '\0'
 * @src: The source string
 * @src_len: The length of @src
 * @search: The search string
 * @s_len: The length of search
 * @replace: The replace string
 * @r_len: The length of replace
 * @out: The out pointer
 *  	return the length of result string porinter after being replaced
 * 	NOTE: Free @outp once @outp  is NOT NULL after using it,
	Or there will cause memory leaking
 */
int str_replace(char *src, int src_len, char *search, int s_len, char *replace, int r_len, char **outp){
	if(src == NULL || src_len == 0 || search == NULL 
			|| s_len == 0 || replace == NULL )  return -1;

	char *out 	= ( char *)calloc(1, src_len + 1);	
	if(out == NULL) return -1;
	char *pos 	= src;
	char *last 	= src + src_len;		
	int  sum	= r_len - s_len;
	int  out_len	= src_len;
	int  cur	= 0;
	while(pos < last){
		char *idx = NULL;
		MEM_SEARCH(idx, pos, last, search, s_len);
		if(idx != NULL){
			if(sum > 0 ) {
				out_len += sum; 					
				out = (char *)realloc(out, out_len + 1);
			}
			if(out == NULL) return -1;

			int dist_len = idx - pos;
			MEM_CPY((out + cur), pos, dist_len);
			cur += dist_len;

			MEM_CPY((out + cur), replace, r_len);
			cur += r_len;
			
			pos = idx + s_len;
		}else{
			MEM_CPY((out + cur), src, src_len);
			cur += src_len;
			break;
		}								
	}
	if(out != NULL ) *(out + cur) = 0;
	*outp = out;
	return cur;
}

/** 
 * free memory allocated  by  str_split and so on
 * @arr: array pointer
 * @size: array size
 * 
 */
int str_free(char **arr, int size){
	int i = size;
	while(--i >= 0){
		if(arr[i] != NULL) free(arr[i]);		
	}
}

/** 
 * print array(string) list
 * @arr: array pointer
 * @size: array size
 */
int str_print(char **arr, int size){
	int i = -1;
	while(++i < size){
		if(arr[i] != NULL) fprintf(stdout, "%s\n", arr[i]);
	}	
}

/**
 * trim @str remove @chr from @str
 * @str: string 
 * 	return string pointer after being trimed
 * 	NOTE: Free @return once @return  is NOT NULL after using it,
	Or there will cause memory leaking
 */
char *str_trim(char *str, char chr){
	if(str == NULL || strlen(str) == 0 ) return NULL;

	char *pos	= str;
	int size 	= strlen(str);	
	char *m 	=  (char *)calloc(1, size + 1);
	char *out 	= m;
	if(out == NULL) return NULL;

	while(size--){
		if( *pos != chr )
			*out++ = *pos++;
		else
			pos++;
	}	
	return m;
}

/**
 * trim @str remove @chr at left of @str 
 * @str: string
 * @chr: char should be removed from string
 * 	return string pointer after being trimed
 * 	NOTE: Free @return once @return  is NOT NULL after using it,
	Or there will cause memory leaking
 */
char *str_ltrim(char *str, char chr){
	if(str == NULL || strlen(str) == 0 ) return NULL;
	
	char *pos	= str;
	int size	= strlen(str);
	char *m 	= (char *)calloc(1, size + 1);
	char *out 	= m;
	if(out == NULL) return NULL;
	
	while(size--){
		if(*pos++ != chr)
			break;		
	}	
	while(size--){
		*out++ = *pos++;	
	}
	return m;
}

/**
 * trim @str remove @chr at right of @str
 * @str: string
 * @chr: char should be removed from string
 *      return string pointer after being trimed
 * 	NOTE: Free @return once @return  is NOT NULL after using it,
	Or there will cause memory leaking
 */
char *str_rtrim(char *str, char chr){
        if(str == NULL || strlen(str) == 0 ) return NULL;

        int size        = strlen(str);
        char *pos       = str;
	char *last	= str + size;
        char *m         = (char *)calloc(1, size + 1);
        char *out       = m;
        if(out == NULL) return NULL;
	
	while(size--){
		if(*--last != chr )	
			break;
	}
	while(size--){
		*out++ = *pos++;
	}
	return m;
}

/**
 * Return part of a string
 * @str: string source
 * @start: position from *@str as 0
 * @len: length required return 
 * 	return new string
 */
char *str_sub(char *str, int start, int len){
        if(str == NULL || strlen(str) == 0 ) return NULL;

	char *out = (char *)calloc(1, len + 1);	 	
	if(out == NULL) return NULL;		
	int slen = strlen(str);	
	char *nout = str;
	char *pos  = str + start;
	char *last = str + len;
	int size = len;
	while(pos < last && size--){
		*nout++ = *pos++;
	}
	return out;
}

char *preg_match(char *pattern, char *str, char **match){
		
}

char *preg_match_free(char *pattern, char *str, char **match){

}

/*
#ifdef DEBUG_TEST
int main(int argc, char **argv){
	//str_split
	//if(cs_ == NULL ) return -1;
	char *str = "abc\0\r\ndefgh\0\r\nijkl\0\r\nmnopq\0\r\nrstu\0\r\nvwxyz\0\r\n";
	char *sep = "\0\r\n";
	int  str_len = 40;
	int  sep_len = 3;
	char **arr = NULL;	
	int  arr_len = str_split(str, str_len, sep, sep_len, &arr);
	if(arr_len > 0 ){
		str_print(arr, arr_len);
		str_free(arr, arr_len);
	}

	//MEM_CMP
	char *cs = "abcd";
	char *ct = "ebcd";
	int cmp = 0;
	MEM_CMP(cmp, cs, ct, 4);
	fprintf(stdout, "campare %s:%s result %d\n", cs, ct, cmp);

	//MEM_MOVE
	//dest > src
	char *cs_ = NULL;
	MEM_DUP(cs_, "abcdefghijhlmnopqrstuvwxyz", 26);
	if(cs_ == NULL ) return -1;
	char *ct_ = cs_ + 10;
	fprintf(stdout, "dest >  src : move [%s] to [%s] ", cs_, ct_);
	MEM_MOVE(ct_, cs_, 10);
	fprintf(stdout, "result: %s\n", cs_);
	// dest <= src	
	ct_ = cs_ + 15;	
	fprintf(stdout, "dest <= src : move [%s] to [%s] ", ct_, cs_);	
	MEM_MOVE(cs_, ct_, 10);
	fprintf(stdout, "result: %s\n", cs_);
	if(cs_ != NULL )free(cs_);

	//MEM_SET
	int  len = 100;
	char *tmp = (char *)malloc(len);	
	if(tmp == NULL ) return -1;
	MEM_SET(tmp, 0, len);	
	MEM_CPY(tmp, "abcdefghijklmnopqrstuvwxyz", 26);
	fprintf(stdout, "string: %s\n", tmp);
	MEM_SET(tmp, 'x', 32);
	fprintf(stdout, "string: %s\n", tmp);
	if(tmp != NULL ) free(tmp);

 	// str_replace
	char *src = "abcdef\0ghijklmno\0pqrstuvwxyz\0";
	char *out = NULL;
	int out_len = str_replace(src, 29, "\0", 1, "\n", 1, &out);
	if(out != NULL){
		printf("replace %s in ", "\\0");
		MEM_DUMP(src, 29);	
		printf(" with %s \nresult: \n%s \n", "\\n", out);
		free(out);
	}

	// str_trim, str_ltrim, str_rtrim
	char *trim_str = "    ab c d e f g hi j k l mn o p q r s tu vw x yz  ";	
	char *str_after_trim = str_trim(trim_str, 0x20);
	char *str_after_ltrim = str_ltrim(trim_str, 0x20);
	char *str_after_rtrim = str_rtrim(trim_str, 0x20);

	printf("original string:%s\n"
		"string after  trim:%s\n"
		"string after ltrim:%s\n"
		"string after rtrim:%s\n",
		trim_str, str_after_trim,
		str_after_ltrim, str_after_rtrim);
	return 0;
}
#endif
*/
