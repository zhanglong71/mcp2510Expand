#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/string.h>
#include <linux/rbtree.h>
#include <linux/vmalloc.h>

#include "../mcp2510.h"
#include "rbtreeWhiteList.h"

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("rbtree WhiteNameArea");

/**
 * 返回一个字符串
 *
 * 返回的字符串中, 第一个字符一定不是sep中的字符
 **/
char *whiteNameNode_start(char *src, char *sep)
{
    int i;
    int len;

    if(NULL == src)
    {   
        return NULL;
    }   
    if(NULL == sep)
    {   
        return src;
    }   

    len = strlen(src);
    for(i = 0; i < len; i++)
    {   
        if(NULL == strchr(sep, src[i]))
        {   
            return  src + i;
        }   
    }   

    return  NULL;
}
EXPORT_SYMBOL(whiteNameNode_start);

/**   **/
unsigned char char2x(unsigned char high, unsigned char low)
{
    //unsigned char val;

    /** low **/
    if((low >= '0') && (low <= '9')) 
    {
        low = low - '0';
    }
    else if((low >= 'a') && (low <= 'f')) 
    {
        low = low - 'a' + 10;
    }
    else if((low >= 'A') && (low <= 'F')) 
    {
        low = low - 'A' + 10;
    }
    else
    {
        return 0xff;
    }

    /** high **/
    if((high >= '0') && (high <= '9')) 
    {
        high = high - '0';
    }
    else if((high >= 'a') && (high <= 'f')) 
    {
        high = high - 'a' + 10;
    }
    else if((high >= 'A') && (high <= 'F')) 
    {
        high = high - 'A' + 10;
    }
    else
    {
        return 0xff;
    }

    return ((high << 4) & 0xf0) | (low & 0x0f) ;
}
EXPORT_SYMBOL(char2x);
/** Same as binary sort tree. Nothing to do with rbtree **/
struct whiteListNode_s *whiteNameNode_search(struct rb_root *root, char *addr, cmp_t cmp)
{
    struct rb_node *node;
    struct whiteListNode_s *data;
    int result;

    if((NULL == root) || (NULL == addr) || (NULL == cmp))
    {
        return  NULL;
    }

    node = root->rb_node;

    while (node) {
        data = rb_entry(node, struct whiteListNode_s , node_rb);
        result = cmp(addr, data->ucPhysAddr);

        if (result < 0)
            node = node->rb_left;
        else if (result > 0)
            node = node->rb_right;
        else
            return data;
    }   
    return NULL;
}
EXPORT_SYMBOL(whiteNameNode_search);

int whiteNameNode_insert(struct rb_root *root, struct whiteListNode_s *data, cmp_t cmp)
{
    struct rb_node **new = &(root->rb_node), *parent = NULL;

    /* Figure out where to put new node */
    while (*new) 
    {
        struct whiteListNode_s *this = rb_entry(*new, struct whiteListNode_s , node_rb);
        int result = cmp(data->ucPhysAddr, this->ucPhysAddr);

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

    return 1;
}

EXPORT_SYMBOL(whiteNameNode_insert);

/**
 * DevNode_export
 *
 * copy the NodeData into export, upto size bytes
 *
 * root: the rbtree root
 * export_addr: the address export into
 * size: the MAX. size to be copy
 **/
int whiteListNode_export(struct rb_root *root, char *exportAddr, int size)
{
    struct whiteListNode_s *data;
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

    //*exportAddr = '\0';
    sprintf(exportAddr, "---whiteListNode_export--------------\n");
    for(curr = rb_first(root); NULL != curr; curr = rb_next(curr))
    {
        data = rb_entry(curr, struct whiteListNode_s, node_rb);

        //strcat(exportAddr, "Addr: ");
        for(i = 0; i < FIELD_SIZEOF(struct whiteListNode_s , ucPhysAddr); i++)
        {
            sprintf(tmpAddr, "0x%02x ", data->ucPhysAddr[i]);
            strcat(exportAddr, tmpAddr);
        }
        strcat(exportAddr, "    ");
        strcat(exportAddr, data->ucDispName);
        strcat(exportAddr, "    ");
        strcat(exportAddr, &(data->ucValid));
        strcat(exportAddr, "\n");
    }
    vfree(tmpAddr);
    ulCnt = strlen(exportAddr);

    return  ulCnt;

}

EXPORT_SYMBOL(whiteListNode_export);


