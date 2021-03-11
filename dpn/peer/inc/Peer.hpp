#pragma once

#include <iostream>
#include <DPNLogger.hpp>
#include <Hub.hpp>
#include <zmq.hpp>
#include <Label.hpp>

namespace dpn
{

#define TEMP_MACRO 0

class Interface;
// class TopicInterface;
class SubscriptionInterface;


class Peer
{
public:

    using PeerID = Label::PeerID;
    using InterfaceID = Label::InterfaceID;
    using PortID = Label::PortID;
    using MessageID = Label::MessageID;
    using Endpoint = Label::LabelEndpoint;
    using Package = std::vector<Message*>;

    explicit Peer(PeerID peerID, Message::MessageSize recvMessageSize = 0x2000); 
    
    // friend TopicInterface;
    friend SubscriptionInterface;






    /**
     * @brief RPC - Request 
     * 
     * @details Sends request to a remote peer's reply interface. The Connection description is used to 
     * determine the remote peer and reply inteface to use. The contents of sendMessage should be set locally 
     * before the Request() call, and the contents of recvMessage are set remotely and accessible after the
     * call. 
     * 
     * The Request() will block until either a reply is received by the requested interface, or the timeout
     * expires, whichever comes first.  
     * 
     * @param connDesc Connection Description; Dest peer ID and interface ID used to identify reply interface
     * @param sendMessage Message send to reply interface
     * @param recvMessage Message received by reply interface
     * @param timeout Milliseconds to wait for recvMessage
     */
    void Request(
        Label & label,
        Package & sendMessage, 
        Package & recvMessage,
        Hub::HubTimeout timeout = Hub::HubTimeout(0));
    void Request(
        Label & label,
        Package && sendMessage, 
        Package && recvMessage,
        Hub::HubTimeout timeout = Hub::HubTimeout(0));


    /**
     * @brief Pub/Sub - Publish
     * 
     * @details Publishes data to subscription interfaces that are subscribed. The Connection description is
     * used to determine the interface ID to publish on (interface ID is used as the topic). The contents of 
     * message should be set locally before calling Publish(). The contents will be sent to each subscription
     * interface that has previously subscribed.
     * 
     * This call does not wait or expect any reply. It will return as soon as the message has been sent to each 
     * subscriber.
     * 
     * @param connDesc Connection Description; Self Interface ID used as topic
     * @param package Package to be published
     */
    void Publish(Label & label, Package & package);
    void Publish(Label & label, Package && package);

    /**
     * @brief Push/Pull - Push
     * 
     * @details Pushes data to a specified peer and pull interface. The Connection description is used to
     * determine the remote peer and its pull interface. The contents of message should be set locally 
     * before calling Push(). The contents will be sent to the peer on its specifid pull inteface.
     * 
     * This call does not wait or expect any reply. It will return as soon as the message has been sent.
     * 
     * @param connDesc Connection Description; Dest peer ID and interface ID used to identify pull interface
     * @param message Message to be pushed
     */
    void Push(Label & label, Package & package);
    void Push(Label & label, Package && package);
 

    /**
     * @brief Service Interfaces - Handle pending messages for interfaces
     * 
     * @details Checks for any pending/queued messages waiting to be read. If a message is found, the contents
     * will be copied into the contents of the user supplied message. This message will then be forwarded to the
     * appropriate interface according to the inteface specified in the header. 
     * 
     * This call will block until either an interface is serviced, or until the timeout expires, whichever comes first.
     * 
     * @param timeout Milliseconds to wait for pending message
     */
    void ServiceInterfaces(const Hub::HubTimeout timeout);

    /**
     * @brief Get Pending Message
     * 
     * @details Checks for any pending/queued messages waiting to be read. If a message is found, the conents will
     * be copied into the user supplied message. The function then returns, allowing the user to 
     * handle the data directly.
     * 
     * This call will block until either a message is read, or until the timeout expires, whichever comes first. 
     *  
     * @param connDesc Connection Description; Set by function to value in received message header
     * @param message Message used for storing contents of read queued message
     * @param timeout Milliseconds to wait for pending message
     */
    void GetPendingPackage(
        Label & label, 
        Package & package, 
        const Hub::HubTimeout & timeout);
    void GetPendingPackage(
        Label & label, 
        Package && package, 
        const Hub::HubTimeout & timeout);


    // Port Managment
    void AddPort(
        Port::Transport transport,  
        Port::PortContext &context, 
        zmq::event_flags flags, 
        PortID localPortID = Label::PEER_PORT_ID_DEFAULT);
    void RemovePort(PortID localPortID = Label::PEER_PORT_ID_DEFAULT);
    void ClosePort(PortID localPortID = Label::PEER_PORT_ID_DEFAULT);
    void Connect(PeerID peerID, PortID portID = Label::PEER_PORT_ID_DEFAULT, PortID localPortID = Label::PEER_PORT_ID_DEFAULT);
    void Disconnect(PeerID peerID, PortID portID = Label::PEER_PORT_ID_DEFAULT, PortID localPortID = Label::PEER_PORT_ID_DEFAULT);
protected:
    virtual ~Peer();
    
    PeerID GetPeerID();
    dpn::Logger & GetLogger(){return logger_;}
    void AddInterface(InterfaceID id, Interface * interface)
    {
        interface_map_.insert(std::pair<InterfaceID, Interface*>(id,interface));
    }

    
    bool PollEventPresent(zmq::event_flags flags, zmq::event_flags event);

private:



    Hub hub_;
    bool heartbeatEnabled_;
    dpn::Logger logger_;
    PeerID peerID_;
    Message labelMessage_;
    Message recvMessage_;
    Package recvPackage_;
    uint32_t messageID_;
    void SendHeartbeat();
    std::unordered_map<InterfaceID, Interface*> interface_map_; 
    
    void Subscribe(Label & label);
    void SendMessages(Label & label, Package & messages);
    void ReceiveMessages(PortID destPortID, Package & package);

    void Poll(
        Package & package, 
        zmq::event_flags flags,
        const Hub::HubTimeout & timeout);

    inline Hub::HubName PeerIDtoHubName(PeerID peerID)
    {
        return std::to_string(peerID);
    }
    
    inline PeerID HubNametoPeerID(Hub::HubName hubName)
    {
        return static_cast<PeerID>(stoi(hubName));
    }

    void RequestImpl(
        Package & recvMessage,
        Hub::HubTimeout timeout);


    std::unordered_map<InterfaceID, std::vector<Label>> pub_map_;
};



}