#ifndef __TIZIANO_PRIV_H__
#define __TIZIANO_PRIV_H__


#include <linux/slab.h>
#include <linux/irqreturn.h>
#include <linux/dma-mapping.h>
#include <linux/delay.h>
#include <linux/list.h>

void *isp_kmalloc(size_t size, gfp_t flags);
void isp_kfree(void *objp);

void isp_spin_lock_init(spinlock_t *lock);
void isp_spin_lock_irqsave(spinlock_t *lock, unsigned long flags);
void isp_spin_unlock_irqrestore(spinlock_t *lock, unsigned long flags);

#define ISP_DMA_BIDIRECTIONAL DMA_BIDIRECTIONAL
void isp_dma_cache_sync(struct device *dev, void *vaddr, size_t size,
			 enum dma_data_direction direction);

void ISP_INIT_LIST_HEAD(struct list_head *list);
void isp_list_add_tail(struct list_head *new, struct list_head *head);
#define isp_list_first_entry(ptr, type, member) list_first_entry(ptr, type, member)
void isp_list_del(struct list_head *entry);
int isp_list_empty(const struct list_head *head);

void isp_complete(struct completion *);
unsigned long isp_wait_for_completion_timeout(struct completion *x, unsigned long timeout);
void isp_init_completion(struct completion *x);



#endif
