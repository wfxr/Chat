#pragma once

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>

class chat_message {
public:
    const static int header_length = 4;
    const static int max_body_length = 512;

    chat_message() : body_length_(0) {}

    chat_message(const std::string msg) { encode(msg.c_str()); }

    const char *data() const { return data_; }
    char *data() { return data_; }

    const char *header() const { return data_; }
    char *header() { return data_; }

    const char *body() const { return data_ + header_length; }
    char *body() { return data_ + header_length; }

    size_t length() const { return header_length + body_length_; }

    bool encode(const char *msg) {
        if (!body_length(strlen(msg))) return false;

        encode_header(body_length());
        encode_body(msg);
        return true;
    }

    size_t body_length() const { return body_length_; }
    bool body_length(size_t new_length) {
        if (new_length > max_body_length) {
            body_length_ = 0;
            return false;
        }

        body_length_ = new_length;
        return true;
    }

    bool decode() {
        char header[header_length + 1] = "";
        std::memcpy(header, data_, header_length);
        return body_length(std::atoi(header));
    }

private:
    void encode_body(const char *content) {
        std::memcpy(body(), content, body_length());
    }

    void encode_header(size_t body_length) {
        char header[header_length + 1] = "";
        std::sprintf(header, "%4d", static_cast<int>(body_length));
        std::memcpy(data_, header, header_length);
    }

    char data_[header_length + max_body_length];
    size_t body_length_;
};
