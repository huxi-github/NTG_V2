// Larbin
// Sebastien Ailleret
// 18-11-99 -> 21-05-01

#ifndef TEXT_H
#define TEXT_H

#include "../ntg_config.h"
#include "../ntg_core.h"

/* lowercase a char */
char lowerCase (char a);

/* tests if b starts with a */
int  startWith (char *a, char *b);

/* test if b is forbidden by pattern a */
int robotsMatch (char *a, char *b);

/* tests if b starts with a ignoring case */
int is_startWithIgnoreCase(char *amin, char *b);

/* test if b end with a */
int  endWith (char *a, char *b);

/* test if b end with a ignoring case
 * a can use min char, '.' (a[i] = a[i] | 32)
 */
int endWithIgnoreCase(char *amin, char *b);

/* tests if b contains a */
int  caseContain (char *a, char *b);

/* create a copy of a string */
char *newString (char *arg);

/* Read a whole file
 */
char *readfile (int fds);

/* find the next token in the robots.txt, or in config file
 * must delete comments
 * no allocation (cf strtok); content is changed
 * param c is a bad hack for using the same function for robots and configfile
 */
//char *nextToken(char **posParse, char c=' ');

/* does this char * match privilegedExt */
int  matchPrivExt(char *file);

/* does this char * match contentType */
int  matchContentType(char *ct);

#endif // TEXT_H
