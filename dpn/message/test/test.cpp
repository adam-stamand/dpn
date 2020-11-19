
#include <gtest/gtest.h>
#include <Port.hpp>
#include <functional>
#include <vector>
#include <zmq.hpp>
#include <DPNLogger.hpp>

using namespace dpn;


TEST(PortTest, TwoPortSend)
{
    Port::PortName portName;
    zmq::context_t context{0};

    Port port1("port1", Port::Transport::inproc,  context);
    Port port2("port2", Port::Transport::inproc,  context);

    port1.Connect("port2");
    port2.Connect("port1");

    
    auto message1 = port1.GetMessage();
    auto message2 = port2.GetMessage();

    strncpy((char*)message2.GetOutgoingBuffer(), "Hello", Port::DEFAULT_MESSAGE_SIZE);
    message2.SetOutgoingBuffer(sizeof("Hello"));
    port2.Send("port1");
    port1.Receive(portName);
    ASSERT_STREQ((char*)message1.GetIncomingBuffer(), "Hello");
    

    strncpy((char*)message1.GetOutgoingBuffer(), "Goodbye", Port::DEFAULT_MESSAGE_SIZE);
    message1.SetOutgoingBuffer(sizeof("Goodbye"));
    port1.Send("port2");
    port2.Receive(portName);
    ASSERT_STREQ((char*)message2.GetIncomingBuffer(), "Goodbye");
    

    
}


TEST(PortTest, ThreePortSend)
{
    Port::PortData portData;
    Port::PortName portName;
    zmq::context_t context{0};

    Port port1("port1",Port::Transport::inproc,  context);
    Port port2("port2",Port::Transport::inproc,  context);
    Port port3("port3",Port::Transport::inproc,  context);

    port1.Connect("port2");
    port1.Connect("port3");
    port2.Connect("port1");
    port2.Connect("port3");
    port3.Connect("port1");
    port3.Connect("port2");

    auto message1 = port1.GetMessage();
    auto message2 = port2.GetMessage();
    auto message3 = port3.GetMessage();
    
    strncpy((char*)message2.GetOutgoingBuffer(), "2->1", Port::DEFAULT_MESSAGE_SIZE);
    message2.SetOutgoingBuffer(sizeof("2->1"));
    port2.Send("port1");
    port1.Receive(portName);
    ASSERT_STREQ((char*)message1.GetIncomingBuffer(), "2->1");
    
    strncpy((char*)message3.GetOutgoingBuffer(), "3->1", Port::DEFAULT_MESSAGE_SIZE);
    message3.SetOutgoingBuffer(sizeof("3->1"));
    port3.Send("port1");
    port1.Receive(portName);
    ASSERT_STREQ((char*)message1.GetIncomingBuffer(), "3->1");

    strncpy((char*)message1.GetOutgoingBuffer(), "1->2", Port::DEFAULT_MESSAGE_SIZE);
    message1.SetOutgoingBuffer(sizeof("1->2"));
    port1.Send("port2");
    port2.Receive(portName);
    ASSERT_STREQ((char*)message2.GetIncomingBuffer(), "1->2");

    strncpy((char*)message3.GetOutgoingBuffer(), "3->2", Port::DEFAULT_MESSAGE_SIZE);
    message3.SetOutgoingBuffer(sizeof("3->2"));
    port3.Send("port2");
    port2.Receive(portName);
    ASSERT_STREQ((char*)message2.GetIncomingBuffer(), "3->2");

    strncpy((char*)message1.GetOutgoingBuffer(), "1->3", Port::DEFAULT_MESSAGE_SIZE);
    message1.SetOutgoingBuffer(sizeof("1->3"));
    port1.Send("port3");
    port3.Receive(portName);
    ASSERT_STREQ((char*)message3.GetIncomingBuffer(), "1->3");

    strncpy((char*)message2.GetOutgoingBuffer(), "2->3", Port::DEFAULT_MESSAGE_SIZE);
    message2.SetOutgoingBuffer(sizeof("2->3"));
    port2.Send("port3");
    port3.Receive(portName);
    ASSERT_STREQ((char*)message3.GetIncomingBuffer(), "2->3");

    
}
 

TEST(PortTest, ThreePortRoute)
{
    Port::PortName portName;
    Port::PortData portData;
    zmq::context_t context{0};

    Port port1("port1", Port::Transport::inproc,  context);
    Port port2("port2", Port::Transport::inproc,  context);
    Port port3("port3", Port::Transport::inproc,  context);

    port1.Connect("port2");
    port2.Connect("port1");
    port2.Connect("port3");
    port3.Connect("port2");

    
    auto message1 = port1.GetMessage();
    auto message3 = port3.GetMessage();


    strncpy((char*)message1.GetOutgoingBuffer(), "Hello", Port::DEFAULT_MESSAGE_SIZE);
    message1.SetOutgoingBuffer(sizeof("Hello"));
    auto vec = std::vector<Port::PortName>{"port2","port3"};
    port1.Send(vec);
    port2.Receive(portName, zmq::recv_flags::dontwait);
    port3.Receive(portName);

    ASSERT_STREQ((char*)message3.GetIncomingBuffer(), "Hello");
    ASSERT_STREQ(portName.c_str(), "port2");
}
 
int main(int argc, char **argv) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
