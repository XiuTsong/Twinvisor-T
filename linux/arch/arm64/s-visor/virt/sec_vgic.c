/*
 * @Description: titanium sel1_vgic.c
 * @Date: 2024-11-25 19:07:50
 */

#include <s-visor/s-visor.h>
#include <s-visor/common/macro.h>
#include <s-visor/mm/buddy_allocator.h>
#include <s-visor/virt/sec_vgic.h>

__secure_text
static bool vgic_irq_empty(struct titanium_vgic *vgic)
{
	return list_empty(&vgic->wait_list);
}

__secure_text
static void vgic_add_irqnr(struct titanium_vgic *vgic, int irqnr)
{
	struct titanium_irq *irq = (struct titanium_irq *)bd_alloc(sizeof(struct titanium_irq), 0);

	irq->irqnr = irqnr;
	list_init(&irq->node);
	list_append(&vgic->wait_list, &irq->node);
}

__secure_text
static inline void vgic_del_irq(struct titanium_vgic *vgic, struct titanium_irq *irq)
{
	list_remove(&irq->node);
	bd_free(irq);
}

__secure_text
void vgic_add_lr_list(struct titanium_vgic *vgic, uint64_t *lr_list)
{
	uint32_t i;
	uint64_t lr;
	int irqnr;

	for (i = 0U; i < VGIC_V3_MAX_LRS; ++i) {
		lr = lr_list[i];
		if (lr != 0) {
			irqnr = lr & 0xffff;
			vgic_add_irqnr(vgic, irqnr);
		}
		lr_list[i] = 0;
	}
}

__secure_text
int vgic_inject_irq(struct titanium_vgic *vgic)
{
	struct titanium_irq *irq;
	int ret;

	if (vgic_irq_empty(vgic)) {
		return -1;
	}
	irq = _container_of(vgic->wait_list.next, struct titanium_irq, node);
	ret = irq->irqnr;
	vgic_del_irq(vgic, irq);

	return ret;
}
