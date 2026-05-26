/* errno.h */
#ifndef CLAUDEC99_ERRNO_H
#define CLAUDEC99_ERRNO_H

int *__errno_location(void);

#define errno (*__errno_location())

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
