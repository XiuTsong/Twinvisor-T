/*
 * @Description: titanium sel1_guest.h
 * @Date: 2024-11-25 19:27:51
 */

#ifndef __SVISOR_SEC_GUEST_H__
#define __SVISOR_SEC_GUEST_H__

#include <s-visor/lib/stdint.h>

/*
 * PSR bits
 */
#define PSR_MODE_EL0t	0x00000000
#define PSR_MODE_EL1t	0x00000004
#define PSR_MODE_EL1h	0x00000005
#define PSR_MODE_EL2t	0x00000008
#define PSR_MODE_EL2h	0x00000009
#define PSR_MODE_EL3t	0x0000000c
#define PSR_MODE_EL3h	0x0000000d
#define PSR_MODE_MASK	0x0000000f

/* AArch64 SPSR bits */
#define PSR_F_BIT	0x00000040
#define PSR_I_BIT	0x00000080
#define PSR_A_BIT	0x00000100
#define PSR_D_BIT	0x00000200
#define PSR_PAN_BIT	0x00400000
#define PSR_UAO_BIT	0x00800000
#define PSR_V_BIT	0x10000000
#define PSR_C_BIT	0x20000000
#define PSR_Z_BIT	0x40000000
#define PSR_N_BIT	0x80000000

/*
 * Groups of PSR bits
 */
#define PSR_f		0xff000000	/* Flags		*/
#define PSR_s		0x00ff0000	/* Status		*/
#define PSR_x		0x0000ff00	/* Extension		*/
#define PSR_c		0x000000ff	/* Control		*/

#define user_mode(spsr)	\
	((spsr & PSR_MODE_MASK) == PSR_MODE_EL0t)

extern void set_guest_sctlr(u_register_t val);
extern u_register_t get_guest_spsr(void);

static inline bool is_guest_user_mode(void)
{
	u_register_t spsr = get_guest_spsr();
	return user_mode(spsr);
}

static inline bool is_guest_local_irq_disable(void)
{
	u_register_t spsr = get_guest_spsr();
	return spsr & PSR_I_BIT;
}

static inline bool is_guest_local_irq_enable(void)
{
	return !is_guest_local_irq_disable();
}

#endif
