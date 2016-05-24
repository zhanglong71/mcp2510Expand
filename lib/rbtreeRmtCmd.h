#ifndef __RBTREERMTCMD_H__
#define __RBTREERMTCMD_H__

struct RmtCmdNode_s *rmtCmdNode_search(struct rb_root *root, char *addr, cmp_t cmp);
int RmtCmdNode_insert(struct rb_root *root, struct RmtCmdNode_s *data, cmp_t cmp);
//void RmtCmdNode_reset(struct host_s *host);
void RmtCmdNode_reset(struct rb_root *root);
//int rmtCmdNode_move(struct host_s *host, cmp_t cmp);
int rmtCmdNode_move(struct rb_root *toRoot, struct rb_root *fromRoot);

#endif
