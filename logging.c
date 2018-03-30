#include <stdarg.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include "headers/logging.h"


char* get_now_time(void)
{
	time_t rawtime = time(NULL);
	struct tm* t_info = localtime(&rawtime);
	char* t_stamp;
	char* format = "%Y/%m/%d %H:%M:%S";

	t_stamp = (char*)malloc(TS_BUFSIZE * sizeof(char) + 1);

	strftime(t_stamp, TS_BUFSIZE, format, t_info);

	return t_stamp;
}


void log_write(FILE* fp, int lvl_code, char *fmt,...)
{
	va_list list;
	char *p, *r, *t_stamp, *lvl;
	int e;

	if (!fp) {
		printf("The passed file pointer to log_write() is NULL\n");
		exit(1);
	}

	switch (lvl_code) {
		case INF :
			lvl = "INFO";
			break;
		case WRN :
			lvl = "WARNING";
			break;
		case ERR :
			lvl = "ERROR";
			break;
		default :
			lvl = "INFO";
			break;
	}

	t_stamp = get_now_time();     
	fprintf(fp,"%s [%s] ", t_stamp, lvl);
	va_start(list, fmt);
 
	for (p = fmt ; *p; ++p) {
		if (*p != '%')
			fputc(*p, fp);
		else {
			switch (*++p) {
				case 's' :
					r = va_arg(list, char *);

					fprintf(fp, "%s", r);
					continue;

				case 'd' :
					e = va_arg(list, int);
 
					fprintf(fp, "%d", e);
					continue;
                
				default:
					fputc(*p, fp);
			}
		}
	}

	va_end(list);
	fflush(fp);
	free(t_stamp);
}
