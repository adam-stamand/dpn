#include <Port.hpp>
#include <iostream>
#include <DPNLogger.hpp>

namespace dpn
{

Port::Port(PortName portName, Transport transport, zmq::context_t &context):
portName_(portName),transport_(transport),socket_(context, zmq::socket_type::router),nullMessage_(0)
{
    socket_.set(zmq::sockopt::routing_id, PortNameToSocketID(portName_, transport_));
    socket_.set(zmq::sockopt::router_mandatory, true);
    Bind(portName_);
}
    
Port::~Port()
{
    Close();
}

void Port::Close()
{
    DPN_SYS_DEBUG(
        "Port::{} is closing on Transport::{}", 
        portName_, transport_);
    
    socket_.close();
}

void Port::Connect(const PortName & portName)
{
    DPN_SYS_DEBUG(
        "Port::{} connecting to Port::{} on Transport::{}", 
        portName_, portName, transport_);
    
    socket_.connect(PortNameToSocketID(portName, transport_));
    connections_.push_back(portName);
}

void Port::Disconnect(const PortName & portName)
{
    auto connectionIter = std::find(connections_.begin(), connections_.end(), portName);
    if (connectionIter == connections_.end())
    {
        DPN_SYS_WARN(
            "Port::{} failed to disconnect from Port::{} on Transport::{}", 
            portName_, portName, transport_);
        return;
    }

    DPN_SYS_DEBUG(
        "Port::{} disconnecting from Port::{} on Transport::{}", 
        portName_, portName, transport_);
    
    socket_.disconnect(PortNameToSocketID(portName, transport_));
    connections_.erase(connectionIter);
    
}


void Port::Bind(const PortName & portName)
{
    DPN_SYS_DEBUG(
        "Port::{} binding to Port::{} on Transport::{}", 
        portName_, portName, transport_);
    
    socket_.bind(PortNameToSocketID(portName_, transport_));
    bindings_.push_back(portName);
}


void Port::Unbind(const PortName & portName)
{
    auto bindingIter = std::find(bindings_.begin(), bindings_.end(), portName);
    if (bindingIter == bindings_.end())
    {
        DPN_SYS_WARN(
            "Port::{} failed to unbind from Port::{} on Transport::{}", 
            portName_, portName, transport_);
        return;
    }
    
    DPN_SYS_DEBUG(
        "Port::{} unbinding from Port::{} on Transport::{}", 
        portName_, portName, transport_);
    
    socket_.unbind(PortNameToSocketID(portName, transport_));
    bindings_.erase(bindingIter);
    
}

void Port::Receive(
    Port::PortName &portName, 
    Message * message,
    zmq::recv_flags flags)
{
    
    std::deque<zmq::message_t*> multiMessage;
    std::vector<zmq::message_t> messageVec(10);
    bool rv = true; 

    bool more = true;
    int idx = 0;

    while (more) 
    {
        if (!socket_.recv(messageVec.at(idx), static_cast<zmq::recv_flags>(flags)))
        {
            rv = false;
            break;
        }

        more = messageVec.at(idx).more();
        multiMessage.push_back(&messageVec.at(idx));
        idx++;
    }

    if (multiMessage.size() < 3)
    {
        DPN_SYS_WARN(
            "Port::{} received incomplete package on Transport::{}: flags = {}", 
            portName_, transport_, flags);
        rv = false;
    }

    if (rv)
    {
        Port::SocketID socketId = std::string(multiMessage.front()->data<char>(), multiMessage.front()->size());
        portName = Port::PortNameFromSocketID(socketId);
        multiMessage.pop_front();

        // Check if next frame is NULL; if not, route message
        auto nextFrame = std::string(multiMessage.front()->data<char>(), multiMessage.front()->size());

        if (nextFrame != "")
        {
            // SendData(multiMessage, zmq::send_flags::dontwait);
            DPN_SYS_WARN(
                "Port::{} received malformed message on Transport::{}: flags = {}", 
                portName_, transport_, flags);
        }
        else
        {
            multiMessage.pop_front(); // remove null frame
            size_t cpySize = 0;
            size_t bufferOffset = 0;
            while (multiMessage.size())
            {
                if (multiMessage.front()->size() > message->capacity())
                {
                    DPN_SYS_WARN(
                        "Port::{} received contents too large for buffer (truncating) from Port::{} on Transport::{}: flags = {}, data ={:Xn}", 
                        portName_, portName, transport_, flags, 
                        spdlog::to_hex(static_cast<uint8_t*>(multiMessage.front()->data<unsigned char>()), static_cast<uint8_t*>(multiMessage.front()->data()) + multiMessage.front()->size()));
                    cpySize = message->capacity();
                }
                else
                {
                    cpySize = multiMessage.front()->size();
                }
                auto dest = reinterpret_cast<unsigned char*>(message->buffer()) + bufferOffset;
                memcpy(dest, multiMessage.front()->data(), cpySize); 
                multiMessage.pop_front();
                bufferOffset += cpySize;
            }
            message->resize(bufferOffset);

            DPN_SYS_DEBUG(
                "Port::{} received package from Port::{} on Transport::{}: flags = {}, data ={:Xn}", 
                portName_, portName, transport_, flags, 
                spdlog::to_hex(static_cast<uint8_t*>(message->buffer()), static_cast<uint8_t*>(message->buffer()) + message->size()));
        }
    }
    else if (flags != zmq::recv_flags::dontwait)
    {
        DPN_SYS_WARN(
            "Port::{} failed to receive message on Transport::{}: flags = {}", 
            portName_, transport_, flags);
    }
    else
    {
        DPN_SYS_INFO(
            "Port::{} timed out while receiving message on Transport::{}: flags = {}", 
            portName_,transport_, flags);
    }
    

}



void Port::Receive(
    Port::PortName &portName, 
    std::vector<Message*>& messages,
    zmq::recv_flags flags)
{
    
    std::deque<zmq::message_t*> multiMessage;
    std::vector<zmq::message_t> messageVec(10);
    bool rv = true; 

    bool more = true;
    int idx = 0;

    while (more) 
    {
        if (!socket_.recv(messageVec.at(idx), static_cast<zmq::recv_flags>(flags)))
        {
            rv = false;
            break;
        }

        more = messageVec.at(idx).more();
        multiMessage.push_back(&messageVec.at(idx));
        idx++;
    }

    if (multiMessage.size() < 3)
    {
        DPN_SYS_WARN(
            "Port::{} received incomplete package on Transport::{}: flags = {}", 
            portName_, transport_, flags);
        rv = false;
    }

    if (rv)
    {
        Port::SocketID socketId = std::string(multiMessage.front()->data<char>(), multiMessage.front()->size());
        portName = Port::PortNameFromSocketID(socketId);
        multiMessage.pop_front();

        // Check if next frame is NULL; if not, route message
        auto nextFrame = std::string(multiMessage.front()->data<char>(), multiMessage.front()->size());

        if (nextFrame != "")
        {
            // SendData(multiMessage, zmq::send_flags::dontwait);
            DPN_SYS_WARN(
                "Port::{} received malformed message on Transport::{}: flags = {}", 
                portName_, transport_, flags);
        }
        else
        {
            multiMessage.pop_front(); // remove null frame
            size_t cpySize = 0;
            size_t bufferOffset = 0;
            size_t messageIdx = 0;
            while (multiMessage.size())
            {
                auto message = messages.at(messageIdx);
                if (multiMessage.front()->size() > message->capacity())
                {
                    DPN_SYS_WARN(
                        "Port::{} received contents too large for buffer (truncating) from Port::{} on Transport::{}: flags = {}, data ={:Xn}", 
                        portName_, portName, transport_, flags, 
                        spdlog::to_hex(static_cast<uint8_t*>(multiMessage.front()->data<unsigned char>()), static_cast<uint8_t*>(multiMessage.front()->data()) + multiMessage.front()->size()));
                    cpySize = message->capacity();
                }
                else
                {
                    cpySize = multiMessage.front()->size();
                }

                auto dest = reinterpret_cast<unsigned char*>(message->buffer()) + bufferOffset;
                memcpy(dest, multiMessage.front()->data(), cpySize); 
                multiMessage.pop_front();
                bufferOffset += cpySize;
                
                message->resize(bufferOffset);
                
                if (messageIdx < messages.size()-1)
                {
                    messageIdx++;
                    bufferOffset = 0;
                }
                
                DPN_SYS_DEBUG(
                    "Port::{} received message from Port::{} on Transport::{}: flags = {}, data ={:Xn}", 
                    portName_, portName, transport_, flags, 
                    spdlog::to_hex(static_cast<uint8_t*>(message->buffer()), static_cast<uint8_t*>(message->buffer()) + message->size()));
            }

        }
    }
    else if (flags != zmq::recv_flags::dontwait)
    {
        DPN_SYS_WARN(
            "Port::{} failed to receive message on Transport::{}: flags = {}", 
            portName_, transport_, flags);
    }
    else
    {
        DPN_SYS_INFO(
            "Port::{} timed out while receiving message on Transport::{}: flags = {}", 
            portName_,transport_, flags);
    }
    

}


void Port::Send(const Port::PortName & portName, Message* message, zmq::send_flags flags)
{
    std::deque<zmq::message_t*> multiMessage;
    zmq::message_t addressMessage(Port::PortNameToSocketID(portName, transport_).c_str(), Port::PortNameToSocketID(portName, transport_).size());
    multiMessage.push_back(&addressMessage);
    LoadMultipart(multiMessage, message);
    SendData(multiMessage, flags);
}

void Port::Send(const Port::PortName & portName, std::vector<Message*> & messages, zmq::send_flags flags)
{
    std::deque<zmq::message_t*> multiMessage;
    zmq::message_t addressMessage(Port::PortNameToSocketID(portName, transport_).c_str(), Port::PortNameToSocketID(portName, transport_).size());
    multiMessage.push_back(&addressMessage);
    LoadMultipart(multiMessage, messages);
    SendData(multiMessage, flags);
}


void Port::SendData(std::deque<zmq::message_t*>& multiMessage, zmq::send_flags flags)
{
    
    Port::PortName portName(static_cast<char*>(multiMessage.front()->data()), multiMessage.front()->size());
    bool rv = true;
    while (multiMessage.size()) 
    {
        auto message = multiMessage.front();
        multiMessage.pop_front();
        DPN_SYS_DEBUG(
            "Port::{} sending message to Port::{}: flags = {}, data ={:Xn}", 
            portName_, Port::PortNameFromSocketID(portName), flags, 
            spdlog::to_hex(static_cast<const uint8_t*>(message->data()), static_cast<const uint8_t*>(message->data()) + message->size()));
    
        if (!socket_.send(*message, static_cast<zmq::send_flags>(
                                    (multiMessage.size() ? ZMQ_SNDMORE : 0) | static_cast<int>(flags))))
        {
            rv = false;
            break;
        }
    }

    if (!rv)
    {
        DPN_SYS_WARN(
            "Port::{} failed to send message to Port::{}: flags = {}", 
            portName_, portName, flags);
        throw std::runtime_error("Failed to send message");
    }
}

void Port::LoadMultipart(std::deque<zmq::message_t*>& multiMessage, std::vector<Message*>& messages)
{
    multiMessage.push_back(&nullMessage_); // add NULL Frame
   
    // Add main content to multi message
    for (auto message : messages)
    {
        multiMessage.push_back(message->contents());
    }
}

void Port::LoadMultipart(std::deque<zmq::message_t*>& multiMessage, Message* message)
{
    multiMessage.push_back(&nullMessage_); // add NULL Frame
   
    // Add main content to multi message
    multiMessage.push_back(message->contents());
}


    
std::string Port::TransportToString(Transport transport_type)
{
    switch(transport_type)
    {
        case Transport::inproc:
            return "inproc://";
        default:
            assert(0);
    }
    return "";
}

} //namespace dpn