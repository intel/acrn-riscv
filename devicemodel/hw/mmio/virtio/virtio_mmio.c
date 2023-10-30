#include <sys/uio.h>
#include <stdio.h>
#include <stddef.h>
#include <pthread.h>
#include <string.h>
#include <stdlib.h>

#include "dm.h"
#include "mem.h"
#include "timer.h"
#include <atomic.h>
#include "virtio_mmio.h"
#include "vmmapi.h"
#include "log.h"

#define DEV_STRUCT(vs) ((void *)(vs))

static uint32_t vmmio_num = 0;
/*
 * Initialize the currently-selected virtio queue (base->curq).
 * The guest just gave us a page frame number, from which we can
 * calculate the addresses of the queue.
 * This interface is only valid for virtio legacy.
 */
void virtio_vq_init(struct virtio_base *base, uint32_t pfn)
{
	struct virtio_vq_info *vq;
	uint64_t phys;
	size_t size;
	char *vb;

	vq = &base->queues[base->curq];
	vq->pfn = pfn;
	phys = (uint64_t)pfn << VRING_PAGE_BITS;
	size = vring_size(vq->qsize, VIRTIO_VRING_ALIGN);
	vb = paddr_guest2host(base->ctx, phys, size);

	/* First page(s) are descriptors... */
	vq->desc = (struct vring_desc *)vb;
	vb += vq->qsize * sizeof(struct vring_desc);

	/* ... immediately followed by "avail" ring (entirely uint16_t's) */
	vq->avail = (struct vring_avail *)vb;
	vb += (2 + vq->qsize + 1) * sizeof(uint16_t);

	/* Then it's rounded up to the next page... */
	vb = (char *)roundup2((uintptr_t)vb, VIRTIO_VRING_ALIGN);

	/* ... and the last page(s) are the used ring. */
	vq->used = (struct vring_used *)vb;

	/* Start at 0 when we use it. */
	vq->last_avail = 0;
	vq->save_used = 0;

	/* Mark queue as allocated after initialization is complete. */
	mb();
	vq->flags = VQ_ALLOC;
}

#if 0
/*
 * Initialize the currently-selected virtio queue (base->curq).
 * The guest just gave us the gpa of desc array, avail ring and
 * used ring, from which we can initialize the virtqueue.
 * This interface is only valid for virtio modern.
 */
static void
virtio_vq_enable(struct virtio_base *base)
{
	struct virtio_vq_info *vq;
	uint16_t qsz;
	uint64_t phys;
	size_t size;
	char *vb;

	vq = &base->queues[base->curq];
	qsz = vq->qsize;

	/* descriptors */
	phys = (((uint64_t)vq->gpa_desc[1]) << 32) | vq->gpa_desc[0];
	size = qsz * sizeof(struct vring_desc);
	vb = paddr_guest2host(base->ctx, phys, size);
	vq->desc = (struct vring_desc *)vb;

	/* available ring */
	phys = (((uint64_t)vq->gpa_avail[1]) << 32) | vq->gpa_avail[0];
	size = (2 + qsz + 1) * sizeof(uint16_t);
	vb = paddr_guest2host(base->ctx, phys, size);
	vq->avail = (struct vring_avail *)vb;

	/* used ring */
	phys = (((uint64_t)vq->gpa_used[1]) << 32) | vq->gpa_used[0];
	size = sizeof(uint16_t) * 3 + sizeof(struct vring_used_elem) * qsz;
	vb = paddr_guest2host(base->ctx, phys, size);
	vq->used = (struct vring_used *)vb;

	/* Start at 0 when we use it. */
	vq->last_avail = 0;
	vq->save_used = 0;

	/* Mark queue as enabled. */
	vq->enabled = true;

	/* Mark queue as allocated after initialization is complete. */
	mb();
	vq->flags = VQ_ALLOC;
}
#endif
/*
 * Helper inline for vq_getchain(): record the i'th "real"
 * descriptor.
 */

