#include "operation_interface.h"

#define DEFAULT_PAGE_SHIFT (-1U)
#define DEFAULT_PAGE_MASK  (-1U)

struct openamp_app_node app_node_group[APP_NODE_MAXNUM] = {0};

static int openamp_dev_role_get(void)
{
#ifdef AMP_MASTER
    return RPMSG_HOST;
#else
    return RPMSG_REMOTE;
#endif
}

static uint8_t Virtio_get_status(struct virtio_device *vdev)
{
    openamp_virtio_device_t dev = metal_container_of(vdev, struct openamp_virtio_device, vdev);

    return *(volatile unsigned char *)(dev->shm_vaddr);
}

static void Virtio_set_status(struct virtio_device *vdev, uint8_t status)
{
    openamp_virtio_device_t dev = metal_container_of(vdev, struct openamp_virtio_device, vdev);

    *(volatile unsigned char *)(dev->shm_vaddr) = status;
}

static uint32_t Virtio_get_features(struct virtio_device *vdev)
{
    return (1UL << (VIRTIO_RPMSG_F_NS));
}

static void Virtio_set_features(struct virtio_device *vdev, unsigned int features) {}

static void Virtio_notify(struct virtqueue *vq)
{
    struct rpmsg_virtio_device *rvdev;
    struct virtio_device       *vdev;
    openamp_virtio_device_t     dev;

    vdev  = vq->vq_dev;
    rvdev = vdev->priv;
    dev   = (openamp_virtio_device_t)rvdev;

    if(ioctl(dev->fd, HW_SECONDARY_SIGNAL_SEND, &dev->secondary_config) < 0)
    {
        perror("ipi send error\n");
        return;
    }
}

static const struct virtio_dispatch dispatch = {
    .get_status   = Virtio_get_status,
    .set_status   = Virtio_set_status,
    .get_features = Virtio_get_features,
    .set_features = Virtio_set_features,
    .notify       = Virtio_notify,
};

static void *openamp_pthread_entry(void *parameter)
{
    openamp_virtio_device_t dev = (openamp_virtio_device_t)parameter;
    struct virtqueue       *rx_vq;
    int                     ret  = 0;
    struct pollfd           fd_s = {0};

    fd_s.fd     = dev->fd;
    fd_s.events = POLLIN;

    if(rpmsg_virtio_get_role(&dev->rvdev) == RPMSG_HOST)
    {
        rx_vq = dev->rvdev.vdev->vrings_info[0].vq;
    }
    else
    {
        rx_vq = dev->rvdev.vdev->vrings_info[1].vq;
    }

    while(1)
    {
        ret = poll(&fd_s, 1, -1);
        if(ret < 0)
        {
            perror("rpmsg_receive_message: poll failed.\n");
            break;
        }

        if(fd_s.revents & POLLIN)
        {
            virtqueue_notification(rx_vq);
        }
    }

    pthread_exit(NULL);
}

static void openamp_ns_bind_cb(struct rpmsg_device *rdev, const char *name, uint32_t dest)
{
    uint32_t i = 0;

    printf("openamp_ns_bind_cb dest = 0x%X name = %s\n", dest, name);

#ifdef AMP_MASTER
    for(i = 0; i < APP_NODE_MAXNUM; i++)
    {
        if(app_node_group[i].name && (!strcmp(name, app_node_group[i].name)) && app_node_group[i].cb && app_node_group[i].unbind_cb)
        {
            app_node_group[i].dest = dest;
            rpmsg_create_ept(&app_node_group[i].ept, rdev, name, app_node_group[i].src, dest, app_node_group[i].cb, app_node_group[i].unbind_cb);
            break;
        }
    }
#endif /* AMP_MASTER */
}

openamp_app_node_t openamp_app_node_register(const char *name, uint32_t src, rpmsg_ept_cb cb, rpmsg_ns_unbind_cb unbind_cb)
{
    uint32_t i = 0;

    for(i = 0; i < APP_NODE_MAXNUM; i++)
    {
        if(app_node_group[i].name == NULL)
        {
            app_node_group[i].name      = name;
            app_node_group[i].cb        = cb ? cb : NULL;
            app_node_group[i].unbind_cb = unbind_cb ? unbind_cb : NULL;
            app_node_group[i].src       = src;
            return &app_node_group[i];
        }
    }

    return NULL;
}

void openamp_app_node_unregister(const char *name)
{
    uint32_t i = 0;

    for(i = 0; i < APP_NODE_MAXNUM; i++)
    {
        if(!strcmp(name, app_node_group[i].name))
        {
            rpmsg_destroy_ept(&app_node_group[i].ept);
            memset((void *)&app_node_group[i], 0, sizeof(struct openamp_app_node));
            break;
        }
    }
}

openamp_app_node_t openamp_find_app_node(const char *name)
{
    uint32_t i = 0;

    for(i = 0; i < APP_NODE_MAXNUM; i++)
    {
        if(!strcmp(name, app_node_group[i].name))
        {
            return &app_node_group[i];
        }
    }

    return NULL;
}

int openamp_app_send(struct rpmsg_endpoint *ept, const void *data, size_t len)
{
    int ret = -1;

    if(ept == NULL || data == NULL || len == 0)
    {
        return ret;
    }

    if(rpmsg_send(ept, data, len) == len)
    {
        ret = len;
    }

    return ret;
}

