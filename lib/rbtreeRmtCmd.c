#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/string.h>
#include <linux/rbtree.h>
#include <linux/vmalloc.h>

#include "../mcp2510.h"
#include "rbtreeRmtCmd.h"

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("rbtree RmtCmd");

/** Same as binary sort tree. Nothing to do with rbtree **/
//struct RmtCmdNode_s *rmtCmdNode_search(struct host_s *host, char *addr, cmp_t cmp)
struct RmtCmdNode_s *rmtCmdNode_search(struct rb_root *root, char *addr, cmp_t cmp)
{
    //struct rb_node *node = host->stRmtCmdList.RcvSimplxRmtCmdRoot_rb.rb_node;
    struct rb_node *node = root->rb_node;
    struct RmtCmdNode_s *data;
    int result;

    while (node) {
        data = rb_entry(node, struct RmtCmdNode_s, node_rb);
        result = cmp(addr, data->devNode->stIdMsg.ucPhysAddr);

        if (result < 0)
            node = node->rb_left;
        else if (result > 0)
            node = node->rb_right;
        else
            return data;
    }   
    return NULL;
}
EXPORT_SYMBOL(rmtCmdNode_search);

//int RmtCmdNode_insert(struct host_s *host, struct RmtCmdNode_s *data, cmp_t cmp)
int RmtCmdNode_insert(struct rb_root *root, struct RmtCmdNode_s *data, cmp_t cmp)
{
    //struct rb_root *root = &(host->stRmtCmdList.RcvSimplxRmtCmdRoot_rb);
    struct rb_node **new = &(root->rb_node), *parent = NULL;

    if(data->devNode == NULL)   /** check weather data field attached or not ? **/
    {
        return 0;
    }
    /* Figure out where to put new node */
    while (*new) 
    {
        struct RmtCmdNode_s  *this = rb_entry(*new, struct RmtCmdNode_s, node_rb);
        int result = cmp(data->devNode->stIdMsg.ucPhysAddr, this->devNode->stIdMsg.ucPhysAddr);

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
EXPORT_SYMBOL(RmtCmdNode_insert);


//void RmtCmdNode_reset(struct host_s *host)
void RmtCmdNode_reset(struct rb_root *root)
{
    struct rb_node *node;
    struct RmtCmdNode_s *data;
    //struct rb_root *root = &(host->stRmtCmdList.DoingRmtCmdRoot_rb);

    if(NULL == root)
    {
        return;
    }

    while(NULL != (node = rb_first(root)))
    {
        data = rb_entry(node, struct RmtCmdNode_s , node_rb);
        rb_erase(&(data->node_rb), root);
        kfree(data);
    }

    return;
}
EXPORT_SYMBOL(RmtCmdNode_reset);

//int rmtCmdNode_move(struct host_s *host, cmp_t cmp)
int rmtCmdNode_move(struct rb_root *toRoot, struct rb_root *fromRoot)
{
    //struct rb_root *fromRoot = &(host->stRmtCmdList.RcvSimplxRmtCmdRoot_rb);
    //struct rb_root *toRoot = &(host->stRmtCmdList.DoingRmtCmdRoot_rb);

#if 0
    struct RmtCmdNode_s *data;
    struct rb_node *node;

    while(NULL != (node = rb_first(fromRoot)))
    {
        struct rb_node **new = &(toRoot->rb_node), *parent = NULL;

        /** delete from ... **/
        data = rb_entry(node, struct RmtCmdNode_s , node_rb);
        rb_erase(&(data->node_rb), fromRoot);

        /**
         * joined into ...
         *
         * Figure out where to put new node
         *
         **/
        while (*new) 
        {
            struct RmtCmdNode_s  *this = rb_entry(*new, struct RmtCmdNode_s, node_rb);
            int result = cmp(data->devNode->stIdMsg.ucPhysAddr, this->devNode->stIdMsg.ucPhysAddr);

            parent = *new;
            if (result < 0)
                new = &((*new)->rb_left);
            else if (result > 0)
                new = &((*new)->rb_right);
          #if 0
            else
                return 0;
          #endif
        }

        /* Add new node and rebalance tree. */
        rb_link_node(&(data->node_rb), parent, new);
        rb_insert_color(&(data->node_rb), toRoot);

        //return 1;
    }
#else
    memcpy(toRoot, fromRoot, sizeof(struct rb_root));
    *fromRoot = RB_ROOT;
#endif
    return  1;
}
EXPORT_SYMBOL(rmtCmdNode_move);

