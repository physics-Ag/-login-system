#pragma once

#include "buffer.hpp"
#include "constants.hpp"
#include "posix_io.hpp"

#include <cstring>
#include <iostream>
#include <string>

namespace database
{
    struct Superblock {
        size_t magic = kMagic;
        size_t page_size = kPageSize;
        size_t root_page = 0;
        size_t page_count = 1;
        size_t heap_next = 0;
    };

    class IndexFile {
    public:
        bool open(const std::string& path, bool empty)
        {
            close();
            path_ = path;
            if (!posix_open_file(path_, empty, fd_)) return false;

            if (empty)
            {
                Superblock sb;
                if (!write_superblock(sb))
                {
                    std::cout << "[error] 初始化索引元数据页失败: " << path_ << std::endl;
                    return false;
                }
            }
            return true;
        }

        void close()
        {
            posix_close_file(fd_);
        }

        bool read_page_raw(size_t page_id, ByteBuffer& buf) const
        {
            if (fd_ < 0) {
                std::cout << "[error] 索引文件未打开" << std::endl;
                return false;
            }
            buf.size = kPageSize;
            buf.data.reset(new char[kPageSize]);
            const size_t off = page_id * kPageSize;
            return posix_read_at(fd_, buf.ref(), kPageSize, off);
        }

        bool write_page_raw(size_t page_id, const ByteBuffer& buf)
        {
            if (fd_ < 0) {
                std::cout << "[error] 索引文件未打开" << std::endl;
                return false;
            }
            if (buf.size != kPageSize) {
                std::cout << "[error] 页缓冲大小无效: " << buf.size << std::endl;
                return false;
            }
            const size_t off = page_id * kPageSize;
            if (!posix_write_at(fd_, buf.ref(), kPageSize, off)) return false;
            return posix_sync(fd_);
        }

        bool read_superblock(Superblock& sb) const
        {
            ByteBuffer page;
            if (!read_page_raw(kSuperPage, page)) {
                std::cout << "[error] 读取索引元数据页失败: " << path_ << std::endl;
                return false;
            }
            decode_superblock(page, sb);
            if (sb.magic != kMagic || sb.page_size != kPageSize) {
                std::cout << "[error] 索引文件格式无效: " << path_ << std::endl;
                return false;
            }
            return true;
        }

        bool write_superblock(const Superblock& sb)
        {
            const ByteBuffer page = encode_superblock(sb);
            if (!write_page_raw(kSuperPage, page)) {
                std::cout << "[error] 写入索引元数据页失败: " << path_ << std::endl;
                return false;
            }
            return true;
        }

        size_t allocate_page(Superblock& sb)
        {
            const size_t id = sb.page_count;
            sb.page_count++;
            const ByteBuffer page = make_page_byte_buffer();

            if (!write_page_raw(id, page)) {
                std::cout << "[error] 分配索引页失败 page_id=" << id << std::endl;
                return 0;
            }
            return id;
        }

        static ByteBuffer encode_superblock(const Superblock& sb)
        {
            ByteBuffer page = make_page_byte_buffer();
            std::memcpy(page.ref() + 0, &sb.magic, 4);
            std::memcpy(page.ref() + 4, &sb.page_size, 4);
            std::memcpy(page.ref() + 8, &sb.root_page, 4);
            std::memcpy(page.ref() + 12, &sb.page_count, 4);
            std::memcpy(page.ref() + 16, &sb.heap_next, 8);
            return page;
        }

        static void decode_superblock(const ByteBuffer& page, Superblock& sb)
        {
            std::memcpy(&sb.magic, page.ref() + 0, 4);
            std::memcpy(&sb.page_size, page.ref() + 4, 4);
            std::memcpy(&sb.root_page, page.ref() + 8, 4);
            std::memcpy(&sb.page_count, page.ref() + 12, 4);
            std::memcpy(&sb.heap_next, page.ref() + 16, 8);
        }

        ~IndexFile()
        {
            close();
        }

    private:
        std::string path_;
        int fd_ = -1;
    };
}
