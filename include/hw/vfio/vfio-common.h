/*
 * common header for vfio based device assignment support
 *
 * Copyright Red Hat, Inc. 2012
 *
 * Authors:
 *  Alex Williamson <alex.williamson@redhat.com>
 *
 * This work is licensed under the terms of the GNU GPL, version 2.  See
 * the COPYING file in the top-level directory.
 *
 * Based on qemu-kvm device-assignment:
 *  Adapted for KVM by Qumranet.
 *  Copyright (c) 2007, Neocleus, Alex Novik (alex@neocleus.com)
 *  Copyright (c) 2007, Neocleus, Guy Zana (guy@neocleus.com)
 *  Copyright (C) 2008, Qumranet, Amit Shah (amit.shah@qumranet.com)
 *  Copyright (C) 2008, Red Hat, Amit Shah (amit.shah@redhat.com)
 *  Copyright (C) 2008, IBM, Muli Ben-Yehuda (muli@il.ibm.com)
 */
#ifndef HW_VFIO_VFIO_COMMON_H
#define HW_VFIO_VFIO_COMMON_H

#include "qemu-common.h"
#include "exec/address-spaces.h"
#include "exec/memory.h"
#include "qemu/queue.h"
#include "qemu/notify.h"

/*#define DEBUG_VFIO*/
#ifdef DEBUG_VFIO
#define DPRINTF(fmt, ...) \
    do { fprintf(stderr, "vfio: " fmt, ## __VA_ARGS__); } while (0)
#else
#define DPRINTF(fmt, ...) \
    do { } while (0)
#endif

/* Extra debugging, trap acceleration paths for more logging */
#define VFIO_ALLOW_MMAP 1
#define VFIO_ALLOW_KVM_INTX 1
#define VFIO_ALLOW_KVM_MSI 1
#define VFIO_ALLOW_KVM_MSIX 1

enum {
    VFIO_DEVICE_TYPE_PCI = 0,
};

typedef struct VFIORegion {
    struct VFIODevice *vbasedev;
    off_t fd_offset; /* offset of region within device fd */
    MemoryRegion mem; /* slow, read/write access */
    MemoryRegion mmap_mem; /* direct mapped access */
    void *mmap;
    size_t size;
    uint32_t flags; /* VFIO region flags (rd/wr/mmap) */
    uint8_t nr; /* cache the region number for debug */
} VFIORegion;

typedef struct VFIOAddressSpace {
    AddressSpace *as;
    QLIST_HEAD(, VFIOContainer) containers;
    QLIST_ENTRY(VFIOAddressSpace) list;
} VFIOAddressSpace;

struct VFIOGroup;

typedef struct VFIOType1 {
    MemoryListener listener;
    int error;
    bool initialized;
} VFIOType1;

typedef struct VFIOContainer {
    VFIOAddressSpace *space;
    int fd; /* /dev/vfio/vfio, empowered by the attached groups */
    struct {
        /* enable abstraction to support various iommu backends */
        union {
            VFIOType1 type1;
        };
        void (*release)(struct VFIOContainer *);
    } iommu_data;
    QLIST_HEAD(, VFIOGuestIOMMU) giommu_list;
    QLIST_HEAD(, VFIOGroup) group_list;
    QLIST_ENTRY(VFIOContainer) next;
} VFIOContainer;

typedef struct VFIOGuestIOMMU {
    VFIOContainer *container;
    MemoryRegion *iommu;
    Notifier n;
    QLIST_ENTRY(VFIOGuestIOMMU) giommu_next;
} VFIOGuestIOMMU;

typedef struct VFIODeviceOps VFIODeviceOps;

typedef struct VFIODevice {
    QLIST_ENTRY(VFIODevice) next;
    struct VFIOGroup *group;
    char *name;
    int fd;
    int type;
    bool reset_works;
    bool needs_reset;
    VFIODeviceOps *ops;
    unsigned int num_irqs;
    unsigned int num_regions;
    unsigned int flags;
} VFIODevice;

struct VFIODeviceOps {
    void (*vfio_compute_needs_reset)(VFIODevice *vdev);
    int (*vfio_hot_reset_multi)(VFIODevice *vdev);
    void (*vfio_eoi)(VFIODevice *vdev);
};

typedef struct VFIOGroup {
    int fd;
    int groupid;
    VFIOContainer *container;
    QLIST_HEAD(, VFIODevice) device_list;
    QLIST_ENTRY(VFIOGroup) next;
    QLIST_ENTRY(VFIOGroup) container_next;
} VFIOGroup;

void vfio_put_base_device(VFIODevice *vbasedev);
void vfio_disable_irqindex(VFIODevice *vbasedev, int index);
void vfio_unmask_single_irqindex(VFIODevice *vbasedev, int index);
void vfio_mask_single_irqindex(VFIODevice *vbasedev, int index);
void vfio_region_write(void *opaque, hwaddr addr,
                           uint64_t data, unsigned size);
uint64_t vfio_region_read(void *opaque,
                          hwaddr addr, unsigned size);
void vfio_listener_release(VFIOContainer *container);
int vfio_mmap_region(Object *vdev, VFIORegion *region,
                     MemoryRegion *mem, MemoryRegion *submem,
                     void **map, size_t size, off_t offset,
                     const char *name);
void vfio_reset_handler(void *opaque);
VFIOGroup *vfio_get_group(int groupid, AddressSpace *as);
void vfio_put_group(VFIOGroup *group);
int vfio_get_device(VFIOGroup *group, const char *name,
                    VFIODevice *vbasedev);

extern const MemoryRegionOps vfio_region_ops;
extern const MemoryListener vfio_memory_listener;
extern QLIST_HEAD(vfio_group_head, VFIOGroup) vfio_group_list;
extern QLIST_HEAD(vfio_as_head, VFIOAddressSpace) vfio_address_spaces;

#endif /* !HW_VFIO_VFIO_COMMON_H */
