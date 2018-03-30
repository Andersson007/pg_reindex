#ifndef LOGGING_H
#define LOGGING_H

#define TS_BUFSIZE 24

#define INF 0
#define WRN 1
#define ERR 2

char* get_now_time(void);
void log_write(FILE *log, int lvl_code, char* fmt,...);

#endif
