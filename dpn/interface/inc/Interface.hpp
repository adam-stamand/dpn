#pragma once
#include <cstdint>
#include <cstdio>
#include <Message.hpp>
#include <Peer.hpp>

namespace dpn
{


#define INTERFACES private


class Interface
{
public:


    Interface(Peer * peer):peer_(peer){}
    virtual ~Interface(){};
    
    virtual void HandleIncomingMessage(Label & label, Peer::Package & package) = 0;
protected:
    Peer * peer_;
};




}