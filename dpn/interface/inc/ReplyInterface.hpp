#pragma once

#include <Interface.hpp>

namespace dpn
{


class ReplyInterface : public Interface
{
public:
    ReplyInterface(Peer * peer):Interface(peer){}
    virtual ~ReplyInterface(){};

    virtual void HandleReply(Label & label, Peer::Package & package, Peer::Package& returnPackage) = 0;
    virtual void HandleIncomingMessage(Label & label, Peer::Package & package)
    {
        Peer::Package returnPackage;
        HandleReply(label, package, returnPackage);
        peer_->Push(label, returnPackage);
    }

};


}