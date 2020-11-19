#pragma once

#include <zmq.hpp>
#include <zmq_addon.hpp>
#include <functional>
#include <deque>
#include <DPNLogger.hpp>
#include <unordered_map>

namespace dpn
{


class Message
{
public:
    using MessageBuffer = void *;
    using MessageSize = size_t;
    using MessageName = std::string;


    const static unsigned int DEFAULT_MESSAGE_CAPACITY = 0x1000;

    Message(MessageSize capacity = DEFAULT_MESSAGE_CAPACITY);
    
    MessageBuffer buffer();
    MessageSize capacity();
    MessageSize size();
    MessageSize offset();

    void resize(MessageSize size, MessageSize offset = 0);

    void reset();

    zmq::message_t * contents(){return contents_;}

private:
    MessageSize size_;
    MessageSize offset_;
    const MessageSize capacity_;
    
    zmq::message_t * contents_;
    MessageBuffer buffer_;




};

}