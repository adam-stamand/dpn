#pragma once

#include <Interface.hpp>

namespace dpn
{


class ReplyInterface : public Interface
{
public:
    ReplyInterface(Peer * peer):Interface(peer){}
    virtual ~ReplyInterface(){};

    virtual void HandleReply(Peer::InterfaceHeader & header, Message & message, Message *& pt_returnMessage) = 0;
    virtual void HandleIncomingMessage(Peer::InterfaceHeader & header, Message & message)
    {
        ConnectionDescription connDesc; 
        connDesc.SetDescription(header.connDesc_);
        connDesc.Swap();
        Message * returnMessage;
        HandleReply(header, message, returnMessage);
        peer_->Push(connDesc, returnMessage);
    }

};


}