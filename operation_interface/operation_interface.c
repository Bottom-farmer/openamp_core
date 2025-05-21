#include "operation_interface.h"

#define DEFAULT_PAGE_SHIFT (-1U)
#define DEFAULT_PAGE_MASK  (-1U)

static struct openamp_app_node app_node_head = {
    .name      = {0},
    .cb        = NULL,
    .unbind_cb = NULL,
    .src       = 0,
    .dest      = 0,
    .node_list = METAL_INIT_LIST(app_node_head.node_list),
};

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
    openamp_virtio_device_t dev = NULL;

    if(vdev == NULL)
    {
        return;
    }

    dev = metal_container_of(vdev, struct openamp_virtio_device, vdev);

    return *(volatile unsigned char *)(dev->shm_vaddr);
}

static void Virtio_set_status(struct virtio_device *vdev, uint8_t status)
{
    openamp_virtio_device_t dev = NULL;

    if(vdev == NULL)
    {
        return;
    }

    dev = metal_container_of(vdev, struct openamp_virtio_device, vdev);

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

    if(vq == NULL)
    {
        return;
    }

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

static int default_bind_cb(struct rpmsg_endpoint *ept, void *data, size_t len, uint32_t src, void *priv)
{
    ((uint8_t *)data)[len] = 0;

    printf("default_cb cpu#%d src:0x%X, len:%d, data:%s\n", 0, src, len, data);
}

static void default_unbind_cb(struct rpmsg_endpoint *ept)
{
    printf("default_uncb ept addr = 0x%X  dest_addr = 0x%X name = %s\n", ept->addr, ept->dest_addr, ept->name);
    openamp_app_node_unregister(ept->name);
}

static void openamp_ns_bind_cb(struct rpmsg_device *rdev, const char *name, uint32_t dest)
{
    openamp_app_node_t app_node  = NULL;
    struct metal_list *node_list = NULL;
    openamp_app_node_t trav_node = NULL;

    if(name == NULL || rdev == NULL)
    {
        return;
    }

    app_node = metal_allocate_memory(sizeof(struct openamp_app_node));
    if(app_node == NULL)
    {
        printf("app_node malloc failed\n");
        return;
    }

    memset((void *)app_node, 0, sizeof(struct openamp_app_node));

    metal_list_for_each(&app_node_head.node_list, node_list)
    {
        trav_node = metal_container_of(node_list, struct openamp_app_node, node_list);
        if(trav_node == NULL)
        {
            printf("app_node is NULL\n");
            break;
        }

        if(!strcmp(name, trav_node->name))
        {
            metal_free_memory(app_node);
            printf("app_node name is already exist\n");
            return;
        }
    }

    strncpy(app_node->name, name, OPENAMP_APP_NODE_NAME_MAXLEN);
    app_node->dest      = dest;
    app_node->cb        = default_bind_cb;
    app_node->unbind_cb = default_unbind_cb;
    metal_list_add_head(&app_node_head.node_list, &app_node->node_list);

    rpmsg_create_ept(&app_node->ept, rdev, app_node->name, RPMSG_ADDR_ANY, RPMSG_ADDR_ANY, app_node->cb, app_node->unbind_cb);
    app_node->ept.dest_addr = dest;

    printf("\n(%s) app node default cb/uncb function is currently in use, and the default can be modified using the register function!\n",
           app_node->name);
}

openamp_app_node_t openamp_app_node_register(const char *name, rpmsg_ept_cb cb, rpmsg_ns_unbind_cb unbind_cb, openamp_virtio_device_t dev)
{
    openamp_app_node_t app_node  = NULL;
    openamp_app_node_t trav_node = NULL;
    struct metal_list *node_list = NULL;
    char               node_flag = 0;

    if(name == NULL || dev == NULL)
    {
        printf("app_node name or dev is NULL\n");
        return NULL;
    }

    if(cb == NULL || unbind_cb == NULL)
    {
        printf("app_node cb/unbind_cb is NULL\n");
        return NULL;
    }

    app_node = metal_allocate_memory(sizeof(struct openamp_app_node));
    if(app_node == NULL)
    {
        printf("app_node malloc failed\n");
        return NULL;
    }

    memset((void *)app_node, 0, sizeof(struct openamp_app_node));

    strncpy(app_node->name, name, OPENAMP_APP_NODE_NAME_MAXLEN);

    metal_list_for_each(&app_node_head.node_list, node_list)
    {
        trav_node = metal_container_of(node_list, struct openamp_app_node, node_list);
        if(trav_node == NULL)
        {
            printf("app_node is NULL\n");
            break;
        }

        if(!strcmp(name, trav_node->name))
        {
            metal_free_memory(app_node);
            if(trav_node->cb == NULL && trav_node->unbind_cb == NULL)
            {
                printf("trav_node cb/unbind_cb is NULL\n");
                return trav_node;
            }

            if(trav_node->cb == cb && trav_node->unbind_cb == unbind_cb)
            {
                printf("app_node name is already exist\n");
                return trav_node;
            }
            else
            {
                trav_node->ept.cb           = cb;
                trav_node->ept.ns_unbind_cb = unbind_cb;
                printf("(%s) node change cb/unbind_cb\n", trav_node->name);
                return trav_node;
            }
        }
    }

    app_node->cb        = cb;
    app_node->unbind_cb = unbind_cb;

    metal_list_add_head(&app_node_head.node_list, &app_node->node_list);
    rpmsg_create_ept(&app_node->ept, &dev->rvdev.rdev, app_node->name, RPMSG_ADDR_ANY, RPMSG_ADDR_ANY, app_node->cb, app_node->unbind_cb);

    return app_node;
}

int openamp_app_node_unregister(const char *name)
{
    openamp_app_node_t app_node  = NULL;
    struct metal_list *node_list = NULL;

    if(name == NULL)
    {
        return -1;
    }

    metal_list_for_each(&app_node_head.node_list, node_list)
    {
        app_node = metal_container_of(node_list, struct openamp_app_node, node_list);
        if(app_node == NULL)
        {
            printf("app_node is NULL\n");
            return -1;
        }

        if(!strcmp(name, app_node->name))
        {
            rpmsg_destroy_ept(&app_node->ept);
            metal_list_del(&app_node->node_list);
            metal_free_memory(app_node);

            return 0;
        }
    }

    return -1;
}

openamp_app_node_t openamp_find_app_node(const char *name)
{
    openamp_app_node_t app_node  = NULL;
    struct metal_list *node_list = NULL;

    if(name == NULL)
    {
        return NULL;
    }

    metal_list_for_each(&app_node_head.node_list, node_list)
    {
        app_node = metal_container_of(node_list, struct openamp_app_node, node_list);
        if(app_node == NULL)
        {
            printf("app_node is NULL\n");
            break;
        }

        if(!strcmp(name, app_node->name))
        {
            return app_node;
        }
    }

    return NULL;
}

void openamp_dump_app_node(void)
{
    openamp_app_node_t app_node  = NULL;
    struct metal_list *node_list = NULL;

    metal_list_for_each(&app_node_head.node_list, node_list)
    {
        app_node = metal_container_of(node_list, struct openamp_app_node, node_list);
        if(app_node == NULL)
        {
            printf("app_node is NULL\n");
            break;
        }

        if(app_node->cb == NULL || app_node->unbind_cb == NULL)
        {
            continue;
        }

        printf("Dump app node name:%s src:%x dest:%x cb:%x unbind_cb:%x\n", app_node->name, app_node->ept.addr, app_node->ept.dest_addr,
               app_node->ept.cb, app_node->ept.ns_unbind_cb);
    }
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

openamp_virtio_device_t openamp_dev_get(void)
{
    if(opdev == NULL)
    {
        printf("openamp_dev_get dev is not created\n");
    }

    return opdev;
}

int openamp_dev_delete(openamp_virtio_device_t dev)
{
    if(dev == NULL)
    {
        printf("openamp_dev_delete dev is NULL\n");
        return -1;
    }

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
