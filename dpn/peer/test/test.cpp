
#include <gtest/gtest.h>
#include <Peer.hpp>
#include <PullInterface.hpp>
#include <ReplyInterface.hpp>
#include <TopicInterface.hpp>
#include <SubscriptionInterface.hpp>

using namespace dpn;

/*************************************************
* User  defined Interface IDs
*************************************************/
namespace TestInterfaceID
{

enum TestInterfaceID : Peer::InterfaceID
{
    Pull,
    Topic,
    Reply,
    Subscription
};
}

/*************************************************
* Interfaces
*************************************************/

class TestPullInterface : public PullInterface
{
public: 
    TestPullInterface(Peer * peer, std::string expectedString):PullInterface(peer),expectedString_(expectedString){}
    void HandlePull(Peer::InterfaceHeader & header, Message & message)
    {
        ASSERT_STREQ((char*)message.buffer(), expectedString_.c_str());
        messageHandled = true;
    }
    bool messageHandled = false;
private:
    std::string expectedString_;


};

class TestSubscriptionInterface : public SubscriptionInterface
{
public: 
    TestSubscriptionInterface(Peer * peer, std::string expectedString):SubscriptionInterface(peer),expectedString_(expectedString){}
    void HandleSubscription(Label::InterfaceHeader & header, Message & message)
    {
        ASSERT_STREQ((char*)message.buffer(), expectedString_.c_str());
        messageHandled = true;
    }
    bool messageHandled = false;
private:
    std::string expectedString_;
};


class TestReplyInterface : public ReplyInterface
{
public:
    TestReplyInterface(Peer* peer):ReplyInterface(peer){}

    void HandleReply(Label::InterfaceHeader & header, Message & message, Message *& pt_returnMessage)
    {
        ASSERT_STREQ((char*)message.buffer(), "REQUEST_INTERFACE");
        
        pt_returnMessage = &returnMessage_;
        strncpy((char*)returnMessage_.buffer(), "REQUEST_REPLY", returnMessage_.capacity());
        returnMessage_.resize(sizeof("REQUEST_REPLY"));
    }


private:
    Message returnMessage_;
};


class TestTopicInterface : public TopicInterface
{
public:
    TestTopicInterface(Peer* peer):TopicInterface(peer){}

    TopicStatus HandleTopic(Peer::InterfaceHeader & header, Message & message)
    {
        return TopicStatus::Pass;
    }
};





/*************************************************
* Peer implementation
*************************************************/

class TestPeer : public Peer
{
public:
    TestPeer(ConnectionDescription connDesc, std::string expectedString = ""): 
    Peer(connDesc),pullInterface_(this, expectedString),topicInterface_(this), replyInterface_(this), subscriptionInterface_(this,expectedString)
    {
        AddInterface(static_cast<Peer::InterfaceID>(TestInterfaceID::Pull), &pullInterface_);
        AddInterface(static_cast<Peer::InterfaceID>(TestInterfaceID::Topic), &topicInterface_);
        AddInterface(static_cast<Peer::InterfaceID>(TestInterfaceID::Reply), &replyInterface_);
        AddInterface(static_cast<Peer::InterfaceID>(TestInterfaceID::Subscription), &subscriptionInterface_);
    }

    bool PushPullHandled(){return pullInterface_.messageHandled;}


private:
    TestPullInterface pullInterface_;
    TestTopicInterface topicInterface_;
    TestReplyInterface replyInterface_;
    TestSubscriptionInterface subscriptionInterface_;
};




/*************************************************
* Tests
*************************************************/


TEST(PeerTest, Connect)
{
    zmq::context_t context{0};
    ConnectionDescription connDesc;
    
    // Create two peers, specifying peer ID
    connDesc.SetPeerID(Peer::PeerID(0), Peer::Endpoint::Self);
    TestPeer peer0(connDesc);
    connDesc.SetPeerID(Peer::PeerID(1), Peer::Endpoint::Self);
    TestPeer peer1(connDesc);

    // Add default ports to each peer
    connDesc.SetPortID(Peer::PEER_PORT_ID_DEFAULT, Peer::Endpoint::Self); // Already set to default; here as example
    peer0.AddPort(connDesc, Port::Transport::inproc, context, zmq::event_flags::pollin);
    peer1.AddPort(connDesc, Port::Transport::inproc, context, zmq::event_flags::pollin);

    // Connect two peers together
    connDesc.SetPeerID(Peer::PeerID(1), Peer::Endpoint::Dest);
    peer0.Connect(connDesc);
    connDesc.SetPeerID(Peer::PeerID(0), Peer::Endpoint::Dest);
    peer1.Connect(connDesc);

    // Close default ports on each peer
    peer0.ClosePort(connDesc);
    peer1.ClosePort(connDesc);
}



