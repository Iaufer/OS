#include<linux/kernel.h>
#include<linux/module.h>
#include<linux/printk.h> 
#include<linux/proc_fs.h>
#include<linux/uaccess.h>
#include<linux/version.h>

#define procfs_name "tsulab"
static struct proc_dir_entry *our_proc_file = NULL;

#define FIB_COUNT 10

static void fib(int major, int minor, char *buffer, size_t buffer_length) {
    int next;
    size_t total_length = 0;

    total_length += snprintf(buffer + total_length, buffer_length - total_length, "%d %d ", minor, major);

    for (int i = 2; i < FIB_COUNT; i++) {
        next = minor + major;
        total_length += snprintf(buffer + total_length, buffer_length - total_length, "%d ", next);
        minor = major;
        major = next;
    }
}

static ssize_t procfile_read(struct file *file_pointer, char __user *buffer, size_t buffer_length, loff_t *offset) {
    int major = LINUX_VERSION_MAJOR;
    int minor = LINUX_VERSION_PATCHLEVEL;

    char fib_buffer[256]; 
    fib(major, minor, fib_buffer, sizeof(fib_buffer));

    if (*offset >= strlen(fib_buffer) || copy_to_user(buffer, fib_buffer, strlen(fib_buffer))) {
        return 0;
    } else {
        pr_info("Fibonacci sequence generated and written to the file: %s\n", file_pointer->f_path.dentry->d_name.name);
        *offset = strlen(fib_buffer);
        return *offset;
    }
}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 6, 0)
static const struct proc_ops proc_file_fops = {
    .proc_read = procfile_read,
};
#else
static const struct file_operations proc_file_fops = {
    .read = procfile_read,
};
#endif

static int __init procfs1_init(void) {
    our_proc_file = proc_create(procfs_name, 0644, NULL, &proc_file_fops);
    if (NULL == our_proc_file) { 
        pr_alert("Error:Could not initialize /proc/%s\n", procfs_name); 
        return -ENOMEM; 
    } 
 
    pr_info("proc %s was created\n", procfs_name);
    return 0;
}

static void __exit procfs1_exit(void) {
    proc_remove(our_proc_file);
    pr_info("proc %s was removed\n", procfs_name);
}

module_init(procfs1_init);
module_exit(procfs1_exit);
MODULE_LICENSE("GPL");
