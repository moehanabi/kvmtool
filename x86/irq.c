#include "kvm/irq.h"
#include "kvm/kvm.h"
#include "kvm/util.h"
#include "kvm/i8259.h"

#include <linux/types.h>
#include <linux/rbtree.h>
#include <linux/list.h>
#include <linux/kvm.h>
#include <sys/ioctl.h>

#include <stddef.h>
#include <stdlib.h>

#define IRQCHIP_MASTER			0
#define IRQCHIP_SLAVE			1
#define IRQCHIP_IOAPIC			2

bool irqchip_split(struct kvm *kvm)
{
	return kvm->cfg.irqchip_split;
}

static int irq__add_routing(u32 gsi, u32 type, u32 irqchip, u32 pin)
{
	int r = irq__allocate_routing_entry();
	if (r)
		return r;

	irq_routing->entries[irq_routing->nr++] =
		(struct kvm_irq_routing_entry) {
			.gsi = gsi,
			.type = type,
			.u.irqchip.irqchip = irqchip,
			.u.irqchip.pin = pin,
		};

	return 0;
}

static int irq__add_msi_routing(u32 gsi, u32 type, u64 addr, u32 data)
{
	int r = irq__allocate_routing_entry();
	if (r)
		return r;

	irq_routing->entries[irq_routing->nr++] =
		(struct kvm_irq_routing_entry) {
			.gsi = gsi,
			.type = type,
			.u.msi.address_lo = (__u32)addr,
			.u.msi.address_hi = addr >> 32,
			.u.msi.data = data,
		};

	return 0;
}

static int irq__init_kernel(struct kvm *kvm)
{
	int i, r;

	/* Hook first 8 GSIs to master IRQCHIP */
	for (i = 0; i < 8; i++)
		if (i != 2)
			irq__add_routing(i, KVM_IRQ_ROUTING_IRQCHIP, IRQCHIP_MASTER, i);

	/* Hook next 8 GSIs to slave IRQCHIP */
	for (i = 8; i < 16; i++)
		irq__add_routing(i, KVM_IRQ_ROUTING_IRQCHIP, IRQCHIP_SLAVE, i - 8);

	/* Last but not least, IOAPIC */
	for (i = 0; i < 24; i++) {
		if (i == 0)
			irq__add_routing(i, KVM_IRQ_ROUTING_IRQCHIP, IRQCHIP_IOAPIC, 2);
		else if (i != 2)
			irq__add_routing(i, KVM_IRQ_ROUTING_IRQCHIP, IRQCHIP_IOAPIC, i);
	}

	r = ioctl(kvm->vm_fd, KVM_SET_GSI_ROUTING, irq_routing);
	if (r) {
		free(irq_routing);
		return errno;
	}

	next_gsi = i;

	return 0;
}

static int irq__init_split(struct kvm *kvm)
{
	int i, r;

	/* Hook first 8 GSIs to master IRQCHIP */
	for (i = 0; i < 8; i++)
		irq__add_msi_routing(i, KVM_IRQ_ROUTING_MSI, 0, 0);

	/* Hook next 8 GSIs to slave IRQCHIP */
	for (i = 8; i < 16; i++)
		irq__add_msi_routing(i, KVM_IRQ_ROUTING_MSI, 0, 0);

	r = ioctl(kvm->vm_fd, KVM_SET_GSI_ROUTING, irq_routing);
	if (r) {
		free(irq_routing);
		return errno;
	}

	next_gsi = i;

	return kvm_pic_init(kvm);
}

int irq__init(struct kvm *kvm)
{
	if (irqchip_split(kvm))
		return irq__init_split(kvm);
	else
		return irq__init_kernel(kvm);
}
dev_base_init(irq__init);

int irq__exit(struct kvm *kvm)
{
	if (irqchip_split(kvm))
		kvm_pic_destroy(kvm);
	return 0;
}
dev_base_exit(irq__exit);
