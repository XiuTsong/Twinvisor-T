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

#endif /* SVISOR_CONSOLE_H */
