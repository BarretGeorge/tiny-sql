#include "tiny_sql/protocol/packet.h"
#include "tiny_sql/common/logger.h"

namespace tiny_sql {

bool Packet::readHeader(Buffer& buffer, PacketHeader& header) {
    if (buffer.readableBytes() < 4) {
        return false;
    }

    // 保存当前位置，以便失败时回退
    size_t save_pos = buffer.readerIndex();

    // 读取3字节长度（小端序）
    uint32_t len = 0;
    len |= buffer.readUint8();
    len |= (static_cast<uint32_t>(buffer.readUint8()) << 8);
    len |= (static_cast<uint32_t>(buffer.readUint8()) << 16);

    // 读取1字节序列号
    uint8_t seq = buffer.readUint8();

    header.payload_length = len;
    header.sequence_id = seq;

    return true;
}

void Packet::writeHeader(Buffer& buffer, uint32_t payload_length, uint8_t sequence_id) {
    // 写入3字节长度（小端序）
    buffer.writeUint8(static_cast<uint8_t>(payload_length & 0xFF));
    buffer.writeUint8(static_cast<uint8_t>((payload_length >> 8) & 0xFF));
    buffer.writeUint8(static_cast<uint8_t>((payload_length >> 16) & 0xFF));

    // 写入1字节序列号
    buffer.writeUint8(sequence_id);
}

bool GenericPacket::decode(Buffer& buffer) {
    PacketHeader header;
    if (!readHeader(buffer, header)) {
        return false;
    }

    // 检查是否有足够的数据
    if (buffer.readableBytes() < header.payload_length) {
        return false;
    }

    // 读取payload
    payload_ = buffer.retrieve(header.payload_length);
    return true;
}

void GenericPacket::encode(Buffer& buffer, uint8_t sequence_id) {
    // 写入包头
    writeHeader(buffer, static_cast<uint32_t>(payload_.size()), sequence_id);

    // 写入payload
    buffer.append(payload_.data(), payload_.size());
}

size_t checkPacketComplete(Buffer& buffer) {
    if (buffer.readableBytes() < 4) {
        return 0;  // 连包头都不完整
    }

    // 读取长度（不移动读指针）
    size_t save_pos = buffer.readerIndex();

    uint32_t payload_length = 0;
    payload_length |= buffer.readUint8();
    payload_length |= (static_cast<uint32_t>(buffer.readUint8()) << 8);
    payload_length |= (static_cast<uint32_t>(buffer.readUint8()) << 16);

    // 恢复读指针
    buffer.setReaderIndex(save_pos);

    // 检查是否有完整的包（4字节头 + payload）
    size_t total_length = 4 + payload_length;
    if (buffer.readableBytes() >= total_length) {
        return total_length;
    }

    return 0;  // 包不完整
}

} // namespace tiny_sql
