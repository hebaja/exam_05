# C Functions Reference

## `write()`

Writes data to a file descriptor.

```c
#include <unistd.h>

int main(void)
{
    write(1, "Hello\n", 6);
    return (0);
}
```

---

## `close()`

Closes an open file descriptor.

```c
#include <unistd.h>

int main(void)
{
    close(1);
    return (0);
}
```

---

## `select()`

Waits until one or more file descriptors become ready for reading, writing, or exceptional conditions.

### Prototype

```c
int select(int nfds,
           fd_set *readfds,
           fd_set *writefds,
           fd_set *exceptfds,
           struct timeval *timeout);
```

### Useful macros

- `FD_ZERO(&set)` initializes (clears) a file descriptor set.
- `FD_SET(fd, &set)` adds a file descriptor to the set.
- `FD_CLR(fd, &set)` removes a file descriptor from the set.
- `FD_ISSET(fd, &set)` checks whether a file descriptor is ready after `select()` returns.

### Arguments

- **nfds**: Highest file descriptor in any set, plus one.
- **readfds**: File descriptors to monitor for reading.
- **writefds**: File descriptors to monitor for writing (`NULL` if unused).
- **exceptfds**: File descriptors to monitor for exceptional conditions (`NULL` if unused).
- **timeout**:
  - `NULL` → wait indefinitely.
  - Pointer to `struct timeval` → maximum time to wait.

### Example

```c
#include <sys/select.h>

fd_set readfds;

FD_ZERO(&readfds);
FD_SET(0, &readfds);

select(
    1,
    &readfds,
    NULL,
    NULL,
    NULL
);
```

---

## `poll()`

Monitors multiple file descriptors until one or more become ready for I/O.

### Prototype

```c
int poll(struct pollfd *fds, nfds_t nfds, int timeout);
```

### Arguments

- **fds**: Array of `struct pollfd`.
- **nfds**: Number of elements in the array.
- **timeout**:
  - `-1` → wait forever.
  - `0` → don't wait.
  - Positive value → wait that many milliseconds.

Each `pollfd` contains:

```c
struct pollfd {
    int fd;
    short events;
    short revents;
};
```

Common event flags:

- `POLLIN` → data available for reading.
- `POLLOUT` → ready for writing.
- `POLLERR` → an error occurred.

### Example

```c
#include <poll.h>

struct pollfd fds[1];

fds[0].fd = 0;
fds[0].events = POLLIN;

poll(fds, 1, -1);
```

---

## `socket()`

Creates a network communication endpoint.

### Prototype

```c
int socket(int domain, int type, int protocol);
```

### Arguments

- **AF_INET**: IPv4 address family.
- **SOCK_STREAM**: TCP socket (connection-oriented).
- **0**: Uses the default protocol for the given socket type (TCP for `SOCK_STREAM`).

### Example

```c
#include <sys/socket.h>
#include <netinet/in.h>

int sockfd = socket(AF_INET, SOCK_STREAM, 0);
```

---

## `accept()`

Accepts a pending client connection on a listening socket.

### Prototype

```c
int accept(int sockfd,
           struct sockaddr *addr,
           socklen_t *addrlen);
```

### Arguments

- **sockfd**: Listening socket.
- **addr**: Stores the client's address.
- **addrlen**: Size of the address structure.

Passing `NULL` for both `addr` and `addrlen` means you don't need the client's IP address or port.

### Example

```c
int client_fd = accept(server_fd, NULL, NULL);
```

To retrieve the client's address:

```c
struct sockaddr_in client;
socklen_t len = sizeof(client);

int client_fd = accept(
    server_fd,
    (struct sockaddr *)&client,
    &len
);
```

---

## `listen()`

Marks a socket as passive so it can accept incoming connections.

### Prototype

```c
int listen(int sockfd, int backlog);
```

### Arguments

- **sockfd**: Socket created by `socket()` and bound with `bind()`.
- **backlog**: Maximum number of pending connection requests that can wait before `accept()` is called.

### Example

```c
listen(server_fd, 10);
```

Here, `10` allows up to **10 pending client connections** to wait in the queue.

---

## `send()`

Sends data through a connected socket.

### Prototype

```c
ssize_t send(int sockfd,
             const void *buf,
             size_t len,
             int flags);
```

### Arguments

- **sockfd**: Connected socket.
- **buf**: Buffer containing the data to send.
- **len**: Number of bytes to send.
- **flags**: Additional options (`0` means no special flags).

### Example

```c
send(sockfd, "Hello", 5, 0);
```

Here:

- `"Hello"` is the data.
- `5` is the number of bytes to send.
- `0` means a normal send with no special options.

---

## `recv()`

Receives data from a connected socket.

```c
char buffer[1024];

recv(sockfd, buffer, sizeof(buffer), 0);
```

---

## `bind()`

Associates a socket with an IP address and port.

```c
struct sockaddr_in addr;

addr.sin_family = AF_INET;
addr.sin_port = htons(6667);
addr.sin_addr.s_addr = INADDR_ANY;

bind(sockfd, (struct sockaddr *)&addr, sizeof(addr));
```

---

## `strstr()`

Returns a pointer to the first occurrence of a substring.

```c
#include <string.h>

char *str = "Hello World";

char *ptr = strstr(str, "World");
```

---

## `malloc()`

Allocates uninitialized memory.

```c
#include <stdlib.h>

int *arr = malloc(10 * sizeof(int));
```

---

## `realloc()`

Changes the size of previously allocated memory.

```c
arr = realloc(arr, 20 * sizeof(int));
```

---

## `free()`

Releases previously allocated memory.

```c
free(arr);
```

---

## `calloc()`

Allocates zero-initialized memory.

```c
int *arr = calloc(10, sizeof(int));
```

---

## `bzero()`

Sets every byte in a memory block to zero.

```c
#include <strings.h>

char buffer[100];

bzero(buffer, sizeof(buffer));
```

---

## `atoi()`

Converts a string to an integer.

```c
#include <stdlib.h>

int n = atoi("42");
```

---

## `sprintf()`

Writes formatted data into a string.

```c
#include <stdio.h>

char str[100];

sprintf(str, "Value: %d", 42);
```

---

## `strlen()`

Returns the length of a string (excluding the null terminator).

```c
#include <string.h>

size_t len = strlen("Hello");
```

---

## `exit()`

Terminates the program immediately.

```c
#include <stdlib.h>

exit(EXIT_SUCCESS);
```

---

## `strcpy()`

Copies one string into another.

```c
#include <string.h>

char dst[20];

strcpy(dst, "Hello");
```

---

## `strcat()`

Appends one string to another.

```c
#include <string.h>

char str[20] = "Hello ";

strcat(str, "World");
```

---

## `memset()`

Fills a block of memory with a specified byte value.

```c
#include <string.h>

char buffer[100];

memset(buffer, 0, sizeof(buffer));
```

---

## `htons()`

Converts a 16-bit integer from host byte order to network byte order (big-endian).

```c
#include <arpa/inet.h>

uint16_t port = htons(6667);
```

---

## `htonl()`

Converts a 32-bit integer from host byte order to network byte order (big-endian).

```c
#include <arpa/inet.h>

uint32_t addr = htonl(INADDR_ANY);
```
