#include "openamp_app.h"

#define ECHO_NAME "echo"

static int echo_cb(struct rpmsg_endpoint *ept, void *data, size_t len, uint32_t src, void *priv)
{
    ((uint8_t *)data)[len] = 0;
#ifdef AMP_MASTER
    printf("cpu#%d echo_cb src:0x%X, len:%d, data:%s\n", 0, src, len, data);
#else
    printf("cpu#%d receive msg len:%d, data:%s\n", 0, len, data);
    openamp_dev_send(ept, data, len);
#endif /* AMP_MASTER */

    return 0;
}

static void echo_unbind_cb(struct rpmsg_endpoint *ept)
{
    printf("echo_unbind_cb ept addr = 0x%X  dest_addr = 0x%X name = %s\n", ept->addr, ept->dest_addr, ept->name);
}

void app_echo_init(void)
{
    struct openamp_app_node *echo_node = NULL;

    echo_node = openamp_app_node_register(ECHO_NAME, RPMSG_ADDR_ANY, echo_cb, echo_unbind_cb);
    if(echo_node == NULL)
    {
        printf("echo_node is NULL\n");
        return;
    }
}

int app_echo_send(char *send_buf)
{
    int                      ret       = -1;
    struct openamp_app_node *echo_node = NULL;

    echo_node = openamp_find_app_node(ECHO_NAME);
    if(echo_node == NULL)
    {
        printf("echo_node is NULL\n");
        return ret;
    }

    if(openamp_app_send(&echo_node->ept, send_buf, strlen(send_buf)) == strlen(send_buf))
    {
        ret = 0;
    }

    return ret;
}
