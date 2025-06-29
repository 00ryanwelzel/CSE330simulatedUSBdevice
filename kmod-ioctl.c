#include <linux/blkdev.h>
#include <linux/uaccess.h>
#include <linux/vmalloc.h>
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/module.h>
#include <linux/cdev.h>
#include <linux/version.h>
#include <linux/ioctl.h>

#include "/home/ryancse330/Downloads/CSE330-Summer-2025-project-5/ioctl-defines.h"
//#include "kmod-ioctl.h"
//#include "../ioctl-defines.h"

static dev_t 		dev = 0;
static struct class* 	kmod_class;
static struct cdev 	kmod_cdev;

struct block_rw_ops rw_request;
struct block_rwoffset_ops rwoffset_request;

//bool kmod_ioctl_init(void);
//void kmow_ioctl_teardown(void);

extern long rw_usb(char* data, unsigned int size, unsigned int offset, bool is_write);

static long kmod_ioctl(struct file *f, unsigned int cmd, unsigned long arg) {
    char* kernbuf;

    switch (cmd) {
        case BREAD:
        case BWRITE:
            if (copy_from_user(&rw_request, (void *)arg, sizeof(struct block_rw_ops))) {
                return -EFAULT;
            }
            kernbuf = vmalloc(rw_request.size);
            if (!kernbuf)
                return -ENOMEM;

            if (cmd == BWRITE) {
                if (copy_from_user(kernbuf, rw_request.data, rw_request.size)) {
                    vfree(kernbuf);
                    return -EFAULT;
                }
                rw_usb(kernbuf, rw_request.size, -1, true);
            } else {
                rw_usb(kernbuf, rw_request.size, -1, false);
                if (copy_to_user(rw_request.data, kernbuf, rw_request.size)) {
                    vfree(kernbuf);
                    return -EFAULT;
                }
            }
            vfree(kernbuf);
            return 0;

        case BREADOFFSET:
        case BWRITEOFFSET:
            if (copy_from_user(&rwoffset_request, (void *)arg, sizeof(struct block_rwoffset_ops))) {
                return -EFAULT;
            }
            kernbuf = vmalloc(rwoffset_request.size);
            if (!kernbuf)
                return -ENOMEM;

            if (cmd == BWRITEOFFSET) {
                if (copy_from_user(kernbuf, rwoffset_request.data, rwoffset_request.size)) {
                    vfree(kernbuf);
                    return -EFAULT;
                }
                rw_usb(kernbuf, rwoffset_request.size, rwoffset_request.offset, true);
            } else {
                rw_usb(kernbuf, rwoffset_request.size, rwoffset_request.offset, false);
                if (copy_to_user(rwoffset_request.data, kernbuf, rwoffset_request.size)) {
                    vfree(kernbuf);
                    return -EFAULT;
                }
            }
            vfree(kernbuf);
            return 0;

        default:
            printk("Error: Incorrect operation requested, returning.\n");
            return -1;
    }

    return 0;
}

static int kmod_open(struct inode* inode, struct file* file) {
    printk("Opened kmod.\n");
    return 0;
}

static int kmod_release(struct inode* inode, struct file* file) {
    printk("Closed kmod.\n");
    return 0;
}

static struct file_operations fops = {
    .owner 		= THIS_MODULE,
    .open 		= kmod_open,
    .release 		= kmod_release,
    .unlocked_ioctl 	= kmod_ioctl,
};

bool kmod_ioctl_init(void) {
    //printk("Here\n");
    if (alloc_chrdev_region(&dev, 0, 1, "usbaccess") < 0) return false;

    cdev_init(&kmod_cdev, &fops);
    if (cdev_add(&kmod_cdev, dev, 1) < 0) goto cdevfailed;

#if LINUX_VERSION_CODE <= KERNEL_VERSION(6,2,16)
    kmod_class = class_create(THIS_MODULE, "kmod_class");
    //printk("herepart2");
#else
    kmod_class = class_create("kmod_class");
    //printk("herepart3");
#endif
    if (IS_ERR(kmod_class)) goto cdevfailed;

    if (IS_ERR(device_create(kmod_class, NULL, dev, NULL, "kmod"))) goto classfailed;

    printk("[*] IOCTL device initialization complete.\n");
    return true;

classfailed:
    class_destroy(kmod_class);
    printk("failed\n");
cdevfailed:
    unregister_chrdev_region(dev, 1);
    return false;
}

void kmod_ioctl_teardown(void) {
    device_destroy(kmod_class, dev);
    class_destroy(kmod_class);
    cdev_del(&kmod_cdev);
    unregister_chrdev_region(dev, 1);
    printk("[*] IOCTL device teardown complete.\n");
}
