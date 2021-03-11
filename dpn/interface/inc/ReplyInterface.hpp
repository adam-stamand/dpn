#pragma once

#include <Interface.hpp>

namespace dpn
{


class ReplyInterface : public Interface
{
public:
    ReplyInterface(Peer * peer):Interface(peer){}

    enum class ReplyStatus
    {
        Pass,
        Fail
    };

    virtual ~ReplyInterface(){};

    virtual ReplyStatus HandleReply(Label & label, Peer::Package & package, Peer::Package& returnPackage) = 0;
    virtual void HandleIncomingMessage(Label & label, Peer::Package & package)
    {
        Peer::Package returnPackage;
        ReplyStatus status = HandleReply(label, package, returnPackage);
        if (status == ReplyStatus::Pass)
        {
            label.Swap();
            peer_->Push(label, returnPackage);
        }
    }

};


}