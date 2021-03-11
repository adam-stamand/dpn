#pragma once

#include <Interface.hpp>

namespace dpn
{


class PullInterface : public Interface
{
public:
    PullInterface(Peer * peer):Interface(peer){}
    virtual ~PullInterface(){};

    virtual void HandlePull(Label & label, Peer::Package & package) = 0;
    virtual void HandleIncomingMessage(Label & label, Peer::Package & package)
    {    
        HandlePull(label, package);
    }


};


}