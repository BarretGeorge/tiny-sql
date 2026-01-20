#pragma once

#include "tiny_sql/common/buffer.h"
#include <cstdint>
#include <vector>

namespace tiny_sql {

/**
 * MySQL 协议包头
 * 格式：3字节payload长度 + 1字节序列号
 */
struct PacketHeader {
    uint32_t payload_length;  // 实际只使用低24位
    uint8_t sequence_id;

    PacketHeader() : payload_length(0), sequence_id(0) {}
    PacketHeader(uint32_t len, uint8_t seq) : payload_length(len), sequence_id(seq) {}
};

/**
 * MySQL 协议包基类
 */
class Packet {
public:
    virtual ~Packet() = default;

    /**
     * 从Buffer解析包
     * @param buffer 数据缓冲区
     * @return 是否解析成功
     */
    virtual bool decode(Buffer& buffer) = 0;

    /**
     * 将包编码到Buffer
     * @param buffer 数据缓冲区
     * @param sequence_id 序列号
     */
    virtual void encode(Buffer& buffer, uint8_t sequence_id) = 0;

    /**
     * 获取payload长度（不包括4字节头）
     */
    virtual size_t getPayloadLength() const = 0;

protected:
    /**
     * 读取包头
     * @param buffer 数据缓冲区
     * @param header 包头结构
     * @return 是否读取成功
     */
    static bool readHeader(Buffer& buffer, PacketHeader& header);

    /**
     * 写入包头
     * @param buffer 数据缓冲区
     * @param payload_length payload长度
     * @param sequence_id 序列号
     */
    static void writeHeader(Buffer& buffer, uint32_t payload_length, uint8_t sequence_id);
};

/**
 * 通用MySQL包（用于未解析的包）
 */
class GenericPacket : public Packet {
public:
    GenericPacket() = default;
    explicit GenericPacket(const std::vector<uint8_t>& data) : payload_(data) {}

    bool decode(Buffer& buffer) override;
    void encode(Buffer& buffer, uint8_t sequence_id) override;
    size_t getPayloadLength() const override { return payload_.size(); }

    const std::vector<uint8_t>& getPayload() const { return payload_; }
    void setPayload(const std::vector<uint8_t>& payload) { payload_ = payload; }

private:
    std::vector<uint8_t> payload_;
};

/**
 * 工具函数：检查缓冲区中是否有完整的包
 * @param buffer 数据缓冲区
 * @return 如果有完整包返回包的总长度（包含头），否则返回0
 */
size_t checkPacketComplete(Buffer& buffer);

} // namespace tiny_sql
