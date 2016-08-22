#ifndef __DUMP_UTILS_H__
#define __DUMP_UTILS_H__

void dump2BMP(const char* fname, int w, int h, char* pdata);
int dumpRaw(const char* fname, int len, char* pdata, int scale, int offset);

#endif // __DUMP_UTILS_H__
