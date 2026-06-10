#include "./include/auth_service.hpp"
#include "include/httplib.h"
#include <jsoncpp/json/json.h>
#include <cstdlib>
#include <iostream>
#include <string>

static void json_error(httplib::Response& res, int code, const std::string& msg)
{
    res.status = code;
    Json::Value out;
    out["ok"] = false;
    out["error"] = msg;
    Json::FastWriter w;
    res.set_content(w.write(out), "application/json; charset=utf-8");
}

static void json_ok(httplib::Response& res, const Json::Value& body)
{
    res.status = 200;
    Json::FastWriter w;
    res.set_content(w.write(body), "application/json; charset=utf-8");
}

int main(int argc, char* argv[])
{
    if (argc != 2)
    {
        std::cerr << "请输入端口号" << std::endl;
        return 1;
    }
    int port = std::atoi(argv[1]);

    httplib::Server svr;
    AuthService auth("data/users.idx", "data/users.data");

    svr.Post("/api/register", [&](const httplib::Request& req, httplib::Response& res) {
        Json::Reader reader;
        Json::Value root;
        reader.parse(req.body, root);

        if (!root.isMember("username") || !root["username"].isString() || !root.isMember("password") || !root["password"].isString())
        {
            json_error(res, 400, "需要字符串字段 username 与 password");
            return;
        }

        std::string username = root["username"].asString();
        std::string password = root["password"].asString();
        std::string err;
        if (!auth.register_user(username, password, err))
        {
            json_error(res, 409, err);
            return;
        }

        Json::Value out;
        out["ok"] = true;
        out["message"] = "注册成功";
        json_ok(res, out);
    });

    svr.Post("/api/login", [&](const httplib::Request& req, httplib::Response& res) {
        Json::Reader reader;
        Json::Value root;
        reader.parse(req.body, root);

        if (!root.isMember("username") || !root["username"].isString() || !root.isMember("password") || !root["password"].isString())
        {
            json_error(res, 400, "需要字符串字段 username 与 password");
            return;
        }

        std::string username = root["username"].asString();
        std::string password = root["password"].asString();
        std::string err;
        if (!auth.login(username, password, err))
        {
            json_error(res, 401, err);
            return;
        }

        Json::Value out;
        out["ok"] = true;
        out["username"] = username;
        json_ok(res, out);
    });

    svr.Get("/", [](const httplib::Request&, httplib::Response& res) {
        res.set_file_content("html/index.html", "text/html; charset=utf-8");
    });

    svr.Post("/api/quit", [&](const httplib::Request& req, httplib::Response& res) {
        Json::Reader reader;
        Json::Value root;
        reader.parse(req.body, root);

        if (!root.isMember("username") || !root["username"].isString())
        {
            json_error(res, 400, "需要字符串字段 username");
            return;
        }

        std::string username = root["username"].asString();
        std::string err;
        if (!auth.quit(username, err))
        {
            json_error(res, 401, err);
            return;
        }

        Json::Value out;
        out["ok"] = true;
        out["username"] = username;
        json_ok(res, out);
    });

    svr.Get("/", [](const httplib::Request&, httplib::Response& res) {
        res.set_file_content("html/index.html", "text/html; charset=utf-8");
    });

    const char* host = "0.0.0.0";

    if (!svr.listen(host, port)) {
        std::cerr << "监听失败\n";
        return 1;
    }
    return 0;
}
