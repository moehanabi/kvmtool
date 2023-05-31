#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/ioport.h>
#include <linux/kernel.h>
#include <linux/module.h>

#define PICTEST_PIO_PORT 0xf000
#define PICTEST_PIO_LEN 0x10
#define PORT_OFFSET_DATA 0x00
#define PORT_OFFSET_IRQ 0x04

static struct resource pictest_pio_res = {
    .name = "pictest",
    .start = PICTEST_PIO_PORT,
    .end = PICTEST_PIO_PORT + PICTEST_PIO_LEN,
    .flags = IORESOURCE_MEM,
};
static void __iomem *iomem;
static int irq;

static irqreturn_t pictest_irq_handler(int irq, void *vdev) {
  /* TODO */
  printk("pictest vPIC with IRQ %d\n", irq);
  return IRQ_HANDLED;
}

static int __init pictest_init(void) {
  int r;

  r = request_resource(&ioport_resource, &pictest_pio_res);
  if (r < 0)
    return r;

  iomem = ioport_map(PICTEST_PIO_PORT, PICTEST_PIO_LEN);
  // iomem = ioremap(PICTEST_PIO_PORT, PICTEST_PIO_LEN);
  if (!iomem) {
    r = -ENOMEM;
    release_resource(&pictest_pio_res);
    return r;
  }

  irq = ioread32(iomem + PORT_OFFSET_DATA);
  printk("irq: %u\n", irq);
  r = request_irq(irq, pictest_irq_handler, IRQF_TRIGGER_HIGH, "pictest", NULL);
  if (r < 0) {
    printk("ERROR: Request IRQ failed.\n");
    ioport_unmap(iomem);
    // iounmap(iomem);
    release_resource(&pictest_pio_res);
    return r;
  }

  /* Trigger Interrupt */
  iowrite16(0x01, iomem + PORT_OFFSET_IRQ);

  return 0;
}

static void __exit pictest_exit(void) {
  free_irq(irq, NULL);
  ioport_unmap(iomem);
  // iounmap(iomem);
  release_resource(&pictest_pio_res);
}

module_init(pictest_init);
module_exit(pictest_exit);

MODULE_LICENSE("GPL");
