
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
    void HandlePull(Label & label, Peer::Package & package)
    {
        ASSERT_STREQ((char*)package[0]->buffer(), expectedString_.c_str());
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
    void HandleSubscription(Label & Label, Peer::Package & package)
    {
        ASSERT_STREQ((char*)package[0]->buffer(), expectedString_.c_str());
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

    void HandleReply(Label & label, Peer::Package & package, Peer::Package & returnPackage)
    {
        ASSERT_STREQ((char*)package[0]->buffer(), "REQUEST_INTERFACE");
        
        strncpy((char*)returnPackage[0]->buffer(), "REQUEST_REPLY", returnPackage[0]->capacity());
        returnPackage[0]->resize(sizeof("REQUEST_REPLY"));
    }


private:
    Message returnMessage_;
};


class TestTopicInterface : public TopicInterface
{
public:
    TestTopicInterface(Peer* peer):TopicInterface(peer){}

    TopicStatus HandleTopic(Label & label, Peer::Package & package)
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
    TestPeer(PeerID peerID, std::string expectedString = ""): 
    Peer(peerID),pullInterface_(this, expectedString),topicInterface_(this), replyInterface_(this), subscriptionInterface_(this,expectedString)
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
    
    // Create two peers, specifying peer ID
    TestPeer peer0(Peer::PeerID(0));
    TestPeer peer1(Peer::PeerID(1));

    // Add default ports to each peer
    peer0.AddPort(Port::Transport::inproc, context, zmq::event_flags::pollin);
    peer1.AddPort(Port::Transport::inproc, context, zmq::event_flags::pollin);

    // Connect two peers together
    peer0.Connect(Peer::PeerID(1));
    peer1.Connect(Peer::PeerID(0));

    // Close default ports on each peer
    peer0.ClosePort();
    peer1.ClosePort();
}



TEST(PeerTest, Push)
{
    zmq::context_t context{0};
    Label label;

    // Create two peers, specifying peer ID
    TestPeer peer0(Peer::PeerID(0), "peer0");
    TestPeer peer1(Peer::PeerID(1), "peer1");
        
    // Add a default port to each peer
    peer0.AddPort(Port::Transport::inproc, context, zmq::event_flags::pollin);
    peer1.AddPort(Port::Transport::inproc, context, zmq::event_flags::pollin);

    // Connect the two peers' ports together
    peer0.Connect(Peer::PeerID(1));
    peer1.Connect(Peer::PeerID(0));

    // Create message for pushing
    Message message1;
    strncpy((char*)message1.buffer(), "peer1", message1.capacity());
    message1.resize(sizeof("peer1"));

    // Push message to from peer 0 to peer 1
    label.SetPeerID(Peer::PeerID(1), Peer::Endpoint::Dest);
    label.SetInterfaceID(TestInterfaceID::Pull, Peer::Endpoint::Dest);
    peer0.Push(label, {&message1});

    // Service the interfaces on peer 1 to check for incoming messages
    peer1.ServiceInterfaces(Hub::HubTimeout(100));
    EXPECT_EQ(peer1.PushPullHandled(), true);

    peer0.Disconnect(Peer::PeerID(1));
    peer1.Disconnect(Peer::PeerID(0));

    // Close default ports
    peer0.ClosePort();
    peer1.ClosePort();
}


TEST(PeerTest, Pull)
{
    zmq::context_t context{0};
    Label label;

    // Create two peers, specifying peer ID
    TestPeer peer0(Peer::PeerID(0), "peer0");
    TestPeer peer1(Peer::PeerID(1), "peer1");

    // Add a default port to each peer
    peer0.AddPort(Port::Transport::inproc, context, zmq::event_flags::pollin);
    peer1.AddPort(Port::Transport::inproc, context, zmq::event_flags::pollin);

    // Connect the two peers' ports together
    peer0.Connect(Peer::PeerID(1));
    peer1.Connect(Peer::PeerID(0));

    // Create message for pushing
    Message message1;
    strncpy((char*)message1.buffer(), "peer1", message1.capacity());
    message1.resize(sizeof("peer1"));

    // Push message to from peer 0 to peer 1
    label.SetPeerID(Peer::PeerID(1), Peer::Endpoint::Dest);
    label.SetInterfaceID(TestInterfaceID::Pull, Peer::Endpoint::Dest);
    peer0.Push(label, {&message1});

    // Pull message explicitly
    Message message2;
    peer1.GetPendingPackage(label, {&message2}, Hub::HubTimeout(100));
    ASSERT_STREQ((char*)message2.buffer(), "peer1");

    // Close default ports
    peer0.ClosePort();
    peer1.ClosePort();
}



TEST(PeerTest, PubSub)
{
    zmq::context_t context{0};
    Label label;

    // Create two peers, specifying peer ID
    TestPeer peer0(Peer::PeerID(0), "peer0");
    TestPeer peer1(Peer::PeerID(1), "peer1");

    // Add a default port to each peer
    peer0.AddPort(Port::Transport::inproc, context, zmq::event_flags::pollin);
    peer1.AddPort(Port::Transport::inproc, context, zmq::event_flags::pollin);

    // Connect the two peers' ports together
    peer0.Connect(Peer::PeerID(1));
    peer1.Connect(Peer::PeerID(0));

    // Create Blank subscription message
    Message message1;
    message1.resize(0);

    // Push subscription from peer 0 to peer 1
    label.SetPeerID(Peer::PeerID(1), Peer::Endpoint::Dest);
    label.SetInterfaceID(TestInterfaceID::Topic, Peer::Endpoint::Dest);
    peer0.Push(label, {&message1});

    // Wait for subscription to arrive on peer 1 and service it
    peer1.ServiceInterfaces(Hub::HubTimeout(100));

    // Create message to publish
    Message message2;
    strncpy((char*)message2.buffer(), "peer1", message2.capacity());
    message2.resize(sizeof("peer1"));
    
    // Publish message using peer 1
    label.SetInterfaceID(TestInterfaceID::Topic, Peer::Endpoint::Src);
    peer1.Publish(label, {&message2});

    // Pull published message to peer 0
    label.SetInterfaceID(TestInterfaceID::Pull, Peer::Endpoint::Src);
    label.SetPeerID(Peer::PeerID(1), Peer::Endpoint::Src);
    peer0.GetPendingPackage(label, {&message1}, Hub::HubTimeout(100));
    EXPECT_STREQ((char*)message1.buffer(), "peer1");

    // Close default ports
    peer0.ClosePort();
    peer1.ClosePort();
}


static bool peer1RunThread = true;

static void Peer1RequestThread(zmq::context_t * context)
{
    Label connDesc;

    // Create peer, specifying peer ID
    TestPeer peer1(Peer::PeerID(1), "peer1");

    // Add a default port to peer
    peer1.AddPort(Port::Transport::inproc, *context, zmq::event_flags::pollin);
    
    // Connect the two peers' ports together
    peer1.Connect(Peer::PeerID(0));

    // Conatantly service interfaces
    while(peer1RunThread)
    {
        peer1.ServiceInterfaces(Hub::HubTimeout(100));
    }
    
    // Close default port
    peer1.ClosePort();
}

TEST(PeerTest, Request)
{
    zmq::context_t context{0};
    Label label;

    // Create a thread for peer 1 so that it can reply to blocked peer 0 request
    std::thread t(&Peer1RequestThread, &context);

    sleep(1);

    // Create peer, specifying peer ID
    TestPeer peer0(Peer::PeerID(0), "peer0");

    // Add a default port to peer
    peer0.AddPort(Port::Transport::inproc, context, zmq::event_flags::pollin);

    // Connect the two peers' ports together
    peer0.Connect(Peer::PeerID(1));

    // Create request message
    Message message1;
    strncpy((char*)message1.buffer(), "REQUEST_INTERFACE", message1.capacity());
    message1.resize(sizeof("REQUEST_INTERFACE"));

    // Send request from peer 0 to peer 1
    Message recvMessage;
    label.SetPeerID(Peer::PeerID(1), Peer::Endpoint::Dest);
    label.SetInterfaceID(TestInterfaceID::Reply, Peer::Endpoint::Dest);
    peer0.Request(label, {&message1}, {&recvMessage}, Hub::HubTimeout(3000));
    ASSERT_STREQ((char*)recvMessage.buffer(), "REQUEST_REPLY");

    // Close peer 1 thread
    peer1RunThread = false;
    t.join();

    // Close default port
    peer0.ClosePort();
}


 
int main(int argc, char **argv) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
