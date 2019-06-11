#ifndef _FILE_LOCK
#define _FILE_LOCK

#include <stdio.h>


#ifdef _WIN32
#include <windows.h>
typedef HANDLE rc_file;
#define RCSEEK_SET FILE_BEGIN
#define RCSEEK_CUR FILE_CURRENT
#define RCSEEK_END FILE_END
#else
#include <sys/file.h>
#include <unistd.h>
typedef FILE * rc_file;
#define RCSEEK_SET SEEK_SET
#define RCSEEK_CUR SEEK_CUR
#define RCSEEK_END SEEK_END
#endif


rc_file rc_fopen(char *filename, int append);
int rc_flock(rc_file f);
size_t rc_fread(void *ptr, size_t size, size_t nmemb, rc_file f);
size_t rc_fwrite(const void *ptr, size_t size, size_t nmemb, rc_file f);
int rc_fseek(rc_file f, long offset, int whence);
long rc_ftell(rc_file f);
int rc_ftruncate(rc_file f, unsigned long length);
void rc_fclose(rc_file f);


#endif
