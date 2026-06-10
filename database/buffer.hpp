#pragma once

#include "constants.hpp"

#include <cstddef>
#include <cstring>
#include <memory>

namespace database {
    struct ByteBuffer {
        std::unique_ptr<char[]> data;
        size_t size = 0;

        char* ref()
        {
            return data.get();
        }

        const char* ref() const
        {
            return data.get();
        }
    };

    inline ByteBuffer make_page_byte_buffer()
    {
        ByteBuffer page;
        page.size = kPageSize;
        page.data.reset(new char[kPageSize]);
        std::memset(page.ref(), 0, kPageSize);
        return page;
    }
}