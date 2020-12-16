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
    
    virtual void HandleIncomingMessage(Label::InterfaceHeader & header, Message & message) = 0;
protected:
    Peer * peer_;
};




}