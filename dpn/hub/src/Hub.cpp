#include <Hub.hpp>

#include <DPNLogger.hpp>

namespace dpn
{


Hub::Hub(const HubName & hubName):hubName_(hubName){};

Hub::~Hub()
{
};


void Hub::Close()
{
    for (auto& port : ports_)
    {   
        port.second->Close();
    }
}

void Hub::ClosePort(const PortID & localPortID)
{
    auto port = FindPort(localPortID);
    if (port == nullptr)
    {
        DPN_SYS_WARN(
            "Hub::{} failed to close to Port::{}", 
            hubName_, localPortID);
        return;
    }
    port->Close();
}

void Hub::PollCallback(const PortID & localPortID, zmq::event_flags flags)
{
    pollPortID_ = localPortID;
    pollEventFlags_ = flags;
};

void Hub::AddPort(
    PortID portID,Port::Transport transport,  
    Port::PortContext &context, 
    zmq::event_flags flags) 
{
    PortSpecification spec(hubName_, portID);
    Port* port = new Port(spec.GetPortName(), transport, context);
 
 
    ports_.insert(std::pair<PortID, Port*>(portID, port));
    poller_.add(
        port->GetSocket(), 
        flags, 
        [&,portID](zmq::event_flags flags){PollCallback(portID, flags);});
    
    DPN_SYS_DEBUG(
        "Port::{} added on Transport::{}: flags = {}", 
        spec.GetPortName(), portID, transport, flags);
}

void Hub::RemovePort(PortID portID)
{
    auto port = ports_.find(portID)->second;
    ports_.erase(portID);
    poller_.remove(port->GetSocket());
    
    DPN_SYS_DEBUG(
        "Hub::{}:{} removed ", 
        hubName_, portID);
}


void Hub::Connect(PortID localPortID, const PortSpecification & portSpec)
{
    auto port = FindPort(localPortID);
    if (port == nullptr)
    {
        DPN_SYS_WARN(
            "Hub::{}:{} failed to connect to Hub::{}:{}", 
            hubName_, localPortID, portSpec.hubName_, portSpec.portID_);
        return;
    }
    port->Connect(portSpec.GetPortName());
}

void Hub::Disconnect(PortID localPortID, const PortSpecification & portSpec)
{
    auto port = FindPort(localPortID);
    if (port == nullptr)
    {
        DPN_SYS_WARN(
            "Hub::{}:{} failed to disconnect from Hub::{}:{}", 
            hubName_, localPortID, portSpec.hubName_, portSpec.portID_);
        return;
    }
    port->Disconnect(portSpec.GetPortName());
}

void Hub::Unbind(PortID localPortID, const PortSpecification & portSpec)
{
    auto port = FindPort(localPortID);
    if (port == nullptr)
    {
        DPN_SYS_WARN(
            "Hub::{}:{} failed to unbind from Hub::{}:{}", 
            hubName_, localPortID, portSpec.hubName_, portSpec.portID_);
        return;
    }
    port->Unbind(portSpec.GetPortName());
}

void Hub::Send(const PortID localPortID, const PortSpecification & portSpec, std::vector<Message*>& messages)
{
    auto port = FindPort(localPortID);
    if (port == nullptr)
    {
        DPN_SYS_WARN(
            "Hub::{}:{} failed to send to Hub::{}/:{}", 
            hubName_, localPortID, portSpec.hubName_, portSpec.portID_);
        return;
    }
    port->Send(portSpec.GetPortName(), messages);
}

void Hub::Send(const PortID localPortID, const PortSpecification & portSpec, Message* message)
{
    auto port = FindPort(localPortID);
    if (port == nullptr)
    {
        DPN_SYS_WARN(
            "Hub::{}:{} failed to send to Hub::{}/:{}", 
            hubName_, localPortID, portSpec.hubName_, portSpec.portID_);
        return;
    }
    port->Send(portSpec.GetPortName(), message);
}


void Hub::Receive(PortID localPortID, PortSpecification & portSpec, Message * message)
{
    Port::PortName portName;
    auto port = FindPort(localPortID);
    if (port == nullptr)
    {
        // DPN_SYS_WARN(
        //     "Hub::{}:{} failed to receive from Hub::{}/:{}", 
        //     hubName_, localPortID, portSpec.hubName_, portSpec.portID_);
        return; //TODO handle error
    }
    port->Receive(portName, message);
    portSpec.SetPortName(portName);
}

void Hub::Receive(PortID localPortID, PortSpecification & portSpec, std::vector<Message*> & messages)
{
    Port::PortName portName;
    auto port = FindPort(localPortID);
    if (port == nullptr)
    {
        // DPN_SYS_WARN(
        //     "Hub::{}:{} failed to receive from Hub::{}/:{}", 
        //     hubName_, localPortID, portSpec.hubName_, portSpec.portID_);
        return; //TODO handle error
    }
    port->Receive(portName, messages);
    portSpec.SetPortName(portName);
}


void Hub::Poll(Hub::PortID & localPortID, zmq::event_flags & flags, HubTimeout timeout)
{
    // Set event flags for polling in each port
    for (auto & port : ports_)
    {
        poller_.modify(port.second->GetSocket(), flags);
    }

    auto rv = poller_.wait(timeout);
    if (rv == 0)
    {
        //TODO use exceptions
        DPN_SYS_WARN(
            "Hub::{} timed out while polling - flags={} timeout={}", 
            hubName_, static_cast<int>(flags), timeout.count());
        throw std::runtime_error("Timed out while polling");  
    }
    localPortID = pollPortID_;
    flags = pollEventFlags_;
}


Port* Hub::FindPort(PortID portID)
{
    auto iter = ports_.find(portID);
    if (iter == ports_.end())
    {
        DPN_SYS_WARN(
            "Hub::{}-{} could not be found", 
            hubName_, portID);
        return nullptr;
    }
    
    return iter->second;
}

}