/*
 * @Description: titanium sel1_emulate.c
 * @Date: 2024-11-25 18:50:43
 */
#include <asm/esr.h>
#include <asm/sysreg.h>

#include <s-visor/s-visor.h>
#include <s-visor/arch/arm64/arch.h>
#include <s-visor/lib/stdio.h>
#include <s-visor/lib/stdint.h>
#include <s-visor/mm/sec_shm.h>
#include <s-visor/mm/sec_mmu.h>
#include <s-visor/virt/sec_emulate.h>
#include <s-visor/virt/sec_defs.h>

__secure_text
static inline int get_rt_from_esr(u_register_t esr)
{
	return (esr & ESR_ELx_SYS64_ISS_RT_MASK) >> ESR_ELx_SYS64_ISS_RT_SHIFT;
}

__secure_text
static void vgic_v3_emulate_sgi(struct titanium_state *state, u_register_t Rt_val)
{
	u_register_t esr;
	unsigned long Rt;

	/*
		* sgi group1, Rt = 2
		* FIXME: hard code here
		*/
	esr = 0x623a3056;
	Rt = get_rt_from_esr(esr);
	write_sysreg(esr, esr_el1);
	emulate_sys_reg_start(state, Rt, Rt_val);
}

__secure_text
void emulate_access_trap(struct titanium_state *state, uint32_t core_id, uint32_t vcpu_id)
{
	int exit_id;
	u_register_t arg1;
	struct sec_shm *shared_mem = NULL;;

	shared_mem = get_shared_mem(state->current_vm, core_id);
	exit_id = shared_mem->exit_reason;
	arg1 = shared_mem->arg1;
	switch (exit_id) {
	case SEL1_WRITE_ICC_SGI1R_EL1:
		vgic_v3_emulate_sgi(state, arg1);
		break;
	default:
		printf("Unsupported access trap: %d\n", exit_id);
		break;
	}
}

__secure_text
void emulate_sys_reg_start(struct titanium_state *state, unsigned long Rt, unsigned long Rt_val)
{
	struct titanium_sys_reg_emulate *emulate = &state->emulate;
	unsigned long orig_val = get_guest_gp_reg(state, Rt);

	emulate->is_emulating = true;
	emulate->Rt = Rt;
	emulate->orig_val = orig_val;
	set_guest_gp_reg(state, Rt, Rt_val);
}

__secure_text
static inline void revert_kvm_skip_instr(struct titanium_state *state)
{
	unsigned long elr_el1 = get_guest_sys_reg(state, elr);

	elr_el1 -= 4;
	set_guest_sys_reg(state, elr, elr_el1);
}

__secure_text
static inline void restore_guest_rt(struct titanium_state *state)
{
	struct titanium_sys_reg_emulate *emulate = &state->emulate;
	unsigned int Rt;
	unsigned long orig_val;

	Rt = emulate->Rt;
	orig_val = emulate->orig_val;
	set_guest_gp_reg(state, Rt, orig_val);
}

__secure_text
void emulate_sys_reg_finish(struct titanium_state *state)
{
	struct titanium_sys_reg_emulate *emulate = &state->emulate;

	if (!emulate->is_emulating) {
		return;
	}
	restore_guest_rt(state);
	revert_kvm_skip_instr(state);
	emulate->is_emulating = false;
}

/* Decode instruction: str/ldr Rd, [Rn]
 * BIT[31:30]: size
 * BIT[23:22]: opc
 * BIT[12]: interger shift
 */
__secure_text
static void arch_decode(uint32_t arm64_code, struct titanium_decode *kvm_decode, int *is_write, int *size) {
	uint32_t n1 = arm64_code & 0xf;
	uint32_t n2 = (arm64_code >> 4) & 0xf;
	uint8_t opc = (arm64_code >> 22) & 0b11;

	kvm_decode->rt = n1 + ((n2 & 0x1) << 4);
	kvm_decode->sign_extend = arm64_code & BIT(12);

	*size = (arm64_code >> 30) & 0b11;
	if (opc == 0b01) {
		*is_write = 0;
	}
	else {
		*is_write = 1;
	}
	if (*size == 0b11) {
		kvm_decode->sixty_four = 1;
	}
	else {
		kvm_decode->sixty_four = 0;
	}
}

