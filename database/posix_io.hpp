#pragma once

#include <cerrno>
#include <cstddef>
#include <cstring>
#include <fcntl.h>
#include <iostream>
#include <string>
#include <unistd.h>

namespace database {

inline bool posix_open_file(const std::string& path, bool empty, int& fd_out)
{
    fd_out = -1;
    if (empty)
    {
        const int fd = open(path.c_str(), O_RDWR | O_CREAT | O_TRUNC, 0644);
        if (fd < 0)
        {
            std::cout << "[error] 打开或新建文件失败: " << path << " (" << std::strerror(errno) << ")" << std::endl;
            return false;
        }
        fd_out = fd;
        return true;
    }
    else
    {
        const int fd = open(path.c_str(), O_RDWR);
        if (fd < 0)
        {
            std::cout << "[error] 打开文件失败: " << path << " (" << std::strerror(errno) << ")" << std::endl;
            return false;
        }
        fd_out = fd;
        return true;
    }
}

inline void posix_close_file(int& fd)
{
    if (fd >= 0) {
        close(fd);
        fd = -1;
    }
}

inline bool posix_read_at(int fd, void* buf, size_t n, size_t offset)
{
    char* p = static_cast<char*>(buf);
    size_t done = 0;
    while (done < n) {
        const ssize_t r = pread(fd, p + done, n - done, static_cast<off_t>(offset + done));
        if (r <= 0) {
            std::cout << "[error] pread 失败: " << std::strerror(errno) << std::endl;
            return false;
        }
        done += static_cast<size_t>(r);
    }
    return true;
}

// offset表示写入的起始位置
inline bool posix_write_at(int fd, const void* buf, size_t n, size_t offset)
{
    const char* p = static_cast<const char*>(buf);
    size_t done = 0;
    while (done < n) {
        const ssize_t w = pwrite(fd, p + done, n - done, static_cast<off_t>(offset + done));
        if (w <= 0) {
            std::cout << "[error] pwrite 失败: " << std::strerror(errno) << std::endl;
            return false;
        }
        done += static_cast<size_t>(w);
    }
    return true;
}

inline bool posix_sync(int fd)
{
    if (fsync(fd) != 0) {
        std::cout << "[error] fsync 失败: " << std::strerror(errno) << std::endl;
        return false;
    }
    return true;
}

// 获取文件大小
inline bool posix_file_size(int fd, size_t& size_out)
{
    const off_t end = lseek(fd, 0, SEEK_END);
    if (end < 0) {
        std::cout << "[error] lseek 失败: " << std::strerror(errno) << std::endl;
        return false;
    }
    size_out = static_cast<size_t>(end);
    return true;
}

} // namespace database