TEST(PeerTest, Push)
{
    zmq::context_t context{0};
    ConnectionDescription connDesc;

    // Create two peers, specifying peer ID
    connDesc.SetPeerID(Peer::PeerID(0), Peer::Endpoint::Self);
    TestPeer peer0(connDesc, "peer0");
    connDesc.SetPeerID(Peer::PeerID(1), Peer::Endpoint::Self);
    TestPeer peer1(connDesc, "peer1");
        
    // Add a default port to each peer
    peer0.AddPort(connDesc, Port::Transport::inproc, context, zmq::event_flags::pollin);
    peer1.AddPort(connDesc, Port::Transport::inproc, context, zmq::event_flags::pollin);

    // Connect the two peers' ports together
    connDesc.SetPeerID(Peer::PeerID(1), Peer::Endpoint::Dest);
    peer0.Connect(connDesc);
    connDesc.SetPeerID(Peer::PeerID(0), Peer::Endpoint::Dest);
    peer1.Connect(connDesc);

    // Create message for pushing
    Message message1;
    strncpy((char*)message1.buffer(), "peer1", message1.capacity());
    message1.resize(sizeof("peer1"));

    // Push message to from peer 0 to peer 1
    connDesc.SetPeerID(Peer::PeerID(1), Peer::Endpoint::Dest);
    connDesc.SetInterfaceID(TestInterfaceID::Pull, Peer::Endpoint::Dest);
    peer0.Push(connDesc, &message1);

    // Service the interfaces on peer 1 to check for incoming messages
    peer1.ServiceInterfaces(connDesc, &message1, Hub::HubTimeout(100));
    ASSERT_EQ(peer1.PushPullHandled(), true);

    // Close default ports
    peer0.ClosePort(connDesc);
    peer1.ClosePort(connDesc);
}


TEST(PeerTest, Pull)
{
    zmq::context_t context{0};
    ConnectionDescription connDesc;

    // Create two peers, specifying peer ID
    connDesc.SetPeerID(Peer::PeerID(0), Peer::Endpoint::Self);
    TestPeer peer0(connDesc, "peer0");
    connDesc.SetPeerID(Peer::PeerID(1), Peer::Endpoint::Self);
    TestPeer peer1(connDesc, "peer1");

    // Add a default port to each peer
    peer0.AddPort(connDesc, Port::Transport::inproc, context, zmq::event_flags::pollin);
    peer1.AddPort(connDesc, Port::Transport::inproc, context, zmq::event_flags::pollin);

    // Connect the two peers' ports together
    connDesc.SetPeerID(Peer::PeerID(1), Peer::Endpoint::Dest);
    peer0.Connect(connDesc);
    connDesc.SetPeerID(Peer::PeerID(0), Peer::Endpoint::Dest);
    peer1.Connect(connDesc);

    // Create message for pushing
    Message message1;
    strncpy((char*)message1.buffer(), "peer1", message1.capacity());
    message1.resize(sizeof("peer1"));

    // Push message to from peer 0 to peer 1
    connDesc.SetPeerID(Peer::PeerID(1), Peer::Endpoint::Dest);
    connDesc.SetInterfaceID(TestInterfaceID::Pull, Peer::Endpoint::Dest);
    peer0.Push(connDesc, &message1);

    // Pull message explicitly
    Message message2;
    peer1.GetPendingMessage(connDesc, &message2, Hub::HubTimeout(100));
    ASSERT_STREQ((char*)message2.buffer(), "peer1");

    // Close default ports
    peer0.ClosePort(connDesc);
    peer1.ClosePort(connDesc);
}



