#pragma once

#include "LlmProviderPool.h"

#include <boost/beast/http.hpp>

#include <string>

class LlmHttpServer {
public:
    LlmHttpServer(std::string host, unsigned short port, LlmProviderPool& provider_pool)
        : host_(std::move(host)), port_(port), provider_pool_(provider_pool) {}

    void Run();

private:
    boost::beast::http::response<boost::beast::http::string_body> HandleRequest(
        const boost::beast::http::request<boost::beast::http::string_body>& req);

    boost::beast::http::response<boost::beast::http::string_body> JsonResponse(
        boost::beast::http::status status,
        const Json::Value& body);

private:
    std::string host_;
    unsigned short port_;
    LlmProviderPool& provider_pool_;
};
