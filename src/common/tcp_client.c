#include "common/tcp.h"
#include "common/tcp_internal.h"

#include "common/log.h"
#include "common/sock.h"

#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>

/*
 * Open a TCP socket connected to the given addr.
 */
int tcp_connect_async (struct event_main *event_main, int *sockp, struct addrinfo *addr)
{
    struct event *event = NULL;
    int sock;
    int err;

    if ((sock = socket(addr->ai_family, addr->ai_socktype, addr->ai_protocol)) < 0) {
        log_pwarning("socket(%d, %d, %d)", addr->ai_family, addr->ai_socktype, addr->ai_protocol);
        return 1;
    }

    if (event_main) {
        if ((err = sock_nonblocking(sock))) {
            log_warning("sock_nonblocking");
            return err;
        }

        if ((err = event_create(event_main, &event, sock))) {
            log_warning("event_create");
            return err;
        }
    }


    err = sock_connect(sock, addr->ai_addr, addr->ai_addrlen);
    
    if (err > 0 && event) {
        // TODO: timeout
        if (event_yield(event, EVENT_WRITE, NULL)) {
            log_error("event_yield");
            goto error;
        }

        err = sock_error(sock);
    }

    if (err) {
        log_pwarning("sock_connect");
        goto error;
    }

    *sockp = sock;

error:
    if (event)
        event_destroy(event);

    if (err)
        close(sock);

    return err;
}

int tcp_connect (struct event_main *event_main, int *sockp, const char *host, const char *port)
{
    int err;
    struct addrinfo hints = {
        .ai_flags        = 0,
        .ai_family        = AF_UNSPEC,
        .ai_socktype    = SOCK_STREAM,
        .ai_protocol    = 0,
    };
    struct addrinfo *addrs, *addr;

    if ((err = getaddrinfo(host, port, &hints, &addrs))) {
        log_perror("getaddrinfo %s:%s: %s", host, port, gai_strerror(err));
        return -1;
    }

    // pre-set err in case of empty addrs
    err = 1;

    for (addr = addrs; addr; addr = addr->ai_next) {
        log_info("%s:%s: %s...", host, port, sockaddr_str(addr->ai_addr, addr->ai_addrlen));

        if ((err = tcp_connect_async(event_main, sockp, addr))) {
            log_perror("%s:%s: %s", host, port, sockaddr_str(addr->ai_addr, addr->ai_addrlen));
            continue;
        }
    
        log_info("%s:%s: %s <- %s", host, port, sockpeer_str(*sockp), sockname_str(*sockp));

        break;
    }

    freeaddrinfo(addrs);

    return err;
}

int tcp_client (struct event_main *event_main, struct tcp **tcpp, const char *host, const char *port)
{
    int sock;

    if (tcp_connect(event_main, &sock, host, port)) {
        log_pwarning("tcp_create: %s:%s", host, port);
        return -1;
    }
    
    log_debug("%d", sock);
            
    return tcp_create(event_main, tcpp, sock);
}
