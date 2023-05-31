#include "kvm/ioport.h"
#include "kvm/irq.h"
#include "kvm/kvm.h"
#include <assert.h>
#include <linux/types.h>

#define PICTEST_PIO_PORT 0xf000
#define PICTEST_PIO_LEN 0x10
#define PORT_OFFSET_DATA 0x00
#define PORT_OFFSET_IRQ 0x04
#define IRQ_LOW 0
#define IRQ_HIGH 1

static int irq;

static void pictest_handler(struct kvm_cpu *vcpu, u64 addr, u8 *data, u32 len,
                            u8 is_write, void *ptr) {
  u64 port_offset = addr - PICTEST_PIO_PORT;

  switch (port_offset) {
  case PORT_OFFSET_DATA:
    assert(len == 4);
    assert(!is_write);
    ioport__write32((void *)data, irq);
    break;
  case PORT_OFFSET_IRQ:
    kvm__irq_line(vcpu->kvm, irq, IRQ_HIGH);
    break;
  }
}

static int pictest__init(struct kvm *kvm) {
  int r;

  irq = irq__alloc_line(); // alloc irq number
  printf("irq:%d\n", irq);
  kvm__irq_line(kvm, irq, IRQ_LOW); // initialize low level

  r = kvm__register_pio(kvm, PICTEST_PIO_PORT, PICTEST_PIO_LEN, pictest_handler,
                        NULL);
  if (r < 0)
    return r;

  return 0;
}
dev_init(pictest__init);

static int pictest__exit(struct kvm *kvm) {
  kvm__deregister_pio(kvm, PICTEST_PIO_PORT);

  return 0;
}
dev_exit(pictest__exit);
