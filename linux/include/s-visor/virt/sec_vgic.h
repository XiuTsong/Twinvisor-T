/*
 * @Description: titanium sel1_vgic.h
 * @Date: 2024-11-25 19:07:58
 */

#ifndef __SVISOR_SEC_VGIC_H__
#define __SVISOR_SEC_VGIC_H__

#include <s-visor/common/list.h>
#include <s-visor/lib/stdint.h>

struct titanium_irq {
	int irqnr;
	struct list_head node;
};

struct titanium_vgic {
	struct list_head wait_list;
};

#define VGIC_V3_MAX_LRS 16

static inline void vgic_init(struct titanium_vgic *vgic)
{
	list_init(&vgic->wait_list);
}

void vgic_add_lr_list(struct titanium_vgic *vgic, uint64_t *lr_list);
int vgic_inject_irq(struct titanium_vgic *vgic);

#endif
