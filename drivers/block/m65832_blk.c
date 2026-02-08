// SPDX-License-Identifier: GPL-2.0
/*
 * M65832 block device driver
 *
 * Simple DMA-based block device for the M65832 platform (DE25).
 * Uses the SD controller in DMA mode for sector read/write.
 *
 * Register layout (offsets from base 0x1000A000):
 *   0x00  CTRL      - Control/command register (W)
 *   0x04  STATUS    - Status register (R)
 *   0x0C  ARG       - Sector number low (R/W)
 *   0x10  RESP0     - Sector number high (R/W)
 *   0x28  BLKCNT    - Block count (R/W)
 *   0x38  DMA_ADDR  - DMA address (R/W)
 *   0x40  CAP_LO    - Capacity low (R)
 *   0x44  CAP_HI    - Capacity high (R)
 */

#include <linux/blkdev.h>
#include <linux/blk-mq.h>
#include <linux/init.h>
#include <linux/io.h>
#include <linux/module.h>

#define DRIVER_NAME	"m65832-blk"

/* Register offsets */
#define BLK_CTRL	0x00
#define BLK_STATUS	0x04
#define BLK_ARG		0x0C
#define BLK_RESP0	0x10
#define BLK_BLKCNT	0x28
#define BLK_DMA_ADDR	0x38
#define BLK_CAP_LO	0x40
#define BLK_CAP_HI	0x44

/* Status bits */
#define STATUS_PRESENT	(1 << 0)
#define STATUS_READY	(1 << 1)
#define STATUS_BUSY	(1 << 2)
#define STATUS_ERROR	(1 << 3)
#define STATUS_COMPLETE	(1 << 8)

/* Commands */
#define CMD_READ	0x01
#define CMD_WRITE	0x02
#define CMD_FLUSH	0x03

/* Platform defaults (DE25) */
#define M65832_BLK_BASE	0x1000A000
#define M65832_BLK_SIZE	0x1000
#define M65832_BLK_IRQ	11

#define SECTOR_SIZE	512

struct m65832_blk {
	void __iomem		*regs;
	struct gendisk		*disk;
	struct blk_mq_tag_set	tag_set;
	unsigned long		capacity;	/* in sectors */
};

static struct m65832_blk *m65832_blk_dev;

static inline u32 blk_readl(struct m65832_blk *dev, unsigned int off)
{
	return readl(dev->regs + off);
}

static inline void blk_writel(struct m65832_blk *dev, unsigned int off, u32 val)
{
	writel(val, dev->regs + off);
}

static void m65832_blk_wait_ready(struct m65832_blk *dev)
{
	while (blk_readl(dev, BLK_STATUS) & STATUS_BUSY)
		cpu_relax();
}

static int m65832_blk_transfer(struct m65832_blk *dev, unsigned long sector,
			       unsigned int nsect, void *buffer, bool write)
{
	dma_addr_t dma_addr = virt_to_phys(buffer);

	m65832_blk_wait_ready(dev);

	blk_writel(dev, BLK_ARG, (u32)sector);
	blk_writel(dev, BLK_RESP0, (u32)(sector >> 32));
	blk_writel(dev, BLK_DMA_ADDR, (u32)dma_addr);
	blk_writel(dev, BLK_BLKCNT, nsect);
	blk_writel(dev, BLK_CTRL, write ? CMD_WRITE : CMD_READ);

	m65832_blk_wait_ready(dev);

	if (blk_readl(dev, BLK_STATUS) & STATUS_ERROR)
		return -EIO;

	return 0;
}

static blk_status_t m65832_blk_queue_rq(struct blk_mq_hw_ctx *hctx,
					 const struct blk_mq_queue_data *bd)
{
	struct request *rq = bd->rq;
	struct m65832_blk *dev = m65832_blk_dev;
	struct req_iterator iter;
	struct bio_vec bvec;
	sector_t sector = blk_rq_pos(rq);
	int err = 0;

	blk_mq_start_request(rq);

	rq_for_each_segment(bvec, rq, iter) {
		void *buf = page_address(bvec.bv_page) + bvec.bv_offset;
		unsigned int nsect = bvec.bv_len / SECTOR_SIZE;
		bool write = (rq_data_dir(rq) == WRITE);

		err = m65832_blk_transfer(dev, sector, nsect, buf, write);
		if (err)
			break;
		sector += nsect;
	}

	blk_mq_end_request(rq, err ? BLK_STS_IOERR : BLK_STS_OK);
	return BLK_STS_OK;
}

static const struct blk_mq_ops m65832_blk_mq_ops = {
	.queue_rq = m65832_blk_queue_rq,
};

static const struct block_device_operations m65832_blk_fops = {
	.owner = THIS_MODULE,
};

static int __init m65832_blk_init(void)
{
	struct m65832_blk *dev;
	struct gendisk *disk;
	u32 status;
	int ret;

	dev = kzalloc(sizeof(*dev), GFP_KERNEL);
	if (!dev)
		return -ENOMEM;

	dev->regs = ioremap(M65832_BLK_BASE, M65832_BLK_SIZE);
	if (!dev->regs) {
		ret = -ENOMEM;
		goto err_free;
	}

	/* Check if device is present */
	status = blk_readl(dev, BLK_STATUS);
	if (!(status & STATUS_PRESENT)) {
		pr_info("m65832-blk: no disk present\n");
		ret = -ENODEV;
		goto err_unmap;
	}

	/* Read capacity */
	dev->capacity = blk_readl(dev, BLK_CAP_LO);
	pr_info("m65832-blk: disk present, %lu sectors (%lu MB)\n",
		dev->capacity, dev->capacity / 2048);

	/* Set up blk-mq */
	memset(&dev->tag_set, 0, sizeof(dev->tag_set));
	dev->tag_set.ops = &m65832_blk_mq_ops;
	dev->tag_set.nr_hw_queues = 1;
	dev->tag_set.queue_depth = 16;
	dev->tag_set.numa_node = NUMA_NO_NODE;
	dev->tag_set.flags = 0;

	ret = blk_mq_alloc_tag_set(&dev->tag_set);
	if (ret)
		goto err_unmap;

	disk = blk_mq_alloc_disk(&dev->tag_set, NULL, dev);
	if (IS_ERR(disk)) {
		ret = PTR_ERR(disk);
		goto err_tags;
	}

	dev->disk = disk;
	disk->major = 0;  /* dynamically allocated */
	disk->first_minor = 0;
	disk->minors = 16;
	disk->fops = &m65832_blk_fops;
	snprintf(disk->disk_name, sizeof(disk->disk_name), "m65832sd");
	set_capacity(disk, dev->capacity);

	/* Block sizes are set via tag_set or default to 512 */

	ret = add_disk(disk);
	if (ret)
		goto err_disk;

	m65832_blk_dev = dev;
	pr_info("m65832-blk: registered as %s\n", disk->disk_name);

	return 0;

err_disk:
	put_disk(disk);
err_tags:
	blk_mq_free_tag_set(&dev->tag_set);
err_unmap:
	iounmap(dev->regs);
err_free:
	kfree(dev);
	return ret;
}

static void __exit m65832_blk_exit(void)
{
	struct m65832_blk *dev = m65832_blk_dev;

	if (!dev)
		return;

	del_gendisk(dev->disk);
	put_disk(dev->disk);
	blk_mq_free_tag_set(&dev->tag_set);
	iounmap(dev->regs);
	kfree(dev);
	m65832_blk_dev = NULL;
}

module_init(m65832_blk_init);
module_exit(m65832_blk_exit);

MODULE_DESCRIPTION("M65832 block device driver");
MODULE_LICENSE("GPL");
