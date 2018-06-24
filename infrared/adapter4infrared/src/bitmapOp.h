#ifndef __BITMAPOP_H__
#define __BITMAPOP_H__

void bitmap_clean(bitmap_t* bitmap);
void bitmap_setall(bitmap_t* bitmap);
unsigned int bitmap_test_set(bitmap_t* bitmap, int offset);
int bitmap_isfull(bitmap_t* bitmap, int num);

#endif
