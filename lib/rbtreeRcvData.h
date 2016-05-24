#ifndef __RBTREERCVDATA_H__
#define __RBTREERCVDATA_H__


//struct RcvCanFrm_s *RcvCanFrm_search(struct rb_root *root, char *string, cmp_t cmp);
int RcvCanFrm_insert(struct rb_root *root, struct RcvCanFrm_s *data, cmp_t cmp);

struct rb_node* RcvCanFrm_complete(struct rb_root *root);
int RcvCanFrm_erase(struct rb_root *root, struct rb_node* start, int __iCnt);
void RcvCanFrm_reset(struct rb_root *root);

void RcvCanFrm_print(const char* string, int lineNo, struct rb_root *root, print_t print);

#endif
