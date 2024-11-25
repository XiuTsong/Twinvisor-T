/*
 * Description: titanium deprecated_console.S
 * Date: 2024-11-18 20:33:12
 */

#include <s-visor/common/asm_macros_common_gobiidae_spec.S>

	.globl	_console_init
	.globl	_console_uninit
	.globl	_console_putc
	.globl	_console_getc
	.globl	_console_flush

	/*
	 *  The console base is in the data section and not in .bss
	 *  even though it is zero-init. In particular, this allows
	 *  the console functions to start using this variable before
	 *  the runtime memory is initialized for images which do not
	 *  need to copy the .data section from ROM to RAM.
	 */

	.pushsection ".svisor.data", "aw"

	.align 3
	console_base: .quad 0x0

	.popsection

	.pushsection ".svisor.text", "ax"

	/* -----------------------------------------------
	 * int console_init(uintptr_t base_addr,
	 * unsigned int uart_clk, unsigned int baud_rate)
	 * Function to initialize the console without a
	 * C Runtime to print debug information. It saves
	 * the console base to the data section.
	 * In: x0 - console base address
	 *     w1 - Uart clock in Hz
	 *     w2 - Baud rate
	 * out: return 1 on success else 0 on error
	 * Clobber list : x1 - x4
	 * -----------------------------------------------
	 */
func _console_init
	/* Check the input base address */
	cbz	x0, init_fail
	adrp	x3, console_base
	str	x0, [x3, :lo12:console_base]
	// b	console_core_init
init_fail:
	ret
endfunc _console_init

	/* -----------------------------------------------
	 * void console_uninit(void)
	 * Function to finish the use of console driver.
	 * It sets the console_base as NULL so that any
	 * further invocation of `console_putc` or
	 * `console_getc` APIs would return error.
	 * -----------------------------------------------
	 */
func _console_uninit
	mov	x0, #0
	adrp	x3, console_base
	str	x0, [x3, :lo12:console_base]
	ret
endfunc _console_uninit

	/* ---------------------------------------------
	 * int console_putc(int c)
	 * Function to output a character over the
	 * console. It returns the character printed on
	 * success or -1 on error.
	 * In : x0 - character to be printed
	 * Out : return -1 on error else return character.
	 * Clobber list : x1, x2
	 * ---------------------------------------------
	 */
func _console_putc
	adrp	x2, console_base
	ldr	x1, [x2, :lo12:console_base]
	b	console_core_putc
endfunc _console_putc

	/* ---------------------------------------------
	 * int console_getc(void)
	 * Function to get a character from the console.
	 * It returns the character grabbed on success
	 * or -1 on error.
	 * Clobber list : x0, x1
	 * ---------------------------------------------
	 */
func _console_getc
	adrp	x1, console_base
	ldr	x0, [x1, :lo12:console_base]
	b	console_core_getc
endfunc _console_getc

	/* ---------------------------------------------
	 * int console_flush(void)
	 * Function to force a write of all buffered
	 * data that hasn't been output. It returns 0
	 * upon successful completion, otherwise it
	 * returns -1.
	 * Clobber list : x0, x1
	 * ---------------------------------------------
	 */
func _console_flush
	adrp	x1, console_base
	ldr	x0, [x1, :lo12:console_base]
	b	console_core_flush
endfunc _console_flush

	.popsection