void
virtio_mmio_linkup(struct virtio_base *base, struct virtio_ops *vops,
	      struct virtio_vq_info *queues,
	      int backend_type)
{
	int i;

	base->vops = vops;
	base->name = vops->name;
	base->backend_type = backend_type;

	base->queues = queues;
	for (i = 0; i < vops->nvq; i++) {
		queues[i].base = base;
		queues[i].num = i;
	}
}
static inline void
_vq_record(int i, volatile struct vring_desc *vd, struct vmctx *ctx,
	   struct iovec *iov, int n_iov, uint16_t *flags) {

	if (i >= n_iov)
		return;
	iov[i].iov_base = paddr_guest2host(ctx, vd->addr, vd->len);
	iov[i].iov_len = vd->len;
	if (flags != NULL)
		flags[i] = vd->flags;
}
#define	VQ_MAX_DESCRIPTORS	512	/* see below */

/*
 * Examine the chain of descriptors starting at the "next one" to
 * make sure that they describe a sensible request.  If so, return
 * the number of "real" descriptors that would be needed/used in
 * acting on this request.  This may be smaller than the number of
 * available descriptors, e.g., if there are two available but
 * they are two separate requests, this just returns 1.  Or, it
 * may be larger: if there are indirect descriptors involved,
 * there may only be one descriptor available but it may be an
 * indirect pointing to eight more.  We return 8 in this case,
 * i.e., we do not count the indirect descriptors, only the "real"
 * ones.
 *
 * Basically, this vets the flags and vd_next field of each
 * descriptor and tells you how many are involved.  Since some may
 * be indirect, this also needs the vmctx (in the pci_vdev
 * at base->dev) so that it can find indirect descriptors.
 *
 * As we process each descriptor, we copy and adjust it (guest to
 * host address wise, also using the vmtctx) into the given iov[]
 * array (of the given size).  If the array overflows, we stop
 * placing values into the array but keep processing descriptors,
 * up to VQ_MAX_DESCRIPTORS, before giving up and returning -1.
 * So you, the caller, must not assume that iov[] is as big as the
 * return value (you can process the same thing twice to allocate
 * a larger iov array if needed, or supply a zero length to find
 * out how much space is needed).
 *
 * If you want to verify the WRITE flag on each descriptor, pass a
 * non-NULL "flags" pointer to an array of "uint16_t" of the same size
 * as n_iov and we'll copy each flags field after unwinding any
 * indirects.
 *
 * If some descriptor(s) are invalid, this prints a diagnostic message
 * and returns -1.  If no descriptors are ready now it simply returns 0.
 *
 * You are assumed to have done a vq_ring_ready() if needed (note
 * that vq_has_descs() does one).
 */
int
vq_getchain(struct virtio_vq_info *vq, uint16_t *pidx,
	    struct iovec *iov, int n_iov, uint16_t *flags)
{
	int i;
	u_int ndesc, n_indir;
	u_int idx, next;

	volatile struct vring_desc *vdir, *vindir, *vp;
	struct vmctx *ctx;
	struct virtio_base *base;
	const char *name;

	base = vq->base;
	name = base->vops->name;

	/*
	 * Note: it's the responsibility of the guest not to
	 * update vq->avail->idx until all of the descriptors
	 * the guest has written are valid (including all their
	 * next fields and vd_flags).
	 *
	 * Compute (last_avail - idx) in integers mod 2**16.  This is
	 * the number of descriptors the device has made available
	 * since the last time we updated vq->last_avail.
	 *
	 * We just need to do the subtraction as an unsigned int,
	 * then trim off excess bits.
	 */
	idx = vq->last_avail;
	ndesc = (uint16_t)((u_int)vq->avail->idx - idx);
	if (ndesc == 0)
		return 0;
	if (ndesc > vq->qsize) {
		/* XXX need better way to diagnose issues */
		pr_err("%s: ndesc (%u) out of range, driver confused?\r\n",
		    name, (u_int)ndesc);
		return -1;
	}