__secure_text
static void emulate_mmio_esr(struct titanium_state *state, unsigned long *esr_el1, uint64_t vcpu_id)
{
	struct titanium_vcpu *vcpu = state->current_vm->vcpus[vcpu_id];
	vaddr_t fault_pc = get_guest_sys_reg(state, elr);
	s1_ptp_t *ttbr;
	paddr_t fault_pc_pa;
	uint32_t arm64_code;
	s1_pte_t entry;
	struct titanium_decode kvm_decode;
	int size;
	int is_write;
	unsigned long esr_el = 0;
	unsigned long ec = ESR_ELx_EC_DABT_LOW;

	if (fault_pc & BIT(63)) {
		ttbr = (s1_ptp_t *)get_shadow_ttbr1(vcpu->s1mmu);
	} else {
		ttbr = (s1_ptp_t *)get_shadow_ttbr0(vcpu->s1mmu);
	}
	entry = translate_spt(ttbr, (fault_pc >> PAGE_SHIFT));
	if (!entry.l3_page.is_valid) {
		printf("Decode instruction failed, fault addr: %lx, ttbr: %p\n", fault_pc, ttbr);
		return;
	}
	fault_pc_pa = ((uint64_t)entry.l3_page.pfn << PAGE_SHIFT) | (fault_pc & PAGE_MASK_INV);

	arm64_code = *((uint32_t*)phys_to_virt(fault_pc_pa));
	arch_decode(arm64_code, &kvm_decode, &is_write, &size);

	if (is_write) {
		esr_el |= ESR_ELx_WNR;
	}

	if (kvm_decode.sign_extend) {
		esr_el |= ESR_ELx_SSE;
	}

	if (kvm_decode.sixty_four) {
		esr_el |= ESR_ELx_SF;
	}

	esr_el |= (kvm_decode.rt << ESR_ELx_SRT_SHIFT) & ESR_ELx_SRT_MASK;
	esr_el |= (size << ESR_ELx_SAS_SHIFT) & ESR_ELx_SAS;
	esr_el |= ESR_ELx_ISV;
	esr_el |= ESR_ELx_IL;
	esr_el |= ec << ESR_EC_SHIFT;
	/* FIXME: DFSC field */
	esr_el |= 0x7;

	*esr_el1 = esr_el;

	return;
}

__secure_text
int emulate_mmio_fault(struct titanium_state *state, unsigned long fault_ipa, uint64_t vcpu_id)
{
	struct titanium_vm *vm = state->current_vm;
	struct titanium_vcpu *vcpu = vm->vcpus[vcpu_id];
	uint64_t real_fault_ipa;
	s1_pte_t entry;
	unsigned long esr;

	if (!(fault_ipa & BIT(63)))
		return 0;

	entry = translate_spt((s1_ptp_t *)get_shadow_ttbr1(vcpu->s1mmu), fault_ipa >> PAGE_SHIFT);

	/* Maybe we can judge this by l3_page.AttrIndx */
	if (entry.l3_page.is_valid != 0 ||
		entry.l3_page.ignored != 1)
		return 0;

	/* fault_ipa is actually gva due to is_valid bit set to zero.
		* See sel1_mmu.c: set_spt_l3_entry().
		* We should convert it to ipa and fill it into FAR_EL1.
		*/
	real_fault_ipa = ((uint64_t)entry.l3_page.pfn << PAGE_SHIFT) | (fault_ipa & PAGE_MASK_INV);
	write_sysreg(real_fault_ipa, far_el1);

	/* Construct esr_el1 */
	emulate_mmio_esr(state, &esr, vcpu_id);
	write_sysreg(esr, esr_el1);

	return 1;
}
