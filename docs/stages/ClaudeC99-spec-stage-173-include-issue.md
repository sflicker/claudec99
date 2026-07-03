# ClaudeC99 stage 173 include issue

The following program has a compile error.

```c

#include <arpa/inet.h>
#include <errno.h>
#include <netdb.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

int read_exact(int fd, void *buffer, size_t size)
{
    char *p = buffer;
    size_t done = 0;

    while(done < size) {
        ssize_t n = read(fd, p + done, size - done);
        if (n == 0) {
            return 0;  /* peer disconnected */
        }
        if (n < 0) {
            if (errno == EINTR) {
                continue;
            }
            return -1;
        }
        done += (size_t)n;
    }

    return 1;
}
"net_helpers.c" 34L, 683B                                                                                      1,1           Top
```

When i try and compile with cc99 I'm getting the following error:

cc99 -c net_helpers.c 
error: include file not found: <arpa/inet.h>

Am i doing something wrong or is there a bug. I thought cc99 would look in /usr/include for header files and I checked and arpa is directory in /usr/include and inet.h is in the arpa directory. I throught this should work. Or do i need do something else.