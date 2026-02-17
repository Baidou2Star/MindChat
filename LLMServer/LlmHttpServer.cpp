#include "LlmHttpServer.h"

#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/strand.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>

#include <iostream>
#include <sstream>

namespace beast = boost::beast;
namespace http = beast::http;
namespace net = boost::asio;
using tcp = net::ip::tcp;

namespace {

bool ParseJsonBody(const std::string& body, Json::Value& out) {
    Json::CharReaderBuilder builder;
    std::string errs;
    std::istringstream ss(body);
    return Json::parseFromStream(builder, ss, &out, &errs);
}

std::string ToCompactJson(const Json::Value& value) {
    Json::StreamWriterBuilder builder;
    builder["indentation"] = "";
    return Json::writeString(builder, value);
}

}  // namespace

void LlmHttpServer::Run() {
    const auto ip = net::ip::make_address(host_);
    net::io_context ioc{1};
    tcp::acceptor acceptor(ioc, tcp::endpoint{ip, port_});

    std::cout << "[LLMServer] listen on " << host_ << ":" << port_ << std::endl;

    for (;;) {
        beast::error_code ec;
        tcp::socket socket(ioc);
        acceptor.accept(socket, ec);
        if (ec) {
            std::cerr << "[LLMServer] accept error: " << ec.message() << std::endl;
            continue;
        }

        beast::flat_buffer buffer;
        http::request<http::string_body> req;
        http::read(socket, buffer, req, ec);
        if (ec) {
            std::cerr << "[LLMServer] read error: " << ec.message() << std::endl;
            continue;
        }

        auto res = HandleRequest(req);
        http::write(socket, res, ec);
        socket.shutdown(tcp::socket::shutdown_send, ec);
    }
}

http::response<http::string_body> LlmHttpServer::JsonResponse(http::status status,
                                                              const Json::Value& body) {
    http::response<http::string_body> res{status, 11};
    res.set(http::field::content_type, "application/json; charset=utf-8");
    res.set(http::field::server, "myChat-LLMServer");
    res.keep_alive(false);
    res.body() = ToCompactJson(body);
    res.prepare_payload();
    return res;
}

http::response<http::string_body> LlmHttpServer::HandleRequest(const http::request<http::string_body>& req) {
    if (req.method() == http::verb::get && req.target() == "/health") {
        Json::Value body;
        body["error"] = 0;
        body["message"] = "ok";
        return JsonResponse(http::status::ok, body);
    }

    if (req.method() != http::verb::post) {
        Json::Value body;
        body["error"] = 1;
        body["message"] = "method not allowed";
        return JsonResponse(http::status::method_not_allowed, body);
    }

    if (req.target() == "/chat") {
        Json::Value input;
        if (!ParseJsonBody(req.body(), input) || !input["messages"].isArray()) {
            Json::Value body;
            body["error"] = 1;
            body["message"] = "invalid request json";
            return JsonResponse(http::status::bad_request, body);
        }

        const double temperature = input["temperature"].isNumeric() ? input["temperature"].asDouble() : 0.2;

        std::string reply;
        std::string err;
        if (!provider_pool_.Chat(input["messages"], reply, err, temperature)) {
            Json::Value body;
            body["error"] = 1;
            body["message"] = err;
            return JsonResponse(http::status::bad_gateway, body);
        }

        Json::Value body;
        body["error"] = 0;
        body["reply"] = reply;
        return JsonResponse(http::status::ok, body);
    }

    if (req.target() == "/extract_todo") {
        Json::Value input;
        if (!ParseJsonBody(req.body(), input) || !input["text"].isString()) {
            Json::Value body;
            body["error"] = 1;
            body["message"] = "invalid request json";
            return JsonResponse(http::status::bad_request, body);
        }

        Json::Value todo;
        std::string err;
        if (!provider_pool_.ExtractTodo(input["text"].asString(), todo, err)) {
            Json::Value body;
            body["error"] = 1;
            body["message"] = err;
            return JsonResponse(http::status::bad_gateway, body);
        }

        Json::Value body;
        body["error"] = 0;
        body["todo"] = todo;
        return JsonResponse(http::status::ok, body);
    }

    Json::Value body;
    body["error"] = 1;
    body["message"] = "not found";
    return JsonResponse(http::status::not_found, body);
}
