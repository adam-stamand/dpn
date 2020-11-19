#pragma once

#include <Interface.hpp>

namespace dpn
{


class TopicInterface : public Interface
{
public:

    enum class TopicStatus
    {
        Pass,
        Fail
    };

    TopicInterface(Peer * peer):Interface(peer){}
    virtual ~TopicInterface(){};

    virtual TopicStatus HandleTopic(Peer::InterfaceHeader & header, Message & message) = 0;


    virtual void HandleIncomingMessage(Peer::InterfaceHeader & header, Message & message)
    {
        ConnectionDescription connDesc; 
        connDesc.SetDescription(header.connDesc_);
        connDesc.Swap();
        
        TopicStatus status = HandleTopic(header, message);
        if (status == TopicStatus::Pass)
        {
            peer_->Subscribe(connDesc);
        }
    }

};


}