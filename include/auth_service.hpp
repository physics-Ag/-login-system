#pragma once

#include "../database/database.hpp"
#include "pthread_scoped_lock.hpp"
#include "user.hpp"
#include <pthread.h>
#include <string>

class AuthService {
public:
    AuthService(std::string idx_path, std::string data_path)
        :db_(std::move(idx_path), std::move(data_path))
    {
        pthread_mutex_init(&mu_, nullptr);
        if (!db_.open())    db_.create();
    }

    ~AuthService()
    {
        db_.close();
        pthread_mutex_destroy(&mu_);
    }

    // 注册
    bool register_user(const std::string& username, const std::string& password, std::string& err)
    {
        if (username.empty() || password.empty()) {
            err = "用户名与密码不能为空";
            return false;
        }

        chat_detail::PthreadLock lk(&mu_);

        name key;
        key.username = username;

        passwd existing;
        if (db_.get(key, existing)) {
            err = "用户名已存在";
            return false;
        }

        passwd value;
        value.password = password;
        if (!db_.put(key, value)) {
            err = "注册失败";
            return false;
        }
        return true;
    }

    // 登录
    bool login(const std::string& username, const std::string& password, std::string& err)
    {
        if (username.empty() || password.empty()) {
            err = "用户名与密码不能为空";
            return false;
        }

        chat_detail::PthreadLock lk(&mu_);

        name key;
        key.username = username;

        passwd stored;
        if (!db_.get(key, stored)) {
            err = "用户名或密码错误";
            return false;
        }
        if (stored.password != password) {
            err = "用户名或密码错误";
            return false;
        }

        return true;
    }

    bool quit(const std::string& username, std::string& err)
    {
        if (username.empty()) {
            err = "用户不为空";
            return false;
        }

        chat_detail::PthreadLock lk(&mu_);

        name key;
        key.username = username;

        if (!db_.remove(key)) {
            err = "注销失败";
            return false;
        }

        return true;
    }

private:
    mutable pthread_mutex_t mu_;
    database::IndexedDatabase<name, passwd> db_;
};