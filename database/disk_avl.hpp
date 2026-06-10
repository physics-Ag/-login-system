#pragma once

#include "buffer.hpp"
#include "index_file.hpp"
#include <cstddef>
#include <cstring>

namespace database {
    template<typename Key>
    class DiskAVLTree {
    public:
        explicit DiskAVLTree(IndexFile& idx)
            :idx_(idx)
        {}

        bool find(const Key& key, size_t& offset_out) const
        {
            Superblock sb;
            if (!idx_.read_superblock(sb)) return false;
            size_t page_id = sb.root_page;
            if (page_id == 0) return false;

            while (page_id != 0)
            {
                Node n;
                if (!read_node(page_id, n)) return false;
                if (key == n.key) {
                    offset_out = n.rid;
                    return true;
                }
                if (key < n.key)
                    page_id = n.left;
                else
                    page_id = n.right;
            }
            return false;
        }

        bool insert(const Key& key, size_t heap_offset, Superblock& sb)
        {
            if (sb.root_page == 0)
            {
                Node root = alloc_node(sb, key, heap_offset, 0, 0, 0, 0);
                if (root.id == 0) return false;
                sb.root_page = root.id;
                return idx_.write_superblock(sb);
            }

            Node par;
            size_t cur = sb.root_page;
            while (cur != 0)
            {
                Node n;
                if (!read_node(cur, n)) return false;
                if (key == n.key) return false;

                par = n;
                if (key < n.key)
                    cur = n.left;
                else
                    cur = n.right;
            }

            Node z = alloc_node(sb, key, heap_offset, 0, 0, 0, par.id);
            if (z.id == 0) return false;

            if (key < par.key)
                par.left = z.id;
            else
                par.right = z.id;

            refresh_balance(par);
            if (!write_node(par)) return false;

            // 叶子一定平衡为0, 父节点才有可能不平衡
            z = rebalance(par);

            while (z.parent != 0)
            {
                if (!read_node(z.parent, par)) return false;
                if (z.key < par.key)
                    par.left = z.id;
                else
                    par.right = z.id;
                refresh_balance(par);
                if (!write_node(par)) return false;
                z = rebalance(par);
            }

            sb.root_page = z.id;
            return idx_.write_superblock(sb);
        }

        bool remove(const Key& key, Superblock& sb)
        {
            if (sb.root_page == 0) return false;

            size_t cur = sb.root_page;
            while (cur != 0) 
            {
                Node n;
                if (!read_node(cur, n)) return false;
                if (key == n.key)
                    return remove_node(n, sb);
                if (key < n.key)
                    cur = n.left;
                else
                    cur = n.right;
            }
            return false;
        }

    private:
        struct Node {
            size_t id = 0;         // 本结点页ID
            Key key;               // key键
            size_t rid = 0;        // 在数据文件的偏移量
            size_t left = 0;       // 左子结点ID
            size_t right = 0;      // 右子结点ID
            size_t parent = 0;     // 父结点ID
            size_t balance = 0;    // 平衡因子
        };

        IndexFile& idx_;

        static size_t key_off()
        {
            return 1;
        }

        static size_t rid_off(const Key& key)
        {
            return key_off() + key.mem_size();
        }

        static size_t left_off(const Key& key)
        {
            return rid_off(key) + 8;
        }

        static size_t right_off(const Key& key)
        {
            return left_off(key) + 4;
        }

        static size_t parent_off(const Key& key)
        {
            return right_off(key) + 4;
        }

        int subtree_height(size_t page_id) const
        {
            if (page_id == 0) return 0;
            Node n;
            if (!read_node(page_id, n)) return 0;
            const int hl = subtree_height(n.left);
            const int hr = subtree_height(n.right);
            return 1 + (hl > hr ? hl : hr);
        }

        void refresh_balance(Node& n) const
        {
            const int hl = subtree_height(n.left);
            const int hr = subtree_height(n.right);
            n.balance = static_cast<size_t>(hl - hr);
        }

        static ByteBuffer encode_node(const Node& n)
        {
            ByteBuffer page = make_page_byte_buffer();
            page.ref()[0] = static_cast<char>(n.balance);
            n.key.write_key(page.ref(), key_off());
            std::memcpy(page.ref() + rid_off(n.key), &n.rid, 8);
            std::memcpy(page.ref() + left_off(n.key), &n.left, 4);
            std::memcpy(page.ref() + right_off(n.key), &n.right, 4);
            std::memcpy(page.ref() + parent_off(n.key), &n.parent, 4);
            return page;
        }

        static void decode_node(const ByteBuffer& page, Node& n)
        {
            n.balance = static_cast<size_t>(page.ref()[0]);
            n.key = Key::read_key(page.ref(), key_off());
            std::memcpy(&n.rid, page.ref() + rid_off(n.key), 8);
            std::memcpy(&n.left, page.ref() + left_off(n.key), 4);
            std::memcpy(&n.right, page.ref() + right_off(n.key), 4);
            std::memcpy(&n.parent, page.ref() + parent_off(n.key), 4);
        }

        bool read_node(size_t page_id, Node& n) const
        {
            n.id = page_id;
            ByteBuffer page;
            if (!idx_.read_page_raw(page_id, page)) return false;
            decode_node(page, n);
            return true;
        }

        bool write_node(const Node& n) const
        {
            return idx_.write_page_raw(n.id, encode_node(n));
        }

