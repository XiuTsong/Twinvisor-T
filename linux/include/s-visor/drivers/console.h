/*
 * Copyright (c) 2013-2018, ARM Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef SVISOR_CONSOLE_H
#define SVISOR_CONSOLE_H

#include <s-visor/lib/utils_def.h>

#define CONSOLE_T_NEXT			(U(0) * REGSZ)
#define CONSOLE_T_FLAGS			(U(1) * REGSZ)
#define CONSOLE_T_PUTC			(U(2) * REGSZ)
#define CONSOLE_T_GETC			(U(3) * REGSZ)
#define CONSOLE_T_FLUSH			(U(4) * REGSZ)
#define CONSOLE_T_DRVDATA		(U(5) * REGSZ)

#define CONSOLE_FLAG_BOOT		(U(1) << 0)
#define CONSOLE_FLAG_RUNTIME		(U(1) << 1)
#define CONSOLE_FLAG_CRASH		(U(1) << 2)
/* Bits 3 to 7 reserved for additional scopes in future expansion. */
#define CONSOLE_FLAG_SCOPE_MASK		((U(1) << 8) - 1)
/* Bits 8 to 31 reserved for non-scope use in future expansion. */

/* Returned by getc callbacks when receive FIFO is empty. */
#define ERROR_NO_PENDING_CHAR		(-1)
/* Returned by console_xxx() if no registered console implements xxx. */
#define ERROR_NO_VALID_CONSOLE		(-128)

/* Platform specific */
#define PLAT_ARM_BOOT_UART_BASE		0x09000000
#define PLAT_ARM_BOOT_UART_CLK_IN_HZ  1

#define PLAT_ARM_BL31_RUN_UART_BASE	0x09040000
#define PLAT_ARM_BL31_RUN_UART_CLK_IN_HZ	1
#define ARM_CONSOLE_BAUDRATE		115200

#ifndef __ASSEMBLY__

#include <s-visor/lib/stdint.h>

typedef struct _console {
	struct _console *next;
	/*
	 * Only the low 32 bits are used. The type is u_register_t to align the
	 * fields of the struct to 64 bits in AArch64 and 32 bits in AArch32
	 */
	u_register_t flags;
	int (*const putc)(int character, struct _console *console);
	int (*const getc)(struct _console *console);
	int (*const flush)(struct _console *console);
	/* Additional private driver data may follow here. */
} _console_t;

/* offset macro assertions for console_t */
#include <s-visor/drivers/console_assertions.h>

/*
 * NOTE: There is no publicly accessible console_register() function. Consoles
 * are registered by directly calling the register function of a specific
 * implementation, e.g. console_16550_register() from <uart_16550.h>. Consoles
 * registered that way can be unregistered/reconfigured with below functions.
 */
/* Remove a single console_t instance from the console list. Return a pointer to
 * the console that was removed if it was found, or NULL if not. */
_console_t *_console_unregister(_console_t *console);
/* Returns 1 if this console is already registered, 0 if not */
int _console_is_registered(_console_t *console);
/*
 * Set scope mask of a console that determines in what states it is active.
 * By default they are registered with (CONSOLE_FLAG_BOOT|CONSOLE_FLAG_CRASH).
 */
void _console_set_scope(_console_t *console, unsigned int scope);

/* Switch to a new global console state (CONSOLE_FLAG_BOOT/RUNTIME/CRASH). */
void _console_switch_state(unsigned int new_state);
/* Output a character on all consoles registered for the current state. */
int _console_putc(int c);
/* Read a character (blocking) from any console registered for current state. */
int _console_getc(void);
/* Flush all consoles registered for the current state. */
int _console_flush(void);

/* REMOVED on AArch64 -- use console_<driver>_register() instead! */
int _console_init(uintptr_t base_addr,
		 unsigned int uart_clk, unsigned int baud_rate);
void _console_uninit(void);

#endif /* __ASSEMBLY__ */

#endif /* SVISOR_CONSOLE_H */
