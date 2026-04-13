/************************************************************
* File name: www.h
* History: created by Harry H. Cheng, 12/28/1995
* last change: Harry H. Cheng, 12/28/1995
************************************************************/
#ifndef _WWW_H
#define	_WWW_H

typedef char ** stringArray;
extern int getnamevalue(stringArray *name, stringArray *value);
extern void delnamevalue(stringArray name, stringArray value, int num);

#endif	/* _WWW_H */

