#pragma once

#include <vector>
#include <string>
#include <cstring>
#include <cstdint>
#include <stdexcept>

namespace tiny_sql {

class Buffer {
public:
    Buffer() : read_index_(0), write_index_(0) {
        data_.reserve(4096);  // 预分配4KB
    }

    explicit Buffer(size_t initial_size) : read_index_(0), write_index_(0) {
        data_.reserve(initial_size);
    }

    explicit Buffer(const std::vector<uint8_t>& data)
        : data_(data), read_index_(0), write_index_(data.size()) {}

    explicit Buffer(const std::string& str)
        : read_index_(0), write_index_(str.size()) {
        data_.assign(str.begin(), str.end());
    }

    // 可读字节数
    size_t readableBytes() const {
        return write_index_ - read_index_;
    }

    // 可写字节数
    size_t writableBytes() const {
        return data_.capacity() - write_index_;
    }

    // 已读字节数
    size_t prependableBytes() const {
        return read_index_;
    }

    // 获取可读数据的指针
    const uint8_t* peek() const {
        return data_.data() + read_index_;
    }

    uint8_t* beginWrite() {
        return data_.data() + write_index_;
    }

    // 读取但不移动读指针
    uint8_t peekUint8() const {
        if (readableBytes() < 1) {
            throw std::runtime_error("Buffer: not enough data to read uint8");
        }
        return data_[read_index_];
    }

    uint16_t peekUint16() const {
        if (readableBytes() < 2) {
            throw std::runtime_error("Buffer: not enough data to read uint16");
        }
        return static_cast<uint16_t>(data_[read_index_]) |
               (static_cast<uint16_t>(data_[read_index_ + 1]) << 8);
    }

    uint32_t peekUint32() const {
        if (readableBytes() < 4) {
            throw std::runtime_error("Buffer: not enough data to read uint32");
        }
        return static_cast<uint32_t>(data_[read_index_]) |
               (static_cast<uint32_t>(data_[read_index_ + 1]) << 8) |
               (static_cast<uint32_t>(data_[read_index_ + 2]) << 16) |
               (static_cast<uint32_t>(data_[read_index_ + 3]) << 24);
    }

    // 读取并移动读指针
    uint8_t readUint8() {
        uint8_t val = peekUint8();
        read_index_ += 1;
        return val;
    }

    uint16_t readUint16() {
        uint16_t val = peekUint16();
        read_index_ += 2;
        return val;
    }

    uint32_t readUint32() {
        uint32_t val = peekUint32();
        read_index_ += 4;
        return val;
    }

    uint64_t readUint64() {
        if (readableBytes() < 8) {
            throw std::runtime_error("Buffer: not enough data to read uint64");
        }
        uint64_t val = 0;
        for (int i = 0; i < 8; i++) {
            val |= static_cast<uint64_t>(data_[read_index_ + i]) << (i * 8);
        }
        read_index_ += 8;
        return val;
    }

    // 读取指定长度的数据
    std::vector<uint8_t> readBytes(size_t len) {
        if (readableBytes() < len) {
            throw std::runtime_error("Buffer: not enough data to read bytes");
        }
        std::vector<uint8_t> result(data_.begin() + read_index_,
                                    data_.begin() + read_index_ + len);
        read_index_ += len;
        return result;
    }

    std::string readString(size_t len) {
        if (readableBytes() < len) {
            throw std::runtime_error("Buffer: not enough data to read string");
        }
        std::string result(data_.begin() + read_index_,
                          data_.begin() + read_index_ + len);
        read_index_ += len;
        return result;
    }

    // 读取以null结尾的字符串
    std::string readNullTerminatedString() {
        const uint8_t* start = peek();
        const uint8_t* end = static_cast<const uint8_t*>(
            std::memchr(start, 0, readableBytes()));

        if (end == nullptr) {
            throw std::runtime_error("Buffer: no null terminator found");
        }

        size_t len = end - start;
        std::string result(reinterpret_cast<const char*>(start), len);
        read_index_ += len + 1;  // +1 for null terminator
        return result;
    }

    // 读取length-encoded integer (MySQL protocol)
    uint64_t readLenencInt() {
        if (readableBytes() < 1) {
            throw std::runtime_error("Buffer: not enough data for lenenc int");
        }

        uint8_t first = readUint8();

        if (first < 0xFB) {
            return first;
        } else if (first == 0xFC) {
            return readUint16();
        } else if (first == 0xFD) {
            if (readableBytes() < 3) {
                throw std::runtime_error("Buffer: not enough data for 3-byte lenenc int");
            }
            uint32_t val = readUint8() | (readUint8() << 8) | (readUint8() << 16);
            return val;
        } else if (first == 0xFE) {
            return readUint64();
        }

        throw std::runtime_error("Buffer: invalid lenenc int (0xFF)");
    }

