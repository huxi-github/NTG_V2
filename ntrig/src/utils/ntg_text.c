// Larbin
// Sebastien Ailleret
// 18-11-99 -> 10-12-01

#include "../ntg_config.h"
#include "../ntg_core.h"

/* lowercase a char */
char lowerCase(char a) {  
	if (a >= 'A' && a <= 'Z') {
		return a - 'A' + 'a';
	} else {
		return a;
	}
}

/*
 * test if b starts with a
 */
int is_startWith(char *a, char *b) {
	int i = 0;
	while (a[i] != 0) {
		if (a[i] != b[i])
			return 0;
		i++;
	}
	return 1;
}

///* test if b is forbidden by pattern a */
//bool robotsMatch (char *a, char *b) {
//  int i=0;
//  int j=0;
//  while (a[i] != 0) {
//    if (a[i] == '*') {
//      i++;
//      char *tmp = strchr(b+j, a[i]);
//      if (tmp == NULL) return false;
//      j = tmp - b;
//    } else {
//      if (a[i] != b[j]) return false;
//      i++; j++;
//    }
//  }
//  return true;
//}

/*
 *test if b starts with a ignoring case
 *注：该函数是以小写为比较基础
 */
int is_startWithIgnoreCase(char *amin, char *b) {
	int i = 0;
	while (amin[i] != 0) {
		if (amin[i] != (b[i] | 32))
			return 0;
		i++;
	}
	return 1;
}

/*
 *test if b end with a
 */
int is_endWith(char *a, char *b) {
	int la = strlen(a);
	int lb = strlen(b);
	return (la <= lb) && !strcmp(a, b + lb - la);
}

/* test if b end with a ignoring case
 * a can use min char, '.' (a[i] = a[i] | 32)
 */
int endWithIgnoreCase(char *amin, char *b) {
	int la = strlen(amin);
	int lb = strlen(b);
	if (la <= lb) {
		int i;
		int diff = lb - la;
		for (i = 0; i < la; i++) {
			if (amin[i] != (b[diff + i] | 32)) {
				return 0;
			}
		}
		return 1;
	} else {
		return 0;
	}
}

/* test if b contains a */
int  caseContain(char *a, char *b) {
	size_t la = strlen(a);
	int i = strlen(b) - la;
	while (i >= 0) {
		if (!strncasecmp(a, b + i, la)) {
			return 1;
		}
		i--;
	}
	return 0;
}
/*
 * 可用strdup代替
 */
///* create a copy of a string
// */
//char * newString(char *arg) {
//	char *res = new
//	char[strlen(arg) + 1];
//	strcpy(res, arg);
//	return res;
//}

/*
 * Read a whole file
 */
//char *readfile(int fds) {
//	ssize_t pos = 0;
//	ssize_t size = 512;
//	int cont = 1;
//	char buf[500];
//	ssize_t nbRead;
//	char *res = new
//	char[size];
//	while (cont == 1) {
//		switch (nbRead = read(fds, &buf, 500)) {
//		case 0:
//			cont = 0;
//			break;
//		case -1:
//			if (errno != EINTR && errno != EIO)
//				cont = -1;
//			break;
//		default:
//			if (pos + nbRead >= size) {
//				size *= 2;
//				char *tmp = new
//				char[size];
//				memcpy(tmp, res, pos);
//				delete res;
//				res = tmp;
//			}
//			memcpy(res + pos, buf, nbRead);
//			pos += nbRead;
//			break;
//		}
//	}
//	res[pos] = 0;
//	return res;
//}

/*
 * find the next token in the robots.txt, or in config file
 * must delete comments
 * no allocation (cf strtok); content is changed
 */
char *nextToken(char **posParse, char c) {
	// go to the beginning of next word
	int cont = 1;
	while (cont) {
		if (**posParse == c || **posParse == ' ' || **posParse == '\t'
				|| **posParse == '\r' || **posParse == '\n') {
			(*posParse)++;
		} else if (**posParse == '#') {
			*posParse = strchr(*posParse, '\n');
			if (*posParse == NULL) {
				return NULL;
			} else {
				(*posParse)++;
			}
		} else {
			cont = 0;
		}
	}
	// find the end of this word
	char *deb = *posParse;
	if (**posParse == '\"') {
		deb++;
		(*posParse)++;
		while (**posParse != 0 && **posParse != '\"')
			(*posParse)++;
	} else {
		while (**posParse != 0 && **posParse != c && **posParse != ' '
				&& **posParse != '\t' && **posParse != '\r'
				&& **posParse != '\n') {
			(*posParse)++;
		}
		if (*posParse == deb)
			return NULL; // EOF
	}
	if (**posParse != 0) {
		**posParse = 0;
		(*posParse)++;
	}
	return deb;
}

//#ifdef SPECIFICSEARCH
//
///* does this char * match privilegedExt */
//bool matchPrivExt (char *file) {
//	int i = 0;
//	int len = strlen(file);
//	while (privilegedExts[i] != NULL) {
//		if (endWithIgnoreCase(privilegedExts[i], file, len))
//		return true;
//		i++;
//	}
//	return false;
//}
//
///* does this char * match contentType */
//bool matchContentType (char *ct) {
//	int i=0;
//	while (contentTypes[i] != NULL) {
//		if (startWithIgnoreCase(contentTypes[i], ct))
//		return true;
//		i++;
//	}
//	return false;
//}
//
//#else // SPECIFICSEARCH is not defined
//
//bool matchPrivExt(char *file) {
//	return false;
//}
//bool matchContentType(char *ct) {
//	assert(false);
//	return true;
//}
//
//#endif // SPECIFICSEARCH

// end of text.cc