	/*
	 * Now count/parse "involved" descriptors starting from
	 * the head of the chain.
	 *
	 * To prevent loops, we could be more complicated and
	 * check whether we're re-visiting a previously visited
	 * index, but we just abort if the count gets excessive.
	 */
	ctx = base->ctx;
	*pidx = next = vq->avail->ring[idx & (vq->qsize - 1)];
	vq->last_avail++;
	for (i = 0; i < VQ_MAX_DESCRIPTORS; next = vdir->next) {
		if (next >= vq->qsize) {
			pr_err("%s: descriptor index %u out of range, "
			    "driver confused?\r\n",
			    name, next);
			return -1;
		}
		vdir = &vq->desc[next];
		if ((vdir->flags & VRING_DESC_F_INDIRECT) == 0) {
			_vq_record(i, vdir, ctx, iov, n_iov, flags);
			i++;
		} else if ((base->device_caps &
		    (1 << VIRTIO_RING_F_INDIRECT_DESC)) == 0) {
			pr_err("%s: descriptor has forbidden INDIRECT flag, "
			    "driver confused?\r\n",
			    name);
			return -1;
		} else {
			n_indir = vdir->len / 16;
			if ((vdir->len & 0xf) || n_indir == 0) {
				pr_err("%s: invalid indir len 0x%x, "
				    "driver confused?\r\n",
				    name, (u_int)vdir->len);
				return -1;
			}
			vindir = paddr_guest2host(ctx,
			    vdir->addr, vdir->len);
			/*
			 * Indirects start at the 0th, then follow
			 * their own embedded "next"s until those run
			 * out.  Each one's indirect flag must be off
			 * (we don't really have to check, could just
			 * ignore errors...).
			 */
			next = 0;
			for (;;) {
				vp = &vindir[next];
				if (vp->flags & VRING_DESC_F_INDIRECT) {
					pr_err("%s: indirect desc has INDIR flag,"
					    " driver confused?\r\n",
					    name);
					return -1;
				}
				_vq_record(i, vp, ctx, iov, n_iov, flags);
				if (++i > VQ_MAX_DESCRIPTORS)
					goto loopy;
				if ((vp->flags & VRING_DESC_F_NEXT) == 0)
					break;
				next = vp->next;
				if (next >= n_indir) {
					pr_err("%s: invalid next %u > %u, "
					    "driver confused?\r\n",
					    name, (u_int)next, n_indir);
					return -1;
				}
			}
		}
		if ((vdir->flags & VRING_DESC_F_NEXT) == 0)
			return i;
	}
loopy:
	pr_err("%s: descriptor loop? count > %d - driver confused?\r\n",
	    name, i);
	return -1;
}

/*
 * Return the currently-first request chain back to the available queue.
 *
 * (This chain is the one you handled when you called vq_getchain()
 * and used its positive return value.)
 */
void
vq_retchain(struct virtio_vq_info *vq)
{
	vq->last_avail--;
}

/*
 * Return specified request chain to the guest, setting its I/O length
 * to the provided value.
 *
 * (This chain is the one you handled when you called vq_getchain()
 * and used its positive return value.)
 */
void
vq_relchain(struct virtio_vq_info *vq, uint16_t idx, uint32_t iolen)
{
	uint16_t uidx, mask;
	volatile struct vring_used *vuh;
	volatile struct vring_used_elem *vue;

	/*
	 * Notes:
	 *  - mask is N-1 where N is a power of 2 so computes x % N
	 *  - vuh points to the "used" data shared with guest
	 *  - vue points to the "used" ring entry we want to update
	 *  - head is the same value we compute in vq_iovecs().
	 *
	 * (I apologize for the two fields named idx; the
	 * virtio spec calls the one that vue points to, "id"...)
	 */
	mask = vq->qsize - 1;
	vuh = vq->used;

	uidx = vuh->idx;
	vue = &vuh->ring[uidx++ & mask];
	vue->id = idx;
	vue->len = iolen;
	vuh->idx = uidx;
}


