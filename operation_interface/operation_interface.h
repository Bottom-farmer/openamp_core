#ifndef __OPERATION_INTERFACE_H_
#define __OPERATION_INTERFACE_H_

#include "openamp/virtio.h"
#include "stdio.h"
#include <metal/alloc.h>
#include <metal/device.h>
#include <metal/irq.h>
#include <metal/irq_controller.h>
#include <metal/list.h>
#include <metal/mutex.h>
#include <metal/sys.h>
#include <metal/utilities.h>
#include <openamp/open_amp.h>
#include <poll.h>
#include <pthread.h>
#include <sched.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <sys/eventfd.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <unistd.h>

#define AMP_MASTER
#define APP_NODE_MAXNUM            16

#define HW_SECONDARY_MAGIC         'D'
#define HW_SECONDARY_CPU_ONLINE    _IOW(HW_SECONDARY_MAGIC, 0, int)
#define HW_SECONDARY_CPU_DOWNLINE  _IOW(HW_SECONDARY_MAGIC, 1, int)
#define HW_SECONDARY_LOAD_FIRMWARE _IOW(HW_SECONDARY_MAGIC, 2, int)
#define HW_SECONDARY_SIGNAL_SEND   _IOW(HW_SECONDARY_MAGIC, 3, int)
#define HW_SECONDARY_IRQ_INSTALL   _IOW(HW_SECONDARY_MAGIC, 4, int)

#define ALIGN(size, align)         ((uintptr_t)((size) + (align) - 1) & (~((uintptr_t)(align) - 1)))

struct openamp_app_node {
    struct rpmsg_endpoint ept;
    const char           *name;
    uint32_t              src;
    uint32_t              dest;
    rpmsg_ept_cb          cb;
    rpmsg_ns_unbind_cb    unbind_cb;
    struct metal_list     node_list;
};
typedef struct openamp_app_node *openamp_app_node_t;

typedef struct secondary_config {
    int        cpu_id;
    long long  entry;
    int        irq_id;
    const char file_path[1024];
} secondary_config;
typedef secondary_config *secondary_config_t;

struct openamp_virtio_device {
    struct rpmsg_virtio_device   rvdev;
    struct virtio_device         vdev;
    struct metal_io_region       io_region;
    metal_phys_addr_t            shm_physmap[1];
    struct rpmsg_virtio_shm_pool shpool;
    struct virtio_vring_info     vrings[2];
    struct rpmsg_endpoint       *ept;
    int                          fd;
    const char                  *dev_name;
    void                        *shm_mpaddr;
    void                        *shm_physaddr;
    void                        *shm_vaddr;
    uint32_t                     shm_size;
    secondary_config             secondary_config;
    void                        *vring_tx;
    unsigned int                 tx_num;
    void                        *vring_rx;
    unsigned int                 rx_num;
    void                        *vring_buf;
    unsigned int                 vring_size;
    unsigned int                 align_size;
    unsigned int                 vdev_status_size;
    void                        *user_data;
};
typedef struct openamp_virtio_device *openamp_virtio_device_t;

int                     openamp_dev_create(openamp_virtio_device_t dev);
int                     openamp_dev_delete(openamp_virtio_device_t dev);
openamp_virtio_device_t openamp_dev_get(void);
openamp_app_node_t      openamp_app_node_register(const char *name, uint32_t src, rpmsg_ept_cb cb, rpmsg_ns_unbind_cb unbind_cb,
                                                  openamp_virtio_device_t dev);
void                    openamp_app_node_unregister(const char *name);
openamp_app_node_t      openamp_find_app_node(const char *name);
int                     openamp_app_send(struct rpmsg_endpoint *ept, const void *data, size_t len);

#endif /* __OPERATION_INTERFACE_H__ */