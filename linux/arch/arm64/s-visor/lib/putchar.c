/*
 * @Date: 2024-11-25 10:02:19
 */

#include <s-visor/s-visor.h>
#include <s-visor/drivers/console.h>

#define EOF		-1

int __secure_text putchar(int c)
{
	int res;
	if (_console_putc((unsigned char)c) >= 0)
		res = c;
	else
		res = EOF;

	return res;
}
