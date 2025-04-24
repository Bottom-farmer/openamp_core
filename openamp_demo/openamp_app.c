#include "openamp_app.h"

#define AMP_DEV_NAME                     "/dev/hw_internuclear"
#define AMP_SHARE_MEMORY_ADDRESS         0x99000000
#define AMP_SHARE_MEMORY_PHYSICS_ADDRESS 0x8000000099000000
#define AMP_SHARE_MEMORY_SIZE            0x10000
#define AMP_VDEV_STATUS_SIZE             0x1000
#define AMP_VDEV_ALIGN_SIZE              0x1000
#define AMP_DEMO_VRING_TX_DES_NUM        16
#define AMP_DEMO_VRING_RX_DES_NUM        16

#define AMP_AECONDARY_FIRMWARE           "./rtthread.bin"
#define AMP_SLAVE_CPU_ID                 1
#define AMP_SLAVE_ENTRY                  0x97000000
#define AMP_SLAVE_IRQ_ID                 7

static struct openamp_virtio_device app = {
    .vdev_status_size           = AMP_VDEV_STATUS_SIZE,
    .align_size                 = AMP_VDEV_ALIGN_SIZE,
    .tx_num                     = AMP_DEMO_VRING_TX_DES_NUM,
    .rx_num                     = AMP_DEMO_VRING_RX_DES_NUM,
    .shm_mpaddr                 = (void *)AMP_SHARE_MEMORY_ADDRESS,
    .shm_physaddr               = (void *)AMP_SHARE_MEMORY_PHYSICS_ADDRESS,
    .shm_size                   = AMP_SHARE_MEMORY_SIZE,
    .dev_name                   = AMP_DEV_NAME,
    .secondary_config.cpu_id    = AMP_SLAVE_CPU_ID,
    .secondary_config.entry     = AMP_SLAVE_ENTRY,
    .secondary_config.irq_id    = AMP_SLAVE_IRQ_ID,
    .secondary_config.file_path = AMP_AECONDARY_FIRMWARE,
};

int main(int argc, char **argv)
{
    char cmd_buf[32]   = {0};
    char data_buf[256] = {0};

    openamp_dev_create(&app);
    app_echo_init();

    sleep(1);
    printf("openamp_app_init done\n");

    while(1)
    {
        printf("\n\ncommand: echo <data>\n");
        scanf("%s%s", cmd_buf, data_buf);
        if(!strcmp(cmd_buf, "echo"))
        {
            printf("send '%s' to sceondary\n", data_buf);
            app_echo_send(data_buf);
        }

        usleep(10000);
    }
}
