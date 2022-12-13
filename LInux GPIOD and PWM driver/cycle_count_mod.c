/*****************************
*
*  An example Linux loadable module to enable ARM cycle counter on each cpu
*
***********************************/

#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
//#include <linux/time.h>
//#include <linux/timekeeping.h>
//#include <linux/delay.h>
MODULE_LICENSE("GPL");

void enable_ccr(void *info) {
  // Set the User Enable register, bit 0
  asm volatile ("mcr p15, 0, %0, c9, c14, 0" :: "r" (1));
  // Enable all counters in the PNMC control-register
  asm volatile ("MCR p15, 0, %0, c9, c12, 0\t\n" :: "r"(1));
  // Enable cycle counter specifically
  // bit 31: enable cycle counter
  // bits 0-3: enable performance counters 0-3
  asm volatile ("MCR p15, 0, %0, c9, c12, 1\t\n" :: "r"(0x80000000));
}

static int cycle_count_init(void)
{

    printk(KERN_ALERT "Welcome  to ARM cycle counter!!!\n");
    
    on_each_cpu(enable_ccr,NULL,0);
  
    return 0;
}

static void cycle_count_exit(void)
{
    printk(KERN_ALERT "Goodbye\n");
}

module_init(cycle_count_init);
module_exit(cycle_count_exit);
