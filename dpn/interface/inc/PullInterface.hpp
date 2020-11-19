#pragma once

#include <Interface.hpp>

namespace dpn
{


class PullInterface : public Interface
{
public:
    PullInterface(Peer * peer):Interface(peer){}
    virtual ~PullInterface(){};

    virtual void HandlePull(Peer::InterfaceHeader & header, Message & message) = 0;
    virtual void HandleIncomingMessage(Peer::InterfaceHeader & header, Message & message)
    {    
        HandlePull(header, message);
    }


};


}