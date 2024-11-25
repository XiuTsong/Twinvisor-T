/*
 * @Date: 2024-11-25 10:13:34
 */

#ifndef __SVISOR_STDIO_H__
#define __SVISOR_STDIO_H__

#define EOF            -1

#define __printflike(fmtarg, firstvararg) \
		__attribute__((__format__ (__printf__, fmtarg, firstvararg)))

int printf(const char *fmt, ...) __printflike(1, 2);
int snprintf(char *s, unsigned long n, const char *fmt, ...) __printflike(3, 4);

void print_lock_init(void);
int printf_error(const char *fmt, ...) __printflike(1, 2);

#ifdef STDARG_H
int vprintf(const char *fmt, va_list args);
#endif

int putchar(int c);
int puts(const char *s);

#endif
