
#include <gtest/gtest.h>
#include <zmq.hpp>
#include <Hub.hpp>
#include <DPNLogger.hpp>

using namespace dpn;



// class PollHub : public Hub
// {
// public:
//     PollHub(HubName hubName) : Hub(hubName){};
//     void PollCallback(const PortID & localPortID, zmq::event_flags flags)
//     {
//         if ((flags & zmq::event_flags::pollin) == zmq::event_flags::pollin)
//         {
//             Message recvMessage;
//             PortSpecification spec;
//             Receive(localPortID, spec, &recvMessage);
//             ASSERT_EQ(*static_cast<Hub::PortID*>(recvMessage.buffer()), spec.portID_);
//         }
//         else
//         {
//             std::cout << "event not handled" << "\n";
//         }
//     }

// };


TEST(HubTest, HubPoll)
{
    Hub hub("hub1");
    zmq::context_t context{0};
    const Hub::PortID peer1Port = 1;
    const Hub::PortID peer2Port = 2;
    Hub::PortID * portID;
    zmq::event_flags flags = zmq::event_flags::pollin;

    hub.AddPort(peer1Port, Port::Transport::inproc, context, zmq::event_flags::pollin);
    hub.AddPort(peer2Port, Port::Transport::inproc, context, zmq::event_flags::pollin);

    Hub::PortSpecification spec;

    spec.hubName_ = "hub1";
    spec.portID_ = peer2Port;
    hub.Connect(peer1Port, spec);

    spec.hubName_ = "hub1"; // Intentionally redundant for verbosity
    spec.portID_ = peer1Port;
    hub.Connect(peer2Port, spec);


    Message message1;
    Message message2;
    
    portID = static_cast<Hub::PortID*>(message1.buffer());
    *portID = peer1Port;
    message1.resize(sizeof(portID));
    
    portID = static_cast<Hub::PortID*>(message2.buffer());
    *portID = peer2Port;
    message2.resize(sizeof(portID));

    spec.hubName_ = "hub1";
    spec.portID_ = peer2Port;
    hub.Send(peer1Port, spec, &message1);

    spec.hubName_ = "hub1";
    spec.portID_ = peer1Port;
    hub.Send(peer2Port, spec, &message2);

    hub.Poll(*portID, flags, Hub::HubTimeout(1000));

    hub.Receive(*portID, spec, &message1);
    ASSERT_EQ(*static_cast<Hub::PortID*>(message1.buffer()), spec.portID_);
    
    
    hub.Close();

    context.close();
}


 
int main(int argc, char **argv) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
