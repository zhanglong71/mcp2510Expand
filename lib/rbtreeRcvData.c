#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/string.h>
#include <linux/rbtree.h>

#include "../mcp2510.h"
#include "rbtreeRcvData.h"

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("rbtree RcvData");

#if 0
/** Same as binary sort tree **/
struct RcvCanFrm_s *RcvCanFrm_search(struct rb_root *root, char *string, cmp_t cmp)
{
    struct rb_node *node = root->rb_node;
    struct RcvCanFrm_s *data;
    int result;

    while (node) {
        data = container_of(node, struct RcvCanFrm_s, node_rb);
        result = cmp(string, &(data->stRcvCanFrm.Messages[2]));

        if (result < 0)
            node = node->rb_left;
        else if (result > 0)
            node = node->rb_right;
        else
            return data;
    }   
    return NULL;
}
EXPORT_SYMBOL(RcvCanFrm_search);
#endif

int RcvCanFrm_insert(struct rb_root *root, struct RcvCanFrm_s *data, cmp_t cmp)
{
    struct rb_node **new = &(root->rb_node), *parent = NULL;

    /* Figure out where to put new node */
    while (*new) 
    {
        struct RcvCanFrm_s *this = container_of(*new, struct RcvCanFrm_s, node_rb);
        int result = cmp(&(data->stRcvCanFrm.Messages[2]), &(this->stRcvCanFrm.Messages[2]));

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
EXPORT_SYMBOL(RcvCanFrm_insert);

/************************************************************************************
 *  Function:
 *
 * input: root of rbtree
 * output:
 * return: the first can frame node of complete packet
 *
 * descript: check can frame data weather they are series can frame NO. in seq and canseq
 *
 ************************************************************************************/
//struct rb_node* RcvCanFrm_complete(struct rb_root *root, int* iCnt)
struct rb_node* RcvCanFrm_complete(struct rb_root *root)
{
    int iCnt = 0;
    struct rb_node *prev;
    struct rb_node *curr;
    struct rb_node *start;

    unsigned char startSeq;
    unsigned char prevSeq;
    unsigned char currSeq;

    unsigned char prevCanseq;
    unsigned char currCanseq;

    curr = rb_first(root);
    if(NULL == curr)
    {
        return  NULL;
    }

    start = curr;
    startSeq = ((rb_entry(start, struct RcvCanFrm_s, node_rb))->stRcvCanFrm).Messages[3];
    for(prev = curr, curr = rb_next(curr); curr; prev = curr, curr = rb_next(curr))
    {
        prevSeq = ((rb_entry(prev, struct RcvCanFrm_s, node_rb))->stRcvCanFrm).Messages[3];
        currSeq = ((rb_entry(curr, struct RcvCanFrm_s, node_rb))->stRcvCanFrm).Messages[3];

        prevCanseq = ((rb_entry(prev, struct RcvCanFrm_s, node_rb))->stRcvCanFrm).Messages[2];
        currCanseq = ((rb_entry(curr, struct RcvCanFrm_s, node_rb))->stRcvCanFrm).Messages[2];

        if((((prevSeq + 1) & 0xff) == currSeq) && (((prevCanseq + 1) & 0xff) == currCanseq))    /** 是连续的can帧吗？**/
        {
            iCnt++;
            if(iCnt + 1 == ((((rb_entry(prev, struct RcvCanFrm_s, node_rb))->stRcvCanFrm).Messages[1] + 3) >> 2)) 
            {
#if 0
                if(0 != startSeq)
                {
                    printk("[%s-%d]Warn: start seq is not 0 !\n", __FILE__, __LINE__);
                }
#endif
                return  start;
            }
        }
        else
        {
            iCnt = 0;
            start = prev;
            startSeq = ((rb_entry(start, struct RcvCanFrm_s, node_rb))->stRcvCanFrm).Messages[3];
        }
    }

    return NULL;
}

EXPORT_SYMBOL(RcvCanFrm_complete);

/**********************************************************************************
 * iCnt can frames that in series will be deleted which start 
 *
 * input: iCnt - the number of node you will be delete
 *        start - the first node pointer you will be delete
 *        root - the root of rbtree which will delete node from 
 * output: 
 * return : > 0: the number of node that have been deleted
 *          -1 - error
 *
 **********************************************************************************/
int RcvCanFrm_erase(struct rb_root *root, struct rb_node* start, int __iCnt)
{
    int iCnt = 0;
    struct rb_node *curr;
    struct RcvCanFrm_s *data;

    unsigned char startSeq;
    unsigned char currSeq;

    unsigned char startCanseq;
    unsigned char currCanseq;

    if((NULL == root) || (NULL == start) || (2 > __iCnt))
    {
        return  0;
    }

    startSeq = ((rb_entry(start, struct RcvCanFrm_s, node_rb))->stRcvCanFrm).Messages[3];
    startCanseq = ((rb_entry(start, struct RcvCanFrm_s, node_rb))->stRcvCanFrm).Messages[2];

    while(NULL != (curr = rb_next(start)))
    {
        currSeq = ((rb_entry(curr, struct RcvCanFrm_s, node_rb))->stRcvCanFrm).Messages[3];
        currCanseq = ((rb_entry(curr, struct RcvCanFrm_s, node_rb))->stRcvCanFrm).Messages[2];

        if((((startSeq + iCnt + 1) & 0xff) == currSeq) && (((startCanseq + iCnt + 1) & 0xff) == currCanseq))    /** 是连续的can帧吗？**/
        {
            rb_erase(curr, root);
            data = container_of(curr, struct RcvCanFrm_s, node_rb);
            kfree(data);
            iCnt++;
        }
        else
        {
            rb_erase(start, root);
            data = container_of(start, struct RcvCanFrm_s, node_rb);
            kfree(data);
            iCnt++;
            return iCnt;
        }
    }

    rb_erase(start, root);
    data = container_of(start, struct RcvCanFrm_s, node_rb);
    kfree(data);
    iCnt++;

    return iCnt;
}
EXPORT_SYMBOL(RcvCanFrm_erase);

/**********************************************************************************
 * print all message on root
 *
 *
 **********************************************************************************/
void RcvCanFrm_print(const char* string, int lineNo, struct rb_root *root, print_t print)
{
    struct rb_node *node;
    struct RcvCanFrm_s *data;

    for(node = rb_first(root); node; node = rb_next(node))
    {
        data = container_of(node, struct RcvCanFrm_s, node_rb);
        print(string, lineNo, &(data->stRcvCanFrm));
    }
}

EXPORT_SYMBOL(RcvCanFrm_print);


void RcvCanFrm_reset(struct rb_root *root)
{
    struct rb_node *node;
    struct RcvCanFrm_s *data;

    if(NULL == root)
    {
        return;
    }

    while(NULL != (node = rb_first(root)))
    {
        data = container_of(node, struct RcvCanFrm_s, node_rb);
        rb_erase(&(data->node_rb), root);
        kfree(data);
    }

    return;
}
EXPORT_SYMBOL(RcvCanFrm_reset);


