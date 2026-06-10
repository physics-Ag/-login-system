#pragma once

#include "disk_avl.hpp"
#include "data_file.hpp"
#include "index_file.hpp"

#include <iostream>
#include <string>

namespace database {
    template<typename Key, typename Value>
    class IndexedDatabase {
    public:
        IndexedDatabase(std::string idx_path, std::string data_path)
            : idx_path_(std::move(idx_path))
            , data_path_(std::move(data_path))
            , index_(idx_)
        {}

        bool create()
        {
            close();
            if (!idx_.open(idx_path_, true)) {
                std::cout << "[error] 创建索引文件失败" << std::endl;
                return false;
            }
            if (!heap_.open(data_path_, true)) {
                std::cout << "[error] 创建数据文件失败" << std::endl;
                return false;
            }
            return true;
        }

        bool open()
        {
            close();
            if (!idx_.open(idx_path_, false)) {
                std::cout << "[error] 打开索引文件失败" << std::endl;
                return false;
            }
            if (!heap_.open(data_path_, false)) {
                std::cout << "[error] 打开数据文件失败" << std::endl;
                return false;
            }
            Superblock sb;
            return idx_.read_superblock(sb);
        }

        void close()
        {
            idx_.close();
            heap_.close();
        }

        bool put(const Key& index_key, const Value& value)
        {
            Superblock sb;
            if (!idx_.read_superblock(sb)) return false;

            size_t offset = sb.heap_next;
            if (!heap_.append_value(value, offset)) return false;
            sb.heap_next = offset + 4 + value.mem_size();

            if (!index_.insert(index_key, offset, sb)) {
                std::cout << "[error] 写入索引失败（可能键已存在）" << std::endl;
                return false;
            }
            return idx_.write_superblock(sb);
        }

        bool remove(const Key& index_key)
        {
            Superblock sb;
            if (!idx_.read_superblock(sb)) return false;
            if (!index_.remove(index_key, sb)) {
                std::cout << "[error] 删除索引失败（键可能不存在）" << std::endl;
                return false;
            }
            return idx_.write_superblock(sb);
        }

        bool get(const Key& index_key, Value& out) const
        {
            size_t offset = 0;
            if (!index_.find(index_key, offset)) {
                std::cout << "[error] 键不存在" << std::endl;
                return false;
            }
            if (!heap_.read_value(offset, out)) {
                std::cout << "[error] 值解码失败" << std::endl;
                return false;
            }
            return true;
        }

    private:
        std::string idx_path_;
        std::string data_path_;
        IndexFile idx_;
        dataFile<Value> heap_;
        DiskAVLTree<Key> index_;
    };
}