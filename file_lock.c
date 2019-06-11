/*
 * Rainbow Crackalack: file_lock.c
 * Copyright (C) 2018-2019  Joe Testa <jtesta@positronsecurity.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms version 3 of the GNU General Public License as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */


/* This contains file handling abstraction functions for when file locking is needed.
 * Windows does not have fcntl(), flock(), or lockf(), so we need to call different
 * functions to accomplish file locking. */

#include "file_lock.h"
#include "misc.h"


/* If append is set to 1, then the file is opened in append mode (similar to mode 'a')
 * in the standard fopen(). */
rc_file rc_fopen(char *filename, int append) {
  rc_file f = NULL;

#ifdef _WIN32
  f = CreateFile(filename, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
  if (f == INVALID_HANDLE_VALUE) {
    f = NULL;
    windows_print_error("CreateFile");
  } else if (append)
    rc_fseek(f, 0, RCSEEK_END);
#else
  if (append)
    f = fopen(filename, "a");
  else
    f = fopen(filename, "r+");

  if (f == NULL)
    perror("fopen");
#endif
  
  return f;
}


/* Returns zero on success. */
int rc_flock(rc_file f) {
  int ret = -1;

#ifdef _WIN32
  OVERLAPPED overlapped;

  memset(&overlapped, 0, sizeof(OVERLAPPED));
  if (LockFileEx(f, LOCKFILE_EXCLUSIVE_LOCK, 0, 0xffffffff, 0xffffffff, &overlapped))
    ret = 0;
  else
    windows_print_error("LockFileEx");
#else
  ret = flock(fileno(f), LOCK_EX);
  if (ret != 0)
    perror("flock");
#endif

  return ret;
}


size_t rc_fread(void *ptr, size_t size, size_t nmemb, rc_file f) {
  size_t ret = -1;

#ifdef _WIN32
  DWORD bytes_read = 0;
  if (!ReadFile(f, ptr, size * nmemb, &bytes_read, NULL))
    windows_print_error("ReadFile");
  else
    ret = bytes_read / size;
#else
  ret = fread(ptr, size, nmemb, f);
#endif
  
  return ret;
}


size_t rc_fwrite(const void *ptr, size_t size, size_t nmemb, rc_file f) {
  size_t ret = -1;

#ifdef _WIN32
  DWORD bytes_written = 0;
  if (!WriteFile(f, ptr, size * nmemb, &bytes_written, NULL))
    windows_print_error("WriteFile");
  else
    ret = bytes_written / size;
#else
  ret = fwrite(ptr, size, nmemb, f);
#endif

  return ret;
}


/* For 'whence' arg, use RCSEEK_* flags.  Returns 0 on success. */
int rc_fseek(rc_file f, long offset, int whence) {
  int ret = -1;

#ifdef _WIN32
  if (SetFilePointer(f, offset, 0, whence) == INVALID_SET_FILE_POINTER)
    windows_print_error("SetFilePointer");
  else
    ret = 0;
#else
  ret = fseek(f, offset, whence);
  if (ret != 0)
    perror("fseek");
#endif

  return ret;
}


long rc_ftell(rc_file f) {

#ifdef _WIN32
  return SetFilePointer(f, 0, 0, RCSEEK_CUR);
#else
  return ftell(f);
#endif
  
}


/* Returns 0 on success. */
int rc_ftruncate(rc_file f, unsigned long length) {
  
#ifdef _WIN32
  int ret = -1;

  if (rc_fseek(f, length, RCSEEK_SET) != 0)
    fprintf(stderr, "Failed to truncate file because seek failed.\n");
  else {
    if (!SetEndOfFile(f))
      windows_print_error("SetEndOfFile");
    else
      ret = 0;
  }

  return ret;
#else
  return ftruncate(fileno(f), length);
#endif

}


void rc_fclose(rc_file f) {
#ifdef _WIN32
  OVERLAPPED overlapped;

  memset(&overlapped, 0, sizeof(OVERLAPPED));
  /* Explicitly unlock the file before closing.  According to the docs, this speeds
   * up any waiting lock requests. */
  if (!UnlockFileEx(f, 0, 0xffffffff, 0xffffffff, &overlapped))
    windows_print_error("UnlockFileEx");

  if (!CloseHandle(f))
    windows_print_error("CloseHandle");
#else
  fclose(f);
#endif
}
