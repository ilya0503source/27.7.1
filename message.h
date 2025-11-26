#pragma once
#include <string>
#include "user.h"

class Message {
private:
    const int _from;
    const int _to;
    const std::string _text;


public:
    Message(int from, int to, const std::string& text)
        : _from(from), _to(to), _text(text) {
    }

    int getFrom() const { return _from; }
    int getTo() const { return _to; }
    const std::string& getText() const { return _text; }
};