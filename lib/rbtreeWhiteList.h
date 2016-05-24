#ifndef __RBTREEWHITELIST_H__
#define __RBTREEWHITELIST_H__

/**
 * 返回一个字符串
 *
 * 返回的字符串中, 第一个字符一定不是sep中的字符
 **/
char *whiteNameNode_start(char *src, char *sep);

unsigned char char2x(unsigned char low, unsigned char high);
/** Same as binary sort tree. Nothing to do with rbtree **/
struct whiteListNode_s *whiteNameNode_search(struct rb_root *root, char *addr, cmp_t cmp);

int whiteNameNode_insert(struct rb_root *root, struct whiteListNode_s *data, cmp_t cmp);

int whiteListNode_export(struct rb_root *root, char *exportAddr, int size);
#endif