void virtio_mmio_write(struct virtio_base *base, uint64_t offset, uint64_t size, uint64_t value)
{

	struct virtio_vq_info *vq;
	struct virtio_ops *vops;
	int error;

	vops = base->vops;

	pr_dbg("virtio_mmio_write offset=%lx size=%lx\n", offset, size);
	if (offset >= VIRTIO_MMIO_CONFIG) {
		offset -= VIRTIO_MMIO_CONFIG;
		error = (*vops->cfgwrite)(DEV_STRUCT(base), offset,
				size, value);
		if (error) {
			pr_err("cfg write failed with error %d\n", error);
		}
		return;
	}

	switch (offset) {
	case VIRTIO_MMIO_DEVICE_FEATURES_SEL:
		base->device_feature_select = value;
		break;
	case VIRTIO_MMIO_DRIVER_FEATURES:
		if (base->status & VIRTIO_CONFIG_S_DRIVER_OK)
			break;
		if (base->driver_feature_select < 2) {
			value &= 0xffffffff;
			base->negotiated_caps =
				(value << (base->driver_feature_select * 32))
				& base->device_caps;
		}
		break;
	case VIRTIO_MMIO_DRIVER_FEATURES_SEL:
		base->driver_feature_select = value;
		break;
	case VIRTIO_MMIO_GUEST_PAGE_SIZE:
		pr_info("write page size !!!\n");
		/*
	    proxy->guest_page_shift = ctz32(value);
	    if (proxy->guest_page_shift > 31) {
	        proxy->guest_page_shift = 0;
	    }*/
	    break;
	case VIRTIO_MMIO_QUEUE_SEL:
		base->curq = value;
		break;
	case VIRTIO_MMIO_QUEUE_NUM:
		vq = &base->queues[base->curq];
		vq->qsize = value;
		break;
	case VIRTIO_MMIO_QUEUE_ALIGN:
		pr_info("set queue  align!!!\n");
		break;
	case VIRTIO_MMIO_QUEUE_PFN:
		virtio_vq_init(base, value);
		break;
	case VIRTIO_MMIO_QUEUE_NOTIFY:
		vq = &base->queues[value];
		if (vq->notify)
			(*vq->notify)(DEV_STRUCT(base), vq);
		else if(vops->qnotify)
			(*vops->qnotify)(DEV_STRUCT(base), vq);
		else
			pr_err("%s: notify queue  missing vq/vops notify\r\n",
				__func__, (int)value);
		break;
	case VIRTIO_MMIO_INTERRUPT_ACK:
		base->isr = value;
		break;
	case VIRTIO_MMIO_STATUS:
		base->status = value;
		if (value == 0) {
			(*vops->reset)(DEV_STRUCT(base));
		}
		break;
	case VIRTIO_MMIO_MAGIC_VALUE:
	case VIRTIO_MMIO_VERSION:
	case VIRTIO_MMIO_DEVICE_ID:
	case VIRTIO_MMIO_VENDOR_ID:
	case VIRTIO_MMIO_DEVICE_FEATURES:
	case VIRTIO_MMIO_QUEUE_NUM_MAX:
	case VIRTIO_MMIO_INTERRUPT_STATUS:
	case VIRTIO_MMIO_CONFIG_GENERATION:
	    pr_err("%s: write to read-only register %lx\n",
	                  __func__, offset);
	    break;
	
	default:
	    pr_err("%s: bad register offset (0x%lx)\n",
	                  __func__, offset);
	}
}

