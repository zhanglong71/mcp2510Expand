#ifndef __RBTREEDATAAREA_H__
#define __RBTREEDATAAREA_H__



struct canbusDevice_s *DevNode_search(struct rb_root *root, char *string, cmp_t cmp);
int DevNode_insert(struct host_s *host, struct canbusDevice_s *data, cmp_t cmp);

//int DevNode_export(struct rb_root *root, char *exportAddr, int size);

#endif
