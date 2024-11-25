/*
 * @Description: titanium main.c
 * @Date: 2024-11-08 15:31:38
 */

#include <s-visor/s-visor.h>
#include <s-visor/mm/mm.h>
#include <s-visor/sched/sched.h>
#include <s-visor/virt/vm.h>
#include <s-visor/drivers/console.h>

#define VAL_EXTRACT_BITS(data, start, end) \
	((data >> start) & ((1ul << (end-start+1))-1))

int __secure_text init_primary_core(void)
{
	(void)_console_init(PLAT_ARM_BOOT_UART_BASE,
			   PLAT_ARM_BOOT_UART_CLK_IN_HZ, ARM_CONSOLE_BAUDRATE);

    /* Allow reading normal memory from secure world in Titanium */
//    tzc_disable_filters();
//    tzc_configure_region(1 << 0x0, 1, 0x80000000, 0xfeffffff, 0x3, 0x83038303);
//    //tzc_configure_region(1 << 0x0, 3, 0x880000000, 0x8ffffffff, 0x3, 0x83038303);
//    tzc_configure_region(1 << 0x0, 2, 0x880000000, 0x9ffffffff, 0x3, 0x83038303);
//    tzc_configure_region(0, 3, 0, 0, 0, 0);
//    tzc_enable_filters();

	/* Use virtual memory and initialize buddy and slab allocators*/
	mm_primary_init();

	/*Sched init */
	tianium_vm_sched_init();

	/* Enable virtualization function */
	init_vms();

	return 0;
}

int __secure_text init_secondary_core(void)
{
	mm_secondary_init();

	return 0;
}