TEST(PeerTest, PubSub)
{
    zmq::context_t context{0};
    ConnectionDescription connDesc;

    // Create two peers, specifying peer ID
    connDesc.SetPeerID(Peer::PeerID(0), Peer::Endpoint::Self);
    TestPeer peer0(connDesc, "peer0");
    connDesc.SetPeerID(Peer::PeerID(1), Peer::Endpoint::Self);
    TestPeer peer1(connDesc, "peer1");

    // Add a default port to each peer
    peer0.AddPort(connDesc, Port::Transport::inproc, context, zmq::event_flags::pollin);
    peer1.AddPort(connDesc, Port::Transport::inproc, context, zmq::event_flags::pollin);

    // Connect the two peers' ports together
    connDesc.SetPeerID(Peer::PeerID(1), Peer::Endpoint::Dest);
    peer0.Connect(connDesc);
    connDesc.SetPeerID(Peer::PeerID(0), Peer::Endpoint::Dest);
    peer1.Connect(connDesc);

    // Create Blank subscription message
    Message message1;
    message1.resize(0);

    // Push subscription from peer 0 to peer 1
    connDesc.SetPeerID(Peer::PeerID(1), Peer::Endpoint::Dest);
    connDesc.SetInterfaceID(TestInterfaceID::Topic, Peer::Endpoint::Dest);
    peer0.Push(connDesc, &message1);

    // Wait for subscription to arrive on peer 1 and service it
    peer1.ServiceInterfaces(connDesc, &message1, Hub::HubTimeout(100));

    // Create message to publish
    Message message2;
    strncpy((char*)message2.buffer(), "peer1", message2.capacity());
    message2.resize(sizeof("peer1"));
    
    // Publish message using peer 1
    connDesc.SetInterfaceID(TestInterfaceID::Topic, Peer::Endpoint::Self);
    peer1.Publish(connDesc, &message2);

    // Pull published message to peer 0
    connDesc.SetInterfaceID(TestInterfaceID::Pull, Peer::Endpoint::Self);
    connDesc.SetPeerID(Peer::PeerID(1), Peer::Endpoint::Self);
    peer0.GetPendingMessage(connDesc, &message1, Hub::HubTimeout(100));
    ASSERT_STREQ((char*)message1.buffer(), "peer1");

    // Close default ports
    peer0.ClosePort(connDesc);
    peer1.ClosePort(connDesc);
}


static bool peer1RunThread = true;

static void Peer1RequestThread(zmq::context_t * context)
{
    
    ConnectionDescription connDesc;

    // Create peer, specifying peer ID
    connDesc.SetPeerID(Peer::PeerID(1), Peer::Endpoint::Self);
    TestPeer peer1(connDesc, "peer1");

    // Add a default port to peer
    peer1.AddPort(connDesc, Port::Transport::inproc, *context, zmq::event_flags::pollin);
    
    // Connect the two peers' ports together
    connDesc.SetPeerID(Peer::PeerID(0), Peer::Endpoint::Dest);
    peer1.Connect(connDesc);

    // Conatantly service interfaces
    while(peer1RunThread)
    {
        Message message;
        peer1.ServiceInterfaces(connDesc, &message, Hub::HubTimeout(100));
    }
    
    // Close default port
    peer1.ClosePort(connDesc);
}

TEST(PeerTest, Request)
{
    zmq::context_t context{0};
    ConnectionDescription connDesc;

    // Create a thread for peer 1 so that it can reply to blocked peer 0 request
    std::thread t(&Peer1RequestThread, &context);

    sleep(1);

    // Create peer, specifying peer ID
    connDesc.SetPeerID(Peer::PeerID(0), Peer::Endpoint::Self);
    TestPeer peer0(connDesc, "peer0");

    // Add a default port to peer
    peer0.AddPort(connDesc, Port::Transport::inproc, context, zmq::event_flags::pollin);

    // Connect the two peers' ports together
    connDesc.SetPeerID(Peer::PeerID(1), Peer::Endpoint::Dest);
    peer0.Connect(connDesc);

    // Create request message
    Message message1;
    strncpy((char*)message1.buffer(), "REQUEST_INTERFACE", message1.capacity());
    message1.resize(sizeof("REQUEST_INTERFACE"));

    // Send request from peer 0 to peer 1
    Message recvMessage;
    connDesc.SetPeerID(Peer::PeerID(1), Peer::Endpoint::Dest);
    connDesc.SetInterfaceID(TestInterfaceID::Reply, Peer::Endpoint::Dest);
    peer0.Request(connDesc, &message1, &recvMessage, Hub::HubTimeout(3000));
    ASSERT_STREQ((char*)recvMessage.buffer(), "REQUEST_REPLY");

    // Close peer 1 thread
    peer1RunThread = false;
    t.join();

    // Close default port
    peer0.ClosePort(connDesc);

}


 
int main(int argc, char **argv) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