    // 读取length-encoded string (MySQL protocol)
    std::string readLenencString() {
        uint64_t len = readLenencInt();
        return readString(static_cast<size_t>(len));
    }

    // 写入数据
    void writeUint8(uint8_t val) {
        ensureWritable(1);
        data_.push_back(val);
        write_index_ += 1;
    }

    void writeUint16(uint16_t val) {
        ensureWritable(2);
        data_.push_back(val & 0xFF);
        data_.push_back((val >> 8) & 0xFF);
        write_index_ += 2;
    }

    void writeUint32(uint32_t val) {
        ensureWritable(4);
        data_.push_back(val & 0xFF);
        data_.push_back((val >> 8) & 0xFF);
        data_.push_back((val >> 16) & 0xFF);
        data_.push_back((val >> 24) & 0xFF);
        write_index_ += 4;
    }

    void writeUint64(uint64_t val) {
        ensureWritable(8);
        for (int i = 0; i < 8; i++) {
            data_.push_back((val >> (i * 8)) & 0xFF);
        }
        write_index_ += 8;
    }

    void writeBytes(const std::vector<uint8_t>& bytes) {
        ensureWritable(bytes.size());
        data_.insert(data_.end(), bytes.begin(), bytes.end());
        write_index_ += bytes.size();
    }

    void writeBytes(const uint8_t* data, size_t len) {
        ensureWritable(len);
        data_.insert(data_.end(), data, data + len);
        write_index_ += len;
    }

    void writeString(const std::string& str) {
        ensureWritable(str.size());
        data_.insert(data_.end(), str.begin(), str.end());
        write_index_ += str.size();
    }

    // 写入null结尾的字符串
    void writeNullTerminatedString(const std::string& str) {
        writeString(str);
        writeUint8(0);
    }

    // 写入length-encoded integer (MySQL protocol)
    void writeLenencInt(uint64_t val) {
        if (val < 0xFB) {
            writeUint8(static_cast<uint8_t>(val));
        } else if (val < 0x10000) {
            writeUint8(0xFC);
            writeUint16(static_cast<uint16_t>(val));
        } else if (val < 0x1000000) {
            writeUint8(0xFD);
            writeUint8(val & 0xFF);
            writeUint8((val >> 8) & 0xFF);
            writeUint8((val >> 16) & 0xFF);
        } else {
            writeUint8(0xFE);
            writeUint64(val);
        }
    }

    // 写入length-encoded string (MySQL protocol)
    void writeLenencString(const std::string& str) {
        writeLenencInt(str.size());
        writeString(str);
    }

    // 跳过n个字节
    void skip(size_t n) {
        if (readableBytes() < n) {
            throw std::runtime_error("Buffer: not enough data to skip");
        }
        read_index_ += n;
    }

    // 重置缓冲区
    void reset() {
        read_index_ = 0;
        write_index_ = 0;
        data_.clear();
    }

    // 获取所有数据
    const std::vector<uint8_t>& data() const {
        return data_;
    }

    std::vector<uint8_t> retrieveAll() {
        std::vector<uint8_t> result(data_.begin() + read_index_,
                                    data_.begin() + write_index_);
        reset();
        return result;
    }

    // 读取指定长度并移除
    std::vector<uint8_t> retrieve(size_t len) {
        if (readableBytes() < len) {
            throw std::runtime_error("Buffer: not enough data to retrieve");
        }
        std::vector<uint8_t> result(data_.begin() + read_index_,
                                    data_.begin() + read_index_ + len);
        read_index_ += len;
        return result;
    }

    // 追加数据
    void append(const uint8_t* data, size_t len) {
        ensureWritable(len);
        data_.insert(data_.end(), data, data + len);
        write_index_ += len;
    }

    void append(const std::vector<uint8_t>& data) {
        append(data.data(), data.size());
    }

    // 获取读指针位置
    size_t readerIndex() const {
        return read_index_;
    }

    // 设置读指针位置
    void setReaderIndex(size_t index) {
        if (index > write_index_) {
            throw std::runtime_error("Buffer: invalid reader index");
        }
        read_index_ = index;
    }

    // 获取写指针位置
    size_t writerIndex() const {
        return write_index_;
    }

    // 获取lenenc int的编码长度（用于计算大小）
    static size_t getLenencIntSize(uint64_t val) {
        if (val < 0xFB) return 1;
        else if (val < 0x10000) return 3;
        else if (val < 0x1000000) return 4;
        else return 9;
    }

    // 从文件描述符读取数据
    ssize_t readFromFd(int fd);

    // 写入到文件描述符
    ssize_t writeToFd(int fd);

private:
    void ensureWritable(size_t len) {
        if (writableBytes() < len) {
            data_.reserve(data_.capacity() + len);
        }
    }

    std::vector<uint8_t> data_;
    size_t read_index_;
    size_t write_index_;
};

} // namespace tiny_sql
