/* errno.h */
#ifndef CLAUDEC99_ERRNO_H
#define CLAUDEC99_ERRNO_H

/* Stage 180: macOS libc exposes errno through __error(); glibc through __errno_location(). */
#if defined(__APPLE__)
int *__error(void);
#define errno (*__error())
#else
int *__errno_location(void);
#define errno (*__errno_location())
#endif

#define EPERM   1
#define ENOENT  2
#define ESRCH   3
#define EINTR   4
#define EIO     5
#define ENOMEM  12
#define EACCES  13
#define EINVAL  22
#define ERANGE  34

#endif
