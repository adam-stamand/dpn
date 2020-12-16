#include <Peer.hpp>
#include <Interface.hpp>
#include <ctime>
namespace dpn
{

const uint16_t Label::PEER_PORT_ID_DEFAULT = PEER_PORT_ID_DEFAULT_VALUE;

Peer::Peer(PeerID peerID): 
hub_(std::to_string(peerID)),
heartbeatEnabled_(true),logger_(std::to_string(peerID)),peerID_(peerID),headerMessage_(sizeof(Label::Contents)),messageID_(0)
{
    DPN_LOGGER_DEBUG(
        GetLogger(), 
        "Peer::{} created", 
        GetPeerID());
    headerMessage_.resize(sizeof(Label::Contents));
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
    auto subVec = pub_map_.find(label.GetInterfaceID(Endpoint::Self));
    if (subVec == pub_map_.end())
    {
        std::vector<Label> newVec{label};
        pub_map_.insert(std::pair<InterfaceID, std::vector<Label>>(label.GetInterfaceID(Endpoint::Self), newVec));
    }
    else
    {
        subVec->second.push_back(label);
    }
}

void Peer::Publish(Label & label, Package & package)
{
    auto subVec = pub_map_.find(label.GetInterfaceID(Endpoint::Self));
    if (subVec == pub_map_.end())
    {
        DPN_LOGGER_WARN(
            GetLogger(), 
            "Peer::{}/Port::{} unable to locate topic {} for publishing", 
            GetPeerID(), label.GetPortID(Endpoint::Self), label.GetInterfaceID(Endpoint::Self));
        return;
    }

    for (auto & receipt : subVec->second)
    {
        // connDesc.GetEndpoint(Endpoint::Dest) = receipt;
        Push(receipt, package);
    }
}


void Peer::Push(Label & label, Package & package)
{
    SendMessages(label, package);
}


void Peer::GetPendingMessage(
    Label & label, 
    Package & package, 
    const Hub::HubTimeout & timeout)
{
    try
    {
        Poll(label, package, zmq::event_flags::pollin, timeout);
    }
    catch(...)
    {
        throw;
    }
}


void Peer::RequestImpl(
        Label & label, 
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
            Poll(label, package, zmq::event_flags::pollin, currTimeout);
        }
        catch(...)
        {
            return; //TODO
        }

        auto header = static_cast<Label::Contents*>(headerMessage_.buffer());
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
    RequestImpl(label, recvPackage, timeout);
}


void Peer::Poll(
    Label & label, 
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
        label.SetPortID(localPortID, Endpoint::Dest);
        ReceiveMessages(label, package);
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
    auto header = static_cast<Label::Contents*>(headerMessage_.buffer());
    label.SetPeerID(GetPeerID(), Endpoint::Self);
    header->connDesc_ = label.GetDescription();
    
    spec.hubName_ = PeerIDtoHubName(label.GetPeerID(Endpoint::Dest));
    spec.portID_ = label.GetPortID(Endpoint::Dest);
    
    messageID_++;
    header->messageID_ = messageID_;

    package.insert(package.begin(), &headerMessage_);
    hub_.Send(label.GetPortID(Endpoint::Self), spec, package);
}


void Peer::ReceiveMessages(Label & label, Package package)
{
    Hub::PortSpecification spec;
    package.insert(package.begin(), &headerMessage_);
    hub_.Receive(label.GetPortID(Endpoint::Dest), spec, package);
    
    auto header = static_cast<Label::Contents*>(headerMessage_.buffer());
    label.GetContents() = *header;

    if (header->src_.portID_ != spec.portID_)
    {
        DPN_LOGGER_WARN(
            GetLogger(), 
            "Peer::{}/Port::{} received bad port ID in header {}", 
            GetPeerID(), connDesc.GetPortID(Endpoint::Dest), header->src_.portID_);
    }
    else if (header->src_.peerID_ != HubNametoPeerID(spec.hubName_))
    {
        DPN_LOGGER_WARN(
            GetLogger(), 
            "Peer::{}/Port::{} received bad peer ID in header {}", 
            GetPeerID(), connDesc.GetPortID(Endpoint::Dest), header->src_.peerID_);
    }
}



void Peer::ServiceInterfaces(Label & label, Package package, const Hub::HubTimeout timeout)
{
    // TODO check return value/exception
    try
    {
        GetPendingMessage(label, package, timeout);
    }
    catch(...)
    {
        return;
    }
    auto header = static_cast<Label::Contents*>(headerMessage_.buffer());

    auto interfacePair = interface_map_.find(label.GetInterfaceID(Endpoint::Dest));
    if (interfacePair == interface_map_.end())
    {
        DPN_LOGGER_WARN(
            GetLogger(), 
            "Peer::{} failed to locate interface {}", 
            GetPeerID(), connDesc.GetInterfaceID(Endpoint::Dest));
        return;
    }

    DPN_LOGGER_DEBUG(
        GetLogger(), 
        "Peer::{} interface {} called", 
        GetPeerID(), connDesc.GetInterfaceID(Endpoint::Dest));
    auto interface = interfacePair->second;
    interface->HandleIncomingMessage(*header, package);
}

void Peer::AddPort(
    PortID localPortID,
    Port::Transport transport,  
    Port::PortContext &context, 
    zmq::event_flags flags)
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
    // std::cout << "Heartbeat sent" << std::endl;
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
