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

    chat_message(const char msg[]) { encode(msg); }

    chat_message(const std::string msg) { encode(msg.c_str()); }

    const char *data() const { return data_; }

    char *data() { return data_; }

    size_t length() const { return header_length + body_length_; }

    const char *body() const { return data_ + header_length; }

    char *body() { return data_ + header_length; }

    void encode_body(char *content) {
        std::memcpy(body(), content, body_length());
    }

    bool encode(const char *msg) {
        body_length_ = strlen(msg);
        if (body_length_ > max_body_length) {
            body_length_ = 0;
            return false;
        }
        encode_header();

        std::memcpy(body(), msg, body_length_);
        return true;
    }

    void encode_header(int body_length) {
        char header[header_length + 1] = "";
        std::sprintf(header, "%4d", body_length);
        std::memcpy(data_, header, header_length);
    }

    size_t body_length() const { return body_length_; }

    void body_length(size_t new_length) {
        body_length_ =
            new_length > max_body_length ? max_body_length : new_length;
    }

    bool decode_header() {
        char header[header_length + 1] = "";
        std::memcpy(header, data_, header_length);
        body_length_ = std::atoi(header);
        if (body_length_ > max_body_length) {
            body_length_ = 0;
            return false;
        }
        return true;
    }

    void encode_header() {
        char header[header_length + 1] = "";
        std::sprintf(header, "%4d", static_cast<int>(body_length_));
        std::memcpy(data_, header, header_length);
    }

private:
    char data_[header_length + max_body_length];
    size_t body_length_;
};
