#pragma once

#include <zmq.hpp>
#include <zmq_addon.hpp>
#include <Port.hpp>
#include <unordered_map>
#include <functional>
#include <iostream>

namespace dpn
{


class Hub
{
public:
    using HubName = std::string;
    using PortID = uint16_t; 
    using HubTimeout = std::chrono::milliseconds;
        

    Hub(const HubName & hubName);
    virtual ~Hub();

    class PortSpecification
    {
    public:
        PortSpecification() = default;
        PortSpecification(const Port::PortName & portName) {SetPortName(portName);}
        PortSpecification(const HubName hubName, PortID portID): hubName_(hubName), portID_(portID){}

        HubName hubName_;
        PortID portID_;
        Port::PortName GetPortName() const {return hubName_ + "-" + std::to_string(portID_);};
        void SetPortName(const Port::PortName & portName)
        {
            std::stringstream ss;
            std::string temp;
            ss.str(portName);
            std::getline(ss, hubName_, '-');
            std::getline(ss, temp, '-');
            portID_ = stoi(temp);
        }
    };


    void PollCallback(const PortID & localPortID, zmq::event_flags flags);

    void AddPort(
        PortID portID,Port::Transport transport,  
        Port::PortContext &context, 
        zmq::event_flags flags = zmq::event_flags::none); 

    void RemovePort(PortID localPortID);

    void Close();
    void ClosePort(const PortID & localPortID);
    void Connect(PortID localPortID, const PortSpecification & portSpec);
    void Disconnect(PortID localPortID, const PortSpecification & portSpec);
    void Unbind(PortID localPortID, const PortSpecification & portSpec);
    void Send(PortID localPortID, const PortSpecification & portSpec, Message* message);
    void Send(PortID localPortID, const PortSpecification & portSpec, std::vector<Message*>& messages);
    void Receive(PortID localPortID, PortSpecification & portSpec, Message* message);
    void Receive(PortID localPortID, PortSpecification & portSpec, std::vector<Message*>& messages);
    

    void Poll(Hub::PortID & localPortID, zmq::event_flags & flags, std::chrono::milliseconds timeout);
    HubName & GetHubName(){return hubName_;}
    
private:
    HubName hubName_;
    zmq::active_poller_t poller_;
    std::unordered_map<PortID, Port*> ports_;
    PortID pollPortID_;
    zmq::event_flags pollEventFlags_;
    Port* FindPort(PortID localPortID);
};




}