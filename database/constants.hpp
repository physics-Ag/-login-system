#pragma once
#include <cstddef>

namespace database {
    constexpr size_t kMagic = 0x41564c44u;
    /// 索引文件一页的字节数（编译期常量）；页号 p 在文件中的偏移为 p * kPageSize
    constexpr size_t kPageSize = 128u;
    constexpr size_t kSuperPage = 0u;
}
