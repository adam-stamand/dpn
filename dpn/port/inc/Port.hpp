#pragma once

#include <zmq.hpp>
#include <zmq_addon.hpp>
#include <functional>
#include <deque>
#include <DPNLogger.hpp>
#include <Message.hpp>

namespace dpn
{

class Port
{
public:
    using PortName = std::string;
    using PortContext = zmq::context_t;
    using SocketID = std::string;

    enum class Transport
    {
        inproc,
        ipc,
        tcp
    };
    
    Port(PortName portName, Transport transport,  zmq::context_t &context);
    ~Port();

    void Close();
    void Connect(const PortName & portName);
    void Disconnect(const PortName & portName);
    void Bind(const PortName & portName);
    void Unbind(const PortName & portName);

    void Send(const Port::PortName & portName, Message* message, zmq::send_flags flags = zmq::send_flags::none);
    void Send(const Port::PortName & portName, std::vector<Message*> & messages, zmq::send_flags flags = zmq::send_flags::none);
    void Receive(Port::PortName &portName, Message* message, zmq::recv_flags flags = zmq::recv_flags::none);
    void Receive(Port::PortName &portName, std::vector<Message*>& messages, zmq::recv_flags flags = zmq::recv_flags::none);
 
    PortName GetPortName(){return portName_;}
    Transport GetTransport(){return transport_;}

    // Consider refactor to hide socket (used by peer for poll registration)
    zmq::socket_t& GetSocket(){return socket_;};
    
    static std::string TransportToString(Transport transport_type);
    static inline SocketID PortNameToSocketID(PortName portName, Transport transport);
    static inline PortName PortNameFromSocketID(Port::SocketID socketID);

private:
    void SendData(std::deque<zmq::message_t*>& multiMessage, zmq::send_flags flags);
    void LoadMultipart(std::deque<zmq::message_t*>& multiMessage, std::vector<Message*> & messages);
    void LoadMultipart(std::deque<zmq::message_t*>& multiMessage, Message* message);
    PortName portName_;
    Transport transport_;
    zmq::socket_t socket_;

    zmq::message_t nullMessage_;
    std::vector<PortName> connections_;
    std::vector<PortName> bindings_; 
};



Port::SocketID Port::PortNameToSocketID(PortName portName, Transport transport)
{
    return Port::TransportToString(transport) + portName;
}


Port::PortName Port::PortNameFromSocketID(Port::SocketID socketID)
{
    std::stringstream ss;
    std::string portNameStr;
    ss.str(socketID);
    std::getline(ss, portNameStr, '/');
    std::getline(ss, portNameStr, '/');
    std::getline(ss, portNameStr, '/');
    return PortName(portNameStr);
}




}