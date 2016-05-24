#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/string.h>
#include <linux/rbtree.h>
#include <linux/vmalloc.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>

#include "../mcp2510.h"
#include "rbtreeDataArea.h"

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("rbtree DataArea");

/** Same as binary sort tree. Nothing to do with rbtree **/
struct canbusDevice_s *DevNode_search(struct rb_root *root, char *string, cmp_t cmp)
{
    struct rb_node *node = root->rb_node;
    struct canbusDevice_s *data;
    int result;

    while (node) {
        data = container_of(node, struct canbusDevice_s, node_rb);
        result = cmp(string, data->stIdMsg.ucPhysAddr);

        if (result < 0)
            node = node->rb_left;
        else if (result > 0)
            node = node->rb_right;
        else
            return data;
    }   
    return NULL;
}
EXPORT_SYMBOL(DevNode_search);

//int DevNode_insert(struct rb_root *root, struct canbusDevice_s *data, cmp_t cmp)
int DevNode_insert(struct host_s *host, struct canbusDevice_s *data, cmp_t cmp)
{
    struct rb_root *root = &(host->DevInfoHead.DevInfoRoot_rb);
    struct rb_node **new = &(root->rb_node), *parent = NULL;

    /* Figure out where to put new node */
    while (*new) 
    {
        struct canbusDevice_s *this = container_of(*new, struct canbusDevice_s, node_rb);
        int result = cmp(data->stIdMsg.ucPhysAddr, this->stIdMsg.ucPhysAddr);

        parent = *new;
        if (result < 0)
            new = &((*new)->rb_left);
        else if (result > 0)
            new = &((*new)->rb_right);
        else
            return 0;
    }

    /* Add new node and rebalance tree. */
    rb_link_node(&(data->node_rb), parent, new);
    rb_insert_color(&(data->node_rb), root);

    host->DevInfoHead.iCount++;

    return 1;
}

EXPORT_SYMBOL(DevNode_insert);

#if 0
/**
 * DevNode_export
 *
 * copy the NodeData into export, upto size bytes
 *
 * root: the rbtree root
 * export_addr: the address export into
 * size: the MAX. size to be copy
 **/
int DevNode_export(struct rb_root *root, char *exportAddr, int size)
{
    struct canbusDevice_s *data;
    char *tmpAddr;
    struct rb_node *curr;
    unsigned long ulCnt = 0;
    int i = 0;

    if((NULL == root) || (NULL == exportAddr))
    {
        return  0;
    }
    if(NULL == (tmpAddr = vmalloc(PAGE_SIZE)))
    {
        return  0;
    }

    sprintf(exportAddr, "---DevNode_export--------------\n");

    for(curr = rb_first(root); NULL != curr; curr = rb_next(curr))
    {
        data = rb_entry(curr, struct canbusDevice_s, node_rb);

        strcat(exportAddr, "Addr: ");
        for(i = 0; i < FIELD_SIZEOF(struct IdentifyMsg_s, ucPhysAddr); i++)
        {
            sprintf(tmpAddr, "0x%02x ", data->stIdMsg.ucPhysAddr[i]);
            strcat(exportAddr, tmpAddr);
        }
        strcat(exportAddr, "\n");

        sprintf(tmpAddr, "Name: %s", data->ucDispName);
        strcat(exportAddr, tmpAddr);
        strcat(exportAddr, "\n");

        sprintf(tmpAddr, "Vendor: 0x%04x ", data->stIdMsg.usVendorNo);
        strcat(exportAddr, tmpAddr);

        sprintf(tmpAddr, "Product: 0x%04x ", data->stIdMsg.usProductNo);
        strcat(exportAddr, tmpAddr);

        sprintf(tmpAddr, "DevType: 0x%04x ", data->stIdMsg.usDevType);
        strcat(exportAddr, tmpAddr);

        sprintf(tmpAddr, "Y/M/D: %d-%d-%d ", 
                data->stIdMsg.usYear,
                data->stIdMsg.ucMonth,
                data->stIdMsg.ucDay
                );
        strcat(exportAddr, tmpAddr);

        strcat(exportAddr, "other: ");
        //for(i = 0; i < FIELD_SIZEOF(struct IdentifyMsg_s, other); i++)
        for(i = 0; i < ARRAY_SIZE(data->stIdMsg.other); i++)
        {
            sprintf(tmpAddr, "0x%02x ", data->stIdMsg.other[i]);
            strcat(exportAddr, tmpAddr);
        }

        strcat(exportAddr, "\nusAttr: ");
        //for(i = 0; i < FIELD_SIZEOF(struct canbusDevice_s, usAttr); i++)
        for(i = 0; i < ARRAY_SIZE(data->usAttr); i++)
        {
            sprintf(tmpAddr, "0x%04x ", data->usAttr[i]);
            strcat(exportAddr, tmpAddr);
        }

        sprintf(tmpAddr, "\nucIdValid: 0x%02x ", data->ucIdValid);
        strcat(exportAddr, tmpAddr);

        sprintf(tmpAddr, "ucStatValid: 0x%02x ", data->ucStatValid);
        strcat(exportAddr, tmpAddr);

        sprintf(tmpAddr, "ulJiffies: 0x%08lx ", data->ulJiffies);
        strcat(exportAddr, tmpAddr);

        sprintf(tmpAddr, "ucAdapterNo: 0x%02x ", data->ucAdapterNo);
        strcat(exportAddr, tmpAddr);

        sprintf(tmpAddr, "ucCmdOpcode: 0x%02x ", data->ucCmdOpcode);
        strcat(exportAddr, tmpAddr);

        sprintf(tmpAddr, "ucCmdAttr: 0x%02x ", data->ucCmdAttr);
        strcat(exportAddr, tmpAddr);

        sprintf(tmpAddr, "usCmdValue: 0x%02x ", data->usCmdValue);
        strcat(exportAddr, tmpAddr);

        sprintf(tmpAddr, "ucCmdCnt: 0x%02x ", data->ucCmdCnt);
        strcat(exportAddr, tmpAddr);

        strcat(exportAddr, "\n---next DevNode --------------\n");
    }
    vfree(tmpAddr);
    ulCnt = strlen(exportAddr);

    return  ulCnt;

}

EXPORT_SYMBOL(DevNode_export);
#endif

