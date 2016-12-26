#pragma once
#ifndef RESTC_CPP_SOCKET_H_
#define RESTC_CPP_SOCKET_H_

#ifndef RESTC_CPP_H_
#       error "Include restc-cpp.h first"
#endif

#include "restc-cpp/SerializeJson.h"
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"


#include <string>
#include <assert.h>

namespace restc_cpp {

class RequestBuilder
{
public:

    RequestBuilder(Context& ctx)
    : ctx_{ctx} {}

    RequestBuilder& Get(std::string url) {
        assert(url_.empty());
        url_ = std::move(url);
        type_ = Request::Type::GET;
        return *this;
    }

    RequestBuilder& Post(std::string url) {
        assert(url_.empty());
        url_ = std::move(url);
        type_ = Request::Type::POST;
        return *this;
    }

    RequestBuilder& Put(std::string url) {
        assert(url_.empty());
        url_ = std::move(url);
        type_ = Request::Type::PUT;
        return *this;
    }

    RequestBuilder& Delete(std::string url) {
        assert(url_.empty());
        url_ = std::move(url);
        type_ = Request::Type::DELETE;
        return *this;
    }


    RequestBuilder& Header(std::string name,
                           std::string value) {
        if (!headers_) {
            headers_ = Request::headers_t();
        }

        headers_->insert({std::move(name), std::move(value)});
        return *this;
    }

    RequestBuilder& Argument(std::string name, int64_t value) {
        return Argument(move(name), std::to_string(value));
    }

    RequestBuilder& Argument(std::string name,
                             std::string value) {
        if (!args_) {
            args_ = Request::args_t();
        }

        args_->push_back({move(name), move(value)});
        return *this;
    }

    RequestBuilder& Data(const std::string& body) {
        assert(!body_);
        body_ = std::make_unique<Request::Body>(body);
        return *this;
    }

    RequestBuilder& Data(std::string&& body) {
        assert(!body_);
        body_ = std::make_unique<Request::Body>(move(body));
        return *this;
    }

    RequestBuilder& File(const boost::filesystem::path& path) {
        assert(!body_);
        body_ = std::make_unique<Request::Body>(path);
        return *this;
    }

    RequestBuilder& DisableCompression() {
        assert(!body_);
        disable_compression_ = true;
        return *this;
    }

    RequestBuilder& BasicAuthentication(const std::string name,
                                        const std::string passwd) {
        assert(!body_);

        auth_ = Request::auth_t(name, passwd);
        return *this;
    }

    // Json serialization
    template<typename T>
    RequestBuilder& Data(const T& data) {
        rapidjson::StringBuffer s;
        rapidjson::Writer<rapidjson::StringBuffer> writer(s);
        restc_cpp::RapidJsonSerializer<T, decltype(writer)>
            serializer(data, writer);
        serializer.IgnoreEmptyMembers();
        serializer.Serialize();
        // TODO: See if we can use the buffer without copying it
        std::string json = s.GetString();
        return Data(std::move(json));
    }

    std::unique_ptr<Request> Build() {
        static const std::string accept_encoding{"Accept-Encoding"};
        static const std::string gzip{"gzip"};
#ifdef DEBUG
        assert(!built_);
        built_ = true;
#endif
        if (!disable_compression_) {
            if (!headers_ || (headers_->find(accept_encoding) == headers_->end())) {
                Header(accept_encoding, gzip);
            }
        }
        return Request::Create(
            url_, type_, ctx_.GetClient(), move(body_), args_, headers_, auth_);
    }

    std::unique_ptr<Reply> Execute() {
        auto request = Build();
        return request->Execute(ctx_);
    }

private:
    Context& ctx_;
    std::string url_;
    Request::Type type_;
    boost::optional<Request::headers_t> headers_;
    boost::optional<Request::args_t> args_;
    boost::optional<Request::auth_t> auth_;
    std::unique_ptr<Request::Body> body_;
    bool disable_compression_ = false;
#ifdef DEBUG
    bool built_ = false;
#endif
};


} // restc_cpp


#endif // RESTC_CPP_SOCKET_H_
