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

    virtual TopicStatus HandleTopic(Label & label, Peer::Package & package) = 0;


    virtual void HandleIncomingMessage(Label & label, Peer::Package & package)
    {
        TopicStatus status = HandleTopic(label, package);
        if (status == TopicStatus::Pass)
        {
            peer_->Subscribe(label);
        }
    }

};


}