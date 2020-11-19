
#include <gtest/gtest.h>
#include <Socket.hpp>
#include <functional>
#include <vector>
#include <zmq.hpp>


using namespace dpn;


TEST(RouterSocketTest, Request) {
    zmq::context_t context{0};
    Socket::SocketName socketName = "inproc://testComponent";
    Socket routerSocket(socketName, zmq::socket_type::req, context);
    zmq::socket_t replySocket(context, zmq::socket_type::rep);

    replySocket.bind("inproc://testComponent");
    routerSocket.Connect("inproc://testComponent");

    Socket::SocketData data("hello");
    routerSocket.Send(socketName, data);

    zmq::message_t message(32);
    replySocket.recv(message, zmq::recv_flags::none);
    replySocket.recv(message, zmq::recv_flags::none);
    replySocket.recv(message, zmq::recv_flags::none);
    std::cout << message.to_string() << std::endl;
    
    zmq::message_t replyMessage("GoodBye", 8);
    replySocket.send(replyMessage, zmq::send_flags::none);

    Socket::SocketConnID connID;

    //TODO SHould test with another router, not a reply
    // Socket::SocketData data2(50);
    // routerSocket.Receive(connID,data2);
    // std::cout << data2.to_string() << std::endl;
    // ASSERT_STREQ(data2.to_string().c_str(), "GoodBye");
}
 

 
int main(int argc, char **argv) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
