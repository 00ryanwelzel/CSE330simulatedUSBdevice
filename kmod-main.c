#include <linux/kthread.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/moduleparam.h>
#include <linux/fs.h>
#include <linux/bio.h>
#include <linux/buffer_head.h>
#include <linux/blkdev.h>
#include <linux/namei.h>
#include <linux/vmalloc.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("RYAN WELZEL");
MODULE_DESCRIPTION("A block Abstraction Read/Write for a USB device.");
MODULE_VERSION("1.0");

//remember 1g device sdc not sdb like example
char* device = "/dev/sdc";
module_param(device, charp, S_IRUGO);

unsigned int cur_dev_sector = 0;
static struct block_device *bdevice = NULL;
static struct file *usb_file = NULL;

extern bool kmod_ioctl_init(void);
extern void kmod_ioctl_teardown(void);

static bool open_usb(void)
{
    usb_file = bdev_file_open_by_path(device, BLK_OPEN_READ | BLK_OPEN_WRITE, NULL, NULL);
    if (IS_ERR(usb_file)) {
	//printk("fail here\n");
        return false;
    }

    bdevice = file_bdev(usb_file);
    if (!bdevice || IS_ERR(bdevice)) {
        return false;
    }

    return true;
}

long rw_usb(char* data, unsigned int size, unsigned int offset, bool is_write)
{
    unsigned int remaining = size;
    unsigned int processed = 0;

    while (remaining > 0) {
        struct bio *bio;
        struct page *page;
        unsigned int chunk = min(remaining, 512u);
        sector_t sector = (offset == -1) ? cur_dev_sector : offset / 512;
        //cur_dev_sector = 1;
	//cur_dev_sector = 0;

        bio = bio_alloc(bdevice, 1, REQ_OP_READ, GFP_NOIO);
	//bio = bio_alloc(get_device(), 1, REQ_OP_READ, GFP_NOIO);
        //bio = bio_alloc(GFP_NOIO, 1);
        if (!bio)
            return -ENOMEM;

        bio_set_dev(bio, bdevice);
	//bio->bi_opf = REQ_OP_WRITE;
	//bio->bi_opf = REQ_OP_READ;
        bio->bi_opf = is_write ? REQ_OP_WRITE : REQ_OP_READ;
        bio->bi_iter.bi_sector = sector;

        page = vmalloc_to_page(data + processed);
        if (!page) {
            bio_put(bio);
            return -EFAULT;
        }

        bio_add_page(bio, page, chunk, offset_in_page(data + processed));

        if (submit_bio_wait(bio) < 0) {
            bio_put(bio);
            return -EIO;
        }

        //bio_put(bio);
	//submit_bio_wait(bio);
	
	//printk("%u, %d, %llu, %u", size, offset, sector, cur_dev_sector);

        processed += chunk;
        remaining -= chunk;
        cur_dev_sector = sector + (chunk / 512);

        if (offset != -1)
            offset += chunk;
    }

    return processed;
}

static void close_usb(void)
{
    if (usb_file) {
        filp_close(usb_file, NULL);
        usb_file = NULL;
        bdevice = NULL;
    }
}

static int __init kmod_init(void)
{
    cur_dev_sector = 0;
    printk("Hello World!\n");
    //printk("Fortnite Battlepass!\n");
    if (!open_usb()) {
        printk("Failed to open USB block device\n");
        return -ENODEV;
    }
    if (!kmod_ioctl_init()) {
        close_usb();
	//printk("fail here\n");
        return -EIO;
    }
    return 0;
}

static void __exit kmod_fini(void)
{
    close_usb();
    kmod_ioctl_teardown();
    printk("Goodbye, World!\n");
}

module_init(kmod_init);
module_exit(kmod_fini);