uint32_t virtio_mmio_read(struct virtio_base *base, uint64_t offset, uint64_t size)
{
	uint32_t value;
	struct virtio_ops *vops;
	int error;

	pr_dbg("virtio_mmio_read offset=%lx size=%lx\n", offset, size);
	vops = base->vops;
	switch (size) {
		case 1:
			value = 0xff;
			break;
		case 2:
			value = 0xffff;
			break;
		case 4:
			value = 0xffffffff;
			break;
		default:
			pr_err("not support size %d\n", size);
			value = 0xffffffff;
	}

	if (offset >= VIRTIO_MMIO_CONFIG) {
		offset -= VIRTIO_MMIO_CONFIG;
		error = (*vops->cfgread)(DEV_STRUCT(base), offset,
				size, &value);
		if (error)
			pr_err("cfg read error %d\n");
		pr_dbg("%s: cfgread offset=%d, size=%d value=%d\n", __func__,
				offset, size, value);
		return value;
	}

	switch (offset) {
	case VIRTIO_MMIO_MAGIC_VALUE:
		value = VIRT_MAGIC;
		break;
	case VIRTIO_MMIO_VERSION:
		value = 1; //VIRT_VERSION;
		break;
	case VIRTIO_MMIO_DEVICE_ID:
		value = base->device_id;
		break;
	case VIRTIO_MMIO_VENDOR_ID:
		value = VIRTIO_VENDOR;
		break;
	case VIRTIO_MMIO_DEVICE_FEATURES:
		if (base->device_feature_select)
			value = 0;
		else
			value = base->device_caps;
		break;
	case VIRTIO_MMIO_QUEUE_NUM_MAX:
		value = base->queues[base->curq].qsize;
		break;
	case VIRTIO_MMIO_QUEUE_PFN:
		value = base->queues[base->curq].pfn;
		break;
	case VIRTIO_MMIO_QUEUE_READY:
		value = base->queues[base->curq].enabled;
		break;
	case VIRTIO_MMIO_INTERRUPT_STATUS:
		// DO we need to clear irq here ????
		value = base->isr;
		break;
	case VIRTIO_MMIO_STATUS:
		value = base->status;
		break;
	case VIRTIO_MMIO_CONFIG_GENERATION:
		value = base->config_generation;
		break;
	case VIRTIO_MMIO_DEVICE_FEATURES_SEL:
	case VIRTIO_MMIO_DRIVER_FEATURES:
	case VIRTIO_MMIO_DRIVER_FEATURES_SEL:
	case VIRTIO_MMIO_GUEST_PAGE_SIZE:
	case VIRTIO_MMIO_QUEUE_SEL:
	case VIRTIO_MMIO_QUEUE_NUM:
	case VIRTIO_MMIO_QUEUE_ALIGN:
	case VIRTIO_MMIO_QUEUE_NOTIFY:
	case VIRTIO_MMIO_INTERRUPT_ACK:
	case VIRTIO_MMIO_QUEUE_DESC_LOW:
	case VIRTIO_MMIO_QUEUE_DESC_HIGH:
	case VIRTIO_MMIO_QUEUE_AVAIL_LOW:
	case VIRTIO_MMIO_QUEUE_AVAIL_HIGH:
	case VIRTIO_MMIO_QUEUE_USED_LOW:
	case VIRTIO_MMIO_QUEUE_USED_HIGH:
	    pr_err("%s: read of write-only register 0x%lx\n",
	                  __func__, offset);
	    return 0;
	default:
	    pr_err("%s: bad register offset %lx\n",
	                  __func__, offset);
	    return 0;
	}
	pr_dbg("%s: offset %x value %x\n", __func__, offset, value);
	return value;

}

static int virtio_emul_mem_handler(struct vmctx *ctx, int vcpu, int dir, uint64_t addr,
		int size, uint64_t *val, void *arg1, long arg2)
{
	struct virtio_base *vbase = arg1;
	struct virtio_mmio_ops *ops = vbase->mm_ops;
	uint64_t mmio_addr = arg2;
	uint64_t offset;

	offset = addr - mmio_addr;
	if (dir == MEM_F_WRITE) {
		if (size == 8) {
			(*ops->mmio_write)(vbase, offset,
					   4, *val & 0xffffffff);
			(*ops->mmio_write)(vbase, offset + 4,
					   4, *val >> 32);
		} else { // we only support size = 4
			(*ops->mmio_write)(vbase, offset,
					   size, *val);
		}
	} else {
		if (size == 8) {
			uint64_t val_lo, val_hi;

			val_lo = (*ops->mmio_read)(vbase, offset, 4);

			val_hi = (*ops->mmio_read)(vbase, offset + 4, 4);

			*val = val_lo | (val_hi << 32);
		} else {
			*val = (*ops->mmio_read)(vbase, offset, size);
		}
	}
	return 0;
}

void virtio_register_mmio(struct virtio_base *vbase, struct virtio_mmio_ops *vmmio_ops)
{
	struct mem_range mr;

	vbase->mm_ops = vmmio_ops;
	bzero(&mr, sizeof(struct mem_range));
	mr.name = "virtio-mmio";
	mr.base = vmmio_ops->base;
	mr.size = vmmio_ops->size;
	mr.flags = MEM_F_RW;
	mr.handler = virtio_emul_mem_handler;
	mr.arg1 = vbase;
	mr.arg2 = vmmio_ops->base;
	register_mem(&mr); 
}

#define MAX_VMMIO_DEV_NUM 6
struct virtio_mmio_dev{
	char name[16];
	char *opts;
};

struct virtio_mmio_dev virtio_mmio_devs[MAX_VMMIO_DEV_NUM];

SET_DECLARE(virtio_mmio_ops_set, struct virtio_mmio_ops);

static struct virtio_mmio_ops *virtio_mmio_finddev(const char *name)
{
	struct virtio_mmio_ops **mdpp, *mdp;

	SET_FOREACH(mdpp, virtio_mmio_ops_set) {
		mdp = *mdpp;
		if (!strcmp(mdp->name, name))
			return mdp;
	}

	return NULL;
}

