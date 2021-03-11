#pragma once

#include <Interface.hpp>

namespace dpn
{


class SubscriptionInterface : public Interface
{
public:
    SubscriptionInterface(Peer * peer):Interface(peer){}
    virtual ~SubscriptionInterface(){};
    
    virtual void HandleSubscription(Label & label, Peer::Package & package) = 0;
    virtual void HandleIncomingMessage(Label & label, Peer::Package & package)
    {    
        HandleSubscription(label, package);
    }
};


}