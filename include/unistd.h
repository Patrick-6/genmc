#ifndef __UNISTD_H__
#define __UNISTD_H__

#include <pthread.h>
#include <stddef.h>
#include <sys/types.h>

/* src/Execution.cpp is based on the values of the definitions below */
#define SEEK_SET	0	/* Seek from beginning of file.  */
#define SEEK_CUR	1	/* Seek from current position.  */
#define SEEK_END	2	/* Seek from end of file.  */
/* #define SEEK_DATA	3	/\* Seek to next data.  *\/ */
/* #define SEEK_HOLE	4	/\* Seek to next hole.  *\/ */

/* Move FD's file position to OFFSET bytes from the
   beginning of the file (if WHENCE is SEEK_SET),
   the current position (if WHENCE is SEEK_CUR),
   or the end of the file (if WHENCE is SEEK_END).
   Return the new file position.  */
extern __off_t lseek (int __fd, __off_t __offset, int __whence);

/* Close the file descriptor FD.

   This function is a cancellation point and therefore not marked with
   __THROW.  */
extern int close (int __fd);

/* Read NBYTES into BUF from FD.  Return the
   number read, -1 for errors or 0 for EOF.

   This function is a cancellation point and therefore not marked with
   __THROW.  */
extern ssize_t read (int __fd, void *__buf, size_t __nbytes);

/* Write N bytes of BUF to FD.  Return the number written, or -1.

   This function is a cancellation point and therefore not marked with
   __THROW.  */
extern ssize_t write (int __fd, const void *__buf, size_t __n);

/* Read NBYTES into BUF from FD at the given position OFFSET without
   changing the file pointer.  Return the number read, -1 for errors
   or 0 for EOF.

   This function is a cancellation point and therefore not marked with
   __THROW.  */
extern ssize_t pread (int __fd, void *__buf, size_t __nbytes,
		      __off_t __offset);

/* Write N bytes of BUF to FD at the given position OFFSET without
   changing the file pointer.  Return the number written, or -1.

   This function is a cancellation point and therefore not marked with
   __THROW.  */
extern ssize_t pwrite (int __fd, const void *__buf, size_t __n,
		       __off_t __offset);

/* Make a link to FROM named TO.  */
extern int link (const char *__from, const char *__to);

/* Remove the link NAME.  */
extern int unlink (const char *__name);

/* Make all changes done to FD actually appear on disk.
   This function is a cancellation point and therefore not marked with
   __THROW.  */
extern int fsync (int __fd);


/* Make all changes done to all files actually appear on disk.  */
extern void sync (void);

/* Truncate FILE to LENGTH bytes.  */
extern int truncate (const char *__file, __off_t __length);


/*
 * ******** GENMC RESERVED NAMESPACE ********
 */

#ifndef __CONFIG_GENMC_INODE_DATA_SIZE
# error "Internal error: Inode size not defined!"
#endif

struct __genmc_inode {
	pthread_mutex_t lock; // setupFsInfo() relies on the layout
	int isize;
	char data[__CONFIG_GENMC_INODE_DATA_SIZE];
};

struct __genmc_file {
	pthread_mutex_t lock;
	struct inode *inode;
	int offset;
};

struct __genmc_inode __attribute((address_space(42))) __genmc_dir_inode;
struct __genmc_file __attribute((address_space(42))) __genmc_dummy_file;

#endif /* __UNISTD_H__ */
