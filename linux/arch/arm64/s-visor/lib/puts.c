/*
 * @Date: 2024-11-25 10:18:29
 */

#include <s-visor/s-visor.h>
#include <s-visor/lib/stdio.h>

int __secure_text puts(const char *s)
{
	int count = 0;

	while (*s != '\0') {
		if (putchar(*s) == EOF)
			return EOF;
		s++;
		count++;
	}

	if (putchar('\n') == EOF)
		return EOF;

	return count + 1;
}
