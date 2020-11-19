#include <Message.hpp>
#include <iostream>
#include <DPNLogger.hpp>

namespace dpn
{



Message::Message(MessageSize capacity):
size_(0),offset_(0),capacity_(capacity)
{
    buffer_ = new unsigned char[capacity];
    contents_ = new zmq::message_t(buffer_,0,nullptr);
}

Message::MessageBuffer  Message::buffer(){return reinterpret_cast<MessageBuffer>(reinterpret_cast<unsigned char*>(buffer_) + offset_);}
Message::MessageSize    Message::capacity(){return capacity_;}
Message::MessageSize    Message::size(){return size_;}
Message::MessageSize    Message::offset(){return offset_;}

void Message::resize(MessageSize size, MessageSize offset)
{
    if (size + offset > capacity_)
    {
        DPN_SYS_WARN(
                "Size exceeds max size of message - size = {}, offset = {}, maxSize = {}", 
                size, offset, capacity_);
        return; 
    }
    size_ = size;
    offset_ = offset;
    contents_->rebuild((void*)buffer_, size_, nullptr, nullptr); // TODO look into implementation to ensure buffer isn't messed with
}

void Message::reset(){size_=0;offset_=0;}


} //namespace dpn