#pragma once


#include <iostream>
#include <DPNLogger.hpp>
#include <Hub.hpp>
#include <zmq.hpp>
#include <optional>

namespace dpn
{


class Interface;

class Label
{

public:

    using PeerID = uint16_t;
    using InterfaceID = uint16_t;
    using PortID = Hub::PortID;
    using MessageID = uint32_t;

    #define PEER_PORT_ID_DEFAULT_VALUE 0

    struct EndpointDescriptionStruct
    {
        PeerID peerID_;
        PortID portID_;
        InterfaceID interfaceID_;
    };

    struct LabelStruct
    {
        EndpointDescriptionStruct self_;
        EndpointDescriptionStruct dest_;
    };
    
    enum class LabelEndpoint : uint32_t
    {
        Self,
        Dest
    };


    const static uint16_t PEER_PORT_ID_DEFAULT;

    Label()
    {
        label_.self_.portID_ = PEER_PORT_ID_DEFAULT;
        label_.self_.peerID_ = 0;
        label_.self_.interfaceID_ = 0;
        label_.dest_.portID_ = PEER_PORT_ID_DEFAULT;
        label_.dest_.peerID_ = 0;
        label_.dest_.interfaceID_ = 0;
    }

    Label(LabelStruct connDesc)
    {
        label_ = connDesc;
    }

    // inline EndpointDescriptionStruct & GetDescription(LabelEndpoint endpoint){return GetEndpoint(endpoint);}
    inline LabelStruct & GetLabel(){return label_;}
    inline void SetDescription(LabelStruct & labelStruct){label_ = labelStruct;}
    inline void SetDescription(EndpointDescriptionStruct & endDesc, LabelEndpoint endpoint){GetEndpoint(endpoint) = endDesc;}
    inline InterfaceID GetInterfaceID(LabelEndpoint endpoint){return GetEndpoint(endpoint).interfaceID_;}
    inline PortID GetPortID(LabelEndpoint endpoint){return GetEndpoint(endpoint).portID_;}
    inline PeerID GetPeerID(LabelEndpoint endpoint){return GetEndpoint(endpoint).peerID_;}
    inline void SetPortID(PortID portID, LabelEndpoint endpoint){GetEndpoint(endpoint).portID_ = portID;}
    inline void SetPeerID(PeerID peerID, LabelEndpoint endpoint){GetEndpoint(endpoint).peerID_ = peerID;}
    inline void SetInterfaceID(InterfaceID interfaceID, LabelEndpoint endpoint){GetEndpoint(endpoint).interfaceID_ = interfaceID;}

    // inline void 

    inline void Swap()
    {
        auto temp = label_.dest_;
        label_.dest_ = label_.self_;
        label_.self_ = temp;
    }

private:
    EndpointDescriptionStruct & GetEndpoint(LabelEndpoint endpoint)
    {
        switch(endpoint)
        {
            case LabelEndpoint::Self:
                return label_.self_;
                break;
                
            case LabelEndpoint::Dest:
                return label_.dest_;
                break;
                
            default:
                // TODO
                break;
        }

        throw; //TODO
    }

    LabelStruct label_;
};


}
