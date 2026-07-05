# ClaudeC99 Stage 175 Fix compile issue

# Task
The follow program fragment generates an error when compiling that is not seen when compiled with gcc.

```C

     #include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>

int main(int argc, char *argv[]) {
    struct addrinfo hints, *res, *p;
    int status;
    char ipstr[INET6_ADDRSTRLEN];

    if (argc != 2) {
        fprintf(stderr, "usage: showip hostname\n");
        return 1;
    }

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;    // either IPv4 or IPv6
    hints.ai_socktype = SOCK_STREAM;

    if ((status = getaddrinfo(argv[1], NULL, &hints, &res)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(status));
        return 2;
    }

    return 0;

}
```

when Compiling with cc99 i get this
```
cc99 -c showip.c 
/tmp/showip.c:29:51: error: function 'getaddrinfo' parameter '__name' expected pointer argument, got integer
```

How if i compile with gcc i don't see an error.
