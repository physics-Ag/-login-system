#pragma once

#include "buffer.hpp"
#include "posix_io.hpp"

#include <cstring>
#include <iostream>
#include <memory>
#include <string>

namespace database {
    // 管理数据的类
    template<typename T>
    class dataFile {
    public:
        bool open(const std::string& path, bool create)
        {
            close();
            path_ = path;
            return posix_open_file(path_, create, fd_);
        }

        void close()
        {
            posix_close_file(fd_);
        }

        bool read_at(size_t offset, ByteBuffer& payload_out) const
        {
            if (fd_ < 0) {
                std::cout << "[error] 数据文件未打开" << std::endl;
                return false;
            }
            size_t len = 0;
            if (!posix_read_at(fd_, &len, 4, offset)) return false;
            payload_out.size = len;
            if (len == 0) {
                payload_out.data.reset();
                return true;
            }
            payload_out.data.reset(new char[len]);
            return posix_read_at(fd_, payload_out.data.get(), len, offset + 4);
        }

        bool append_value(const T& value, size_t& offset_out)
        {
            const ByteBuffer payload = encode_value(value);

            if (fd_ < 0) {
                std::cout << "[error] 数据文件未打开" << std::endl;
                return false;
            }
            size_t end = 0;
            if (!posix_file_size(fd_, end)) return false;
            offset_out = end;

            const size_t len = payload.size;
            if (!posix_write_at(fd_, &len, 4, offset_out)) return false;
            if (len > 0 && !posix_write_at(fd_, payload.ref(), len, offset_out + 4)) return false;
            return posix_sync(fd_);
        }

        bool read_value(size_t offset, T& out) const
        {
            ByteBuffer payload;
            if (!read_at(offset, payload)) return false;
            if (payload.size < 2) return false;
            out = decode_value(payload);
            return true;
        }

        ByteBuffer encode_value(const T& value) const
        {
            ByteBuffer out;
            out.size = value.mem_size();
            out.data.reset(new char[out.size]);
            value.write_value(out.ref(), 0);
            return out;
        }
    
        T decode_value(const ByteBuffer& in) const
        {
            return T::read_value(in.ref(), 0);
        }

    private:
        std::string path_;
        int fd_ = -1;
    };
}