int parse_virtio_mmiodev(char *opt)
{
	int ret = 0;
	char *str, *cp = NULL;

	str = strdup(opt);
	if (!str) {
		fprintf(stderr, "%s: strdup returns NULL\n", __func__);
		return -1;
	}

	cp = strsep(&str, ",");

	if (cp) {
		strncpy(virtio_mmio_devs[vmmio_num].name, cp, strnlen(cp, 20));
		virtio_mmio_devs[vmmio_num].opts = str;
		vmmio_num++;
	}

	if (vmmio_num >= MAX_VMMIO_DEV_NUM) {
		pr_err("Only support max %d virtio-mmio devices!\n", MAX_VMMIO_DEV_NUM);
		ret = -1;
	}

	return ret;
}

int virtio_mmio_devs_init(struct vmctx *ctx)
{
	int i, ret = 0;
	struct virtio_mmio_ops *ops;

	for (i = 0; i < MAX_VMMIO_DEV_NUM; i++) {
		ops = virtio_mmio_finddev(virtio_mmio_devs[i].name);
		if (ops != NULL) {
			ret = (*ops->init)(ctx, ops, virtio_mmio_devs[i].opts);
			if (ret < 0)
				return -1;
		}
	}
	return 0;
}

void
virtio_mmio_reset_dev(struct virtio_base *base)
{
	struct virtio_vq_info *vq;
	int i, nvq;

	nvq = base->vops->nvq;
	for (vq = base->queues, i = 0; i < nvq; vq++, i++) {
		vq->flags = 0;
		vq->last_avail = 0;
		vq->save_used = 0;
		vq->pfn = 0;
		vq->gpa_desc[0] = 0;
		vq->gpa_desc[1] = 0;
		vq->gpa_avail[0] = 0;
		vq->gpa_avail[1] = 0;
		vq->gpa_used[0] = 0;
		vq->gpa_used[1] = 0;
		vq->enabled = 0;
	}
	base->negotiated_caps = 0;
	base->curq = 0;
	if (base->isr)
		vm_set_gsi_irq(base->ctx, base->spi_irq, 0);
	base->isr = 0;
	base->device_feature_select = 0;
	base->driver_feature_select = 0;
	base->config_generation = 0;
}

static inline void
vq_interrupt_mmio(struct virtio_base *base, struct virtio_vq_info *vq)
{
	VIRTIO_BASE_LOCK(base);
	base->isr |= 1;
	vm_set_gsi_irq(base->ctx, base->spi_irq, 1);
	VIRTIO_BASE_UNLOCK(base);
}
void
vq_endchains_mmio(struct virtio_vq_info *vq, int used_all_avail)
{
	struct virtio_base *base;
	uint16_t event_idx, new_idx, old_idx;
	int intr;

	/*
	 * Interrupt generation: if we're using EVENT_IDX,
	 * interrupt if we've crossed the event threshold.
	 * Otherwise interrupt is generated if we added "used" entries,
	 * but suppressed by VRING_AVAIL_F_NO_INTERRUPT.
	 *
	 * In any case, though, if NOTIFY_ON_EMPTY is set and the
	 * entire avail was processed, we need to interrupt always.
	 */

	atomic_thread_fence();

	base = vq->base;
	old_idx = vq->save_used;
	vq->save_used = new_idx = vq->used->idx;
	if (used_all_avail &&
	    (base->negotiated_caps & (1 << VIRTIO_F_NOTIFY_ON_EMPTY)))
		intr = 1;
	else if (base->negotiated_caps & (1 << VIRTIO_RING_F_EVENT_IDX)) {
		event_idx = VQ_USED_EVENT_IDX(vq);
		/*
		 * This calculation is per docs and the kernel
		 * (see src/sys/dev/virtio/virtio_ring.h).
		 */
		intr = (uint16_t)(new_idx - event_idx - 1) <
			(uint16_t)(new_idx - old_idx);
	} else {
		intr = new_idx != old_idx &&
		    !(vq->avail->flags & VRING_AVAIL_F_NO_INTERRUPT);
	}
	if (intr)
		vq_interrupt_mmio(base, vq);
}

void vq_clear_used_ring_flags(struct virtio_base *base, struct virtio_vq_info *vq)
{
	vq->used->flags &= ~VRING_USED_F_NO_NOTIFY;
}
