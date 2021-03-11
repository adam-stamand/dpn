#pragma once

#include <Interface.hpp>

namespace dpn
{


class SubscriptionInterface : public Interface
{
public:
    SubscriptionInterface(Peer * peer):Interface(peer){}
    virtual ~SubscriptionInterface(){};
    
    enum class SubscriptionStatus
    {
        Pass,
        Fail
    };
    
    virtual SubscriptionStatus HandleSubscription(Label & label, Peer::Package & package) = 0;
    

    virtual void HandleIncomingMessage(Label & label, Peer::Package & package)
    {
        SubscriptionStatus status = HandleSubscription(label, package);
        if (status == SubscriptionStatus::Pass)
        {
            label.Swap();
            peer_->Subscribe(label);
        }
    }
};


}