#include <Peer.hpp>
#include <Interface.hpp>
#include <ctime>
namespace dpn
{

const uint16_t Label::PEER_PORT_ID_DEFAULT = PEER_PORT_ID_DEFAULT_VALUE;

Peer::Peer(PeerID peerID, Message::MessageSize recvMessageSize): 
hub_(std::to_string(peerID)),
heartbeatEnabled_(true),logger_(std::to_string(peerID)),peerID_(peerID),labelMessage_(sizeof(Label::Contents)),recvMessage_(recvMessageSize),messageID_(0)
{
    DPN_LOGGER_DEBUG(
        GetLogger(), 
        "Peer::{} created", 
        GetPeerID());
    labelMessage_.resize(sizeof(Label::Contents));
    recvPackage_.push_back(&recvMessage_);
}


Peer::~Peer()
{
    // TODO is it OK to use logger in destructor?
    DPN_LOGGER_DEBUG(
        GetLogger(), 
        "Peer::{} destroyed", 
        GetPeerID());    
}


void Peer::Subscribe(Label & label)
{
    auto subVec = pub_map_.find(label.GetInterfaceID(Endpoint::Src));
    if (subVec == pub_map_.end())
    {
        pub_map_.insert({label.GetInterfaceID(Endpoint::Src), {label}});
    }
    else
    {
        subVec->second.push_back(label);
    }
}

void Peer::Publish(Label & label, Package & package)
{
    auto subVec = pub_map_.find(label.GetInterfaceID(Endpoint::Src));
    if (subVec == pub_map_.end())
    {
        DPN_LOGGER_WARN(
            GetLogger(), 
            "Peer::{}/Port::{} unable to locate topic {} for publishing", 
            GetPeerID(), label.GetPortID(Endpoint::Src), label.GetInterfaceID(Endpoint::Src));
        return;
    }

    for (auto & receipt : subVec->second)
    {
        Push(receipt, package);
    }
}

void Peer::Publish(Label & label, Package && package)
{
    Publish(label, package);
}


void Peer::Push(Label & label, Package & package)
{
    SendMessages(label, package);
}
void Peer::Push(Label & label, Package && package)
{
    Push(label, package);
}


void Peer::GetPendingPackage(
    Label & label, 
    Package & package, 
    const Hub::HubTimeout & timeout)
{
    try
    {
        Poll(package, zmq::event_flags::pollin, timeout);
    }
    catch(...)
    {
        throw;
    }
    
    //TODO potential unaligned memory access
    auto labelContents = static_cast<Label::Contents*>(labelMessage_.buffer());
    label.GetContents() = *labelContents; 
}

void Peer::GetPendingPackage(
    Label & label, 
    Package && package, 
    const Hub::HubTimeout & timeout)
{
    GetPendingPackage(label, package, timeout);
}


void Peer::RequestImpl(
        Package & package,
        Hub::HubTimeout timeout)
{
    std::chrono::milliseconds startTime;
    std::chrono::milliseconds currTime;
    Hub::HubTimeout currTimeout = timeout;
    Hub::HubTimeout elapsedTimeout = timeout;   
    startTime = std::chrono::duration_cast< std::chrono::milliseconds >(
      std::chrono::system_clock::now().time_since_epoch()
    );
    
    while (elapsedTimeout <= timeout)
    {
        try
        {
            Poll(package, zmq::event_flags::pollin, currTimeout);
        }
        catch(...)
        {
            return; //TODO
        }

        auto header = static_cast<Label::Contents*>(labelMessage_.buffer());
        if (header->messageID_  == messageID_)
        {
            // Found correct message
            return;
        }
        else
        {
            // TODO queue message instead of dropping it
            DPN_LOGGER_WARN(
                GetLogger(), 
                "Peer::{} received message with ID::{} while expecting ID::{}. Dropping message.", 
                GetPeerID(), header->messageID_,  messageID_);
        }

        // Determine how long was waited
        currTime = std::chrono::duration_cast< std::chrono::milliseconds >(
        std::chrono::system_clock::now().time_since_epoch());
        
        elapsedTimeout = Hub::HubTimeout(currTime - startTime);
        currTimeout = timeout - elapsedTimeout;
    }
}

void Peer::Request(
        Label & label, 
        Package & sendPackage, 
        Package & recvPackage,
        Hub::HubTimeout timeout) 
{
    Push(label, sendPackage);
    RequestImpl(recvPackage, timeout);
}

void Peer::Request(
        Label & label, 
        Package && sendPackage, 
        Package && recvPackage,
        Hub::HubTimeout timeout) 
{
    Request(
        label,
        sendPackage,
        recvPackage,
        timeout);
}


void Peer::Poll(
    Package & package, 
    zmq::event_flags flags,
    const Hub::HubTimeout & timeout)
{
    PortID localPortID;
    Hub::PortSpecification spec;
    zmq::event_flags polledFlags = flags;

    try
    {
        hub_.Poll(localPortID, polledFlags, timeout);
    }
    catch(...)
    {
        throw;
    }

    if (PollEventPresent(polledFlags,zmq::event_flags::pollin))
    {
        ReceiveMessages(localPortID, package);
    }
    else
    {
        DPN_LOGGER_WARN(
            GetLogger(), 
            "Peer::{}/Port::{} received unhandled event {}", 
            GetPeerID(), localPortID, flags);
        throw std::runtime_error("Unhandled event while polling");
    }

}


void Peer::SendMessages(Label & label, Package package)
{
    Hub::PortSpecification spec;
    // TODO may result in unaligned access
    auto labelContent = static_cast<Label::Contents*>(labelMessage_.buffer());
    label.SetPeerID(GetPeerID(), Endpoint::Src);
    *labelContent = label.GetContents();

    spec.hubName_ = PeerIDtoHubName(label.GetPeerID(Endpoint::Dest));
    spec.portID_ = label.GetPortID(Endpoint::Dest);
    
    messageID_++;
    labelContent->messageID_ = messageID_;

    package.insert(package.begin(), &labelMessage_);
    hub_.Send(label.GetPortID(Endpoint::Src), spec, package);
}


void Peer::ReceiveMessages(PortID destPortID, Package package)
{
    Hub::PortSpecification spec;
    package.insert(package.begin(), &labelMessage_);
    hub_.Receive(destPortID, spec, package);
    // TODO may result in unaligned access
    auto labelContents = static_cast<Label::Contents*>(labelMessage_.buffer());

    if (labelContents->src_.portID_ != spec.portID_)
    {
        DPN_LOGGER_WARN(
            GetLogger(), 
            "Peer::{}/Port::{} received bad port ID {} in label contents", 
            GetPeerID(), labelContents->dest_.portID_, labelContents->src_.portID_);
    }
    else if (labelContents->src_.peerID_ != HubNametoPeerID(spec.hubName_))
    {
        DPN_LOGGER_WARN(
            GetLogger(), 
            "Peer::{}/Port::{} received bad peer ID in header {}", 
            GetPeerID(), labelContents->dest_.portID_, labelContents->src_.peerID_);
    }
}



void Peer::ServiceInterfaces(const Hub::HubTimeout timeout)
{
    Label label;
    // TODO check return value/exception
    try
    {
        GetPendingPackage(label, recvPackage_, timeout);
    }
    catch(...)
    {
        return;
    }

    auto interfacePair = interface_map_.find(label.GetInterfaceID(Endpoint::Dest));
    if (interfacePair == interface_map_.end())
    {
        DPN_LOGGER_WARN(
            GetLogger(), 
            "Peer::{} failed to locate interface {}", 
            GetPeerID(), label.GetInterfaceID(Endpoint::Dest));
        return;
    }

    DPN_LOGGER_DEBUG(
        GetLogger(), 
        "Peer::{} interface {} called", 
        GetPeerID(), label.GetInterfaceID(Endpoint::Dest));
    auto interface = interfacePair->second;
    interface->HandleIncomingMessage(label, recvPackage_);
}

void Peer::AddPort(
    Port::Transport transport,  
    Port::PortContext &context, 
    zmq::event_flags flags,
    PortID localPortID)
{
    hub_.AddPort(localPortID, transport, context, flags);
}

void Peer::RemovePort(PortID localPortID)
{
    hub_.RemovePort(localPortID);
}


void Peer::ClosePort(PortID localPortID)
{
    hub_.ClosePort(localPortID);
}

void Peer::Connect(PeerID peerID, PortID portID, PortID localPortID)
{
    Hub::PortSpecification spec(PeerIDtoHubName(peerID), portID);
    hub_.Connect(localPortID, spec);
}

void Peer::Disconnect(PeerID peerID, PortID portID, PortID localPortID)
{
    Hub::PortSpecification spec(PeerIDtoHubName(peerID), portID);
    hub_.Disconnect(portID, spec);
}



void Peer::SendHeartbeat()
{
    std::cout << "Heartbeat sent" << std::endl;
}

bool Peer::PollEventPresent(zmq::event_flags flags, zmq::event_flags event)
{
    return (flags & event) == event; 
}



Peer::PeerID Peer::GetPeerID()
{
    return peerID_;
}


}
