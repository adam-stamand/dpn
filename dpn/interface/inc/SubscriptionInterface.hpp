#pragma once

#include <Interface.hpp>

namespace dpn
{


class SubscriptionInterface : public Interface
{
public:
    SubscriptionInterface(Peer * peer):Interface(peer){}
    virtual ~SubscriptionInterface(){};


    
    virtual void HandleSubscription(Peer::InterfaceHeader & header, Message & message) = 0;
    virtual void HandleIncomingMessage(Peer::InterfaceHeader & header, Message & message)
    {    
        HandleSubscription(header, message);
    }
};


}