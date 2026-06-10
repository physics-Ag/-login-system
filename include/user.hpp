#pragma once
#include <cstring>
#include <string>

struct name {
    std::string username;

    bool operator<(const name& o) const  
    { 
        return username < o.username;
    }

    bool operator>(const name& o) const  
    { 
        return username > o.username;
    }
    
    bool operator==(const name& o) const
    {
        return username == o.username;
    }

    size_t size() const
    {
        return username.size();
    }

    // 返回的是写入缓冲区的大小(长度 + 内容)
    size_t mem_size() const
    {
        return 2 + username.size();
    }
    
    // 将key的大小和长度写入缓冲区
    void write_key(char* page, size_t off) const
    {
        const size_t len = username.size();
        std::memcpy(page + off, &len, 2);
        std::memcpy(page + off + 2, username.c_str(), len);
    }

    // 将key从缓冲区读出
    static name read_key(const char* page, size_t off)
    {
        size_t len = 0;
        std::memcpy(&len, page + off, 2);
        name k;
        k.username.assign(page + off + 2, len);
        return k;
    }
};

struct passwd {
    std::string password;

    bool operator<(const passwd& o) const  
    { 
        return password < o.password;
    }

    bool operator>(const passwd& o) const  
    { 
        return password > o.password;
    }
    
    bool operator==(const passwd& o) const
    {
        return password == o.password;
    }

    size_t size() const
    {
        return password.size();
    }

    // 返回的是写入缓冲区的大小(长度 + 内容)
    size_t mem_size() const
    {
        return 2 + password.size();
    }
    
    // 将value的大小和长度写入缓冲区
    void write_value(char* page, size_t off) const
    {
        const size_t len = password.size();
        std::memcpy(page + off, &len, 2);
        std::memcpy(page + off + 2, password.c_str(), len);
    }

    // 将value从缓冲区读出
    static passwd read_value(const char* page, size_t off)
    {
        size_t len = 0;
        std::memcpy(&len, page + off, 2);
        passwd p;
        p.password.assign(page + off + 2, len);
        return p;
    }
};