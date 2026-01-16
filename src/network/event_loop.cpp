#include "tiny_sql/network/event_loop.h"

#ifdef __linux__
#include "tiny_sql/network/epoll_event_loop.h"
#elif defined(__APPLE__) || defined(__FreeBSD__) || defined(__OpenBSD__) || defined(__NetBSD__)
#include "tiny_sql/network/kqueue_event_loop.h"
#endif

#include "tiny_sql/common/logger.h"

namespace tiny_sql {

EventLoop* createEventLoop() {
#ifdef __linux__
    LOG_INFO("Creating Epoll event loop (Linux)");
    return new EpollEventLoop();
#elif defined(__APPLE__) || defined(__FreeBSD__) || defined(__OpenBSD__) || defined(__NetBSD__)
    LOG_INFO("Creating Kqueue event loop (macOS/BSD)");
    return new KqueueEventLoop();
#else
    #error "Unsupported platform: neither epoll nor kqueue available"
    return nullptr;
#endif
}

} // namespace tiny_sql
