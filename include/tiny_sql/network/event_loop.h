#pragma once

#include <cstdint>
#include <functional>

namespace tiny_sql {

// 事件类型
enum class EventType : uint32_t {
    READ = 0x01,
    WRITE = 0x02,
    ERROR = 0x04,
    CLOSE = 0x08
};

// 事件循环抽象接口
class EventLoop {
public:
    using EventCallback = std::function<void(int fd, uint32_t events)>;

    virtual ~EventLoop() = default;

    // 初始化事件循环
    virtual bool init() = 0;

    // 添加文件描述符到事件循环
    virtual bool addFd(int fd, uint32_t events) = 0;

    // 修改文件描述符的事件
    virtual bool modifyFd(int fd, uint32_t events) = 0;

    // 从事件循环移除文件描述符
    virtual bool removeFd(int fd) = 0;

    // 等待事件（阻塞）
    // timeout: 超时时间（毫秒），-1表示永久等待
    // 返回就绪的事件数量
    virtual int wait(int timeout = -1) = 0;

    // 获取就绪事件的文件描述符
    virtual int getReadyFd(int index) const = 0;

    // 获取就绪事件的事件类型
    virtual uint32_t getReadyEvents(int index) const = 0;

    // 关闭事件循环
    virtual void close() = 0;
};

// 工厂函数：根据平台创建对应的事件循环
EventLoop* createEventLoop();

} // namespace tiny_sql