int openamp_dev_create(openamp_virtio_device_t dev)
{
    struct metal_io_region *shm_io;
    struct virtqueue       *vq[2];
    pthread_t               tid;
    int                     ret = 0;

    if(dev->dev_name == NULL || dev->shm_mpaddr == NULL || dev->shm_physaddr == NULL || dev->shm_size == 0)
    {
        printf("openamp_dev_create: invalid parameter\n");
        return -1;
    }

    if(dev->secondary_config.cpu_id < 0 || dev->secondary_config.entry == 0 || dev->secondary_config.irq_id < 0)
    {
        printf("openamp_dev_create: invalid secondary config\n");
        return -1;
    }

    if(dev->tx_num == 0 || dev->rx_num == 0)
    {
        printf("openamp_dev_create: invalid vring number\n");
        return -1;
    }

    if(dev->vdev_status_size == 0 || dev->align_size == 0)
    {
        printf("openamp_dev_create: invalid vdev status size or align size\n");
        return -1;
    }

    dev->fd = open(dev->dev_name, O_RDWR);
    if(dev->fd < 0)
    {
        perror("open device failed");
        return -1;
    }

    ret = ioctl(dev->fd, HW_SECONDARY_LOAD_FIRMWARE, &dev->secondary_config);
    if(ret < 0)
    {
        perror("load failed");
        return -1;
    }

    ret = ioctl(dev->fd, HW_SECONDARY_IRQ_INSTALL, &dev->secondary_config);
    if(ret < 0)
    {
        return -1;
    }

    dev->shm_vaddr = (void *)mmap(NULL, dev->shm_size, PROT_READ | PROT_WRITE, MAP_SHARED, dev->fd, (off_t)(dev->shm_mpaddr));
    if(dev->shm_vaddr == MAP_FAILED)
    {
        printf("mmap failed: shared_mem_vaddr: %p\n", dev->shm_vaddr);
        return -EPERM;
    }

    memset((void *)dev->shm_vaddr, 0, dev->shm_size);

    dev->vring_tx  = (void *)ALIGN(dev->shm_vaddr + dev->vdev_status_size, dev->align_size);
    dev->vring_rx  = (void *)ALIGN(dev->vring_tx + (dev->tx_num + 1) * 0x20, dev->align_size);
    dev->vring_buf = (void *)ALIGN(dev->vring_rx + (dev->rx_num + 1) * 0x20, dev->align_size);

    dev->vring_size     = dev->shm_vaddr + dev->shm_size - dev->vring_buf;
    dev->shm_physmap[0] = (unsigned long)dev->shm_physaddr + (dev->vring_buf - dev->shm_vaddr);
    metal_io_init(&dev->io_region, (void *)dev->vring_buf, dev->shm_physmap, dev->shm_size - dev->vdev_status_size, DEFAULT_PAGE_SHIFT, 0, NULL);
    shm_io = &dev->io_region;

    vq[0] = virtqueue_allocate(dev->tx_num);
    if(!vq[0])
    {
        perror("virtqueue_allocate failed to alloc vq[0]");
        return -1;
    }

    vq[1] = virtqueue_allocate(dev->rx_num);
    if(!vq[1])
    {
        perror("virtqueue_allocate failed to alloc vq[1]");
        return -1;
    }

    dev->vrings[0].io             = shm_io;
    dev->vrings[0].info.vaddr     = (void *)(dev->vring_tx);
    dev->vrings[0].info.num_descs = dev->tx_num;
    dev->vrings[0].info.align     = 4;
    dev->vrings[0].vq             = vq[0];

    dev->vrings[1].io             = shm_io;
    dev->vrings[1].info.vaddr     = (void *)(dev->vring_rx);
    dev->vrings[1].info.num_descs = dev->rx_num;
    dev->vrings[1].info.align     = 4;
    dev->vrings[1].vq             = vq[1];

    dev->vdev.role        = openamp_dev_role_get();
    dev->vdev.vrings_num  = 2;
    dev->vdev.func        = &dispatch;
    dev->vdev.vrings_info = &dev->vrings[0];

    rpmsg_virtio_init_shm_pool(&dev->shpool, (void *)dev->vring_buf, (size_t)dev->vring_size);
    rpmsg_init_vdev(&dev->rvdev, &dev->vdev, openamp_ns_bind_cb, shm_io, &dev->shpool);

    ret = pthread_create(&tid, NULL, openamp_pthread_entry, (void *)dev);
    if(ret != 0)
    {
        perror("pthread_create failed");
        exit(1);
    }

#ifdef AMP_SLAVE
    for(uint32_t i = 0; i < APP_NUM; i++)
    {
        if(app_node_group[i].name && app_node_group[i].cb && app_node_group[i].unbind_cb)
        {
            rpmsg_create_ept(app_node_group[i].ept, &dev->rvdev.rdev, app_node_group[i].name, RPMSG_ADDR_ANY, RPMSG_ADDR_ANY, app_node_group[i].cb,
                             app_node_group[i].unbind_cb);
        }
    }
#endif /* AMP_SLAVE */

    if(ioctl(dev->fd, HW_SECONDARY_CPU_DOWNLINE, &dev->secondary_config) < 0)
    {
        perror("Offline cpu error");
        return -1;
    }

    if(ioctl(dev->fd, HW_SECONDARY_CPU_ONLINE, &dev->secondary_config) < 0)
    {
        perror("Online cpu error");
        return -1;
    }

    return 0;
}

int openamp_dev_delete(openamp_virtio_device_t dev)
{
    if(dev->shm_vaddr)
    {
        munmap(dev->shm_vaddr, dev->shm_size);
    }

    if(dev->fd)
    {
        close(dev->fd);
    }

    if(dev->vrings[0].vq)
    {
        metal_free_memory(dev->vrings[0].vq);
    }

    if(dev->vrings[1].vq)
    {
        metal_free_memory(dev->vrings[1].vq);
    }

    return 0;
}