        bool remove_node(Node victim, Superblock& sb)
        {
            if (victim.left != 0 && victim.right != 0)
            {
                // 两个孩子：找右子树最左结点
                size_t cur = victim.right;
                Node succ;
                while (true)
                {
                    if (!read_node(cur, succ)) return false;
                    if (succ.left == 0) break;
                    cur = succ.left;
                }

                // 找到了, 交换数据并将victim写入文件
                victim.key = succ.key;
                victim.rid = succ.rid;
                if (!write_node(victim)) return false;

                // 移除数据, 已经交换为一个孩子或者是叶子结点
                return remove_leaf_or_one_child(succ, sb);
            }

            // 一个孩子或者是叶子结点
            return remove_leaf_or_one_child(victim, sb);
        }

        bool remove_leaf_or_one_child(const Node& victim, Superblock& sb)
        {
            // 找孩子
            const size_t child = victim.left != 0 ? victim.left : victim.right;

            Node z;
            if (victim.parent == 0)
            {
                // 说明删除的是根节点
                sb.root_page = child;
                // 这里写得很粗糙, 页数没有减减, 主要是后续不好挪动数据
                if (child == 0)     return idx_.write_superblock(sb);  // 说明没有根结点
                if (!read_node(child, z))    return false;      // 有孩子结点, 则加载孩子结点
                z.parent = 0;
                if (!write_node(z)) return false;
            }
            else
            {
                // 说明删除的不是根节点
                Node par;
                if (!read_node(victim.parent, par)) return false;    // 加载父节点

                // 确认是左孩子还是有右孩子, 然后作为对应父节点的父节点的孩子
                if (par.left == victim.id)
                    par.left = child;
                else
                    par.right = child;

                // 更新孩子和父亲
                if (child != 0)
                {
                    if (!read_node(child, z)) return false;
                    z.parent = par.id;
                    if (!write_node(z)) return false;
                }
                if (!write_node(par)) return false;
                z = par;
            }

            // 父节点的平衡因子需要改变
            return fixup_after_remove(z, sb);
        }

        bool fixup_after_remove(Node z, Superblock& sb)
        {
            // 更新父节点
            refresh_balance(z);
            if (!write_node(z)) return false;
            z = rebalance(z);

            // 继续向上更新父节点
            while (z.parent != 0)
            {
                Node par;
                if (!read_node(z.parent, par)) return false;
                if (z.key < par.key)
                    par.left = z.id;
                else
                    par.right = z.id;
                refresh_balance(par);
                if (!write_node(par)) return false;
                z = rebalance(par);
            }
            sb.root_page = z.id;
            return idx_.write_superblock(sb);
        }

        Node alloc_node(Superblock& sb, const Key& key, size_t rid, size_t left, size_t right, size_t balance, size_t parent)
        {
            Node n;
            n.id = idx_.allocate_page(sb);
            if (n.id == 0) return n;

            n.key = key;
            n.rid = rid;
            n.left = left;
            n.right = right;
            n.parent = parent;
            n.balance = balance;
            if (!write_node(n)) n.id = 0;
            return n;
        }

        // 右旋就是把当前结点放在左节点的右边, 返回新的父节点
        // (1)、左节点的两个指针要变
        // (2)、当前结点的两个指针要变
        // (3)、左节点的右节点的一个指针要变
        Node rotate_right(Node& y)
        {
            if (y.left == 0) return y;

            Node x;
            if (!read_node(y.left, x)) return y;

            y.left = x.right;
            // 判断左节点的右节点是否为空，为空就不用改变左节点的右节点的指向了
            if (y.left != 0)
            {
                Node t2;
                if (read_node(y.left, t2))
                {
                    t2.parent = y.id;
                    write_node(t2);
                }
            }

            // 注意顺序
            x.right = y.id;
            x.parent = y.parent;
            y.parent = x.id;
            refresh_balance(y);
            refresh_balance(x);
            write_node(y);
            write_node(x);
            return x;
        }

        // 左旋就是把当前结点放在右节点的左边, 返回新的父节点
        Node rotate_left(Node& x)
        {
            if (x.right == 0) return x;

            Node y;
            if (!read_node(x.right, y)) return x;

            x.right = y.left;
            if (x.right != 0) {
                Node t2;
                if (read_node(x.right, t2))
                {
                    t2.parent = x.id;
                    write_node(t2);
                }
            }
            y.left = x.id;
            y.parent = x.parent;
            x.parent = y.id;
            refresh_balance(x);
            refresh_balance(y);
            write_node(x);
            write_node(y);
            return y;
        }

        Node rebalance(Node& n)
        {
            const int bf = static_cast<int>(n.balance);
            if (bf > 1)
            {
                // 说明左边高
                if (n.left != 0)
                {
                    Node left;
                    if (read_node(n.left, left) && static_cast<int>(left.balance) < 0) 
                    {
                        // 说明高在左边的右边
                        const Node rotated = rotate_left(left);
                        n.left = rotated.id;
                    }
                }

                write_node(n);
                return rotate_right(n);
            }
            if (bf < -1)
            {
                // 说明右边高, 左旋
                if (n.right != 0)
                {
                    Node right;
                    if (read_node(n.right, right) && static_cast<int>(right.balance) > 0)
                    {
                        // 说明高在右边的左边
                        const Node rotated = rotate_right(right);
                        n.right = rotated.id;
                    }
                }
                write_node(n);
                return rotate_left(n);
            }
            return n;
        }
    };
}