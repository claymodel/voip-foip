/* Compatibility functions for strsep and strtoq missing on Solaris */
#ifdef HAVE_CONFIG_H
#include "confdefs.h"
#endif

#include <sys/types.h>
#include <stdio.h>

#ifndef HAVE_STRSEP
char* strsep(char** str, const char* delims)
{
    char* token;

    if (*str==NULL) {
        /* No more tokens */
        return NULL;
    }

    token=*str;
    while (**str!='\0') {
        if (strchr(delims,**str)!=NULL) {
            **str='\0';
            (*str)++;
            return token;
        }
        (*str)++;
    }
    /* There is no other token */
    *str=NULL;
    return token;
}
#endif


#ifndef HAVE_SETENV
int setenv(const char *name, const char *value, int overwrite)
{
	unsigned char *buf;
	int buflen, ret;

	buflen = strlen(name) + strlen(value) + 2;
	if ((buf = malloc(buflen)) == NULL)
 		return -1;

	if (!overwrite && getenv(name))
		return 0;

	snprintf(buf, buflen, "%s=%s", name, value);
	ret = putenv(buf);

	free(buf);

	return ret;
}
#endif

#ifndef HAVE_UNSETENV
int unsetenv(const char *name)
{
  setenv(name,"",0);
}
#endif

