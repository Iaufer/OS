#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/version.h>

#define FIB_COUNT 10


static void fib(int major, int minor){
    int next;

    printk(KERN_INFO "%d ", minor);
    printk(KERN_INFO "%d ", major);

    for(int i = 2; i < FIB_COUNT; i++){
        next = minor + major;
        printk(KERN_INFO "%d ", next);
        minor = major;
        major = next;
    }
}

static int __init hello_init(void) {

    int major = LINUX_VERSION_MAJOR;
    int minor = LINUX_VERSION_PATCHLEVEL;

    printk(KERN_INFO "Mодуль ядра начинает работу.\n");

    printk(KERN_INFO "Kernel version: %d.%d\n", major, minor);

    fib(major, minor);
    

    return 0;
}

static void __exit hello_exit(void) {
    printk(KERN_INFO "Mодуль ядра завершает работу.\n");
}

module_init(hello_init);
module_exit(hello_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Artyom");
MODULE_DESCRIPTION("Пример модуля ядра Linux");
MODULE_VERSION("1.0");
