#pragma once


#include <iostream>
#include <DPNLogger.hpp>
#include <Hub.hpp>
#include <zmq.hpp>
#include <optional>

namespace dpn
{


#define PEER_PORT_ID_DEFAULT_VALUE 0
class Label
{

public:

    using PeerID = uint16_t;
    using PortID = Hub::PortID;
    using InterfaceID = uint16_t;
    using MessageID = uint32_t;


    const static uint16_t PEER_PORT_ID_DEFAULT;

    /**
     * @brief Interface Type
     * 
     * @details The Interface Type enumerates the 
     * different types of interfaces that exist.
     * Interface type is passed with each message
     * so that the message can be processed by the 
     * correct interface when received.
     * 
     */
    enum class InterfaceType :uint16_t
    {
        PushPull,
        Request,
        PubSub
    };



    /**
     * @brief Interface Description
     * 
     * @details The Interface Description struct is used to 
     * uniqely identify/describe an interface. Only one interface
     * can exist per unique combination of peerID, portID, and 
     * interfaceID.  
     * 
     */
    struct InterfaceDescription
    {
        PeerID peerID_;
        PortID portID_;
        InterfaceID interfaceID_;
    };

    /**
     * @brief Interface Pair
     * 
     * @details The Interface Pair is used to store information
     * about two interface descriptions as a pair. This is useful
     * data to include in messages that get passed around between
     * peers, as it provides information about where the message
     * came from and where it's going. 
     * 
     */


    /**
     * @brief Contents
     * 
     * @details 
     * 
     */
    // TODO Proper serailization needs to take place to ensure this struct
    // is able to written/read from a buffer
    struct Contents
    {
        MessageID messageID_;
        InterfaceType type_;        
        InterfaceDescription src_;
        InterfaceDescription dest_;
    };


    enum class LabelEndpoint : uint32_t
    {
        Src,
        Dest
    };


    Label()
    {
        reset();
    }

    Label(Contents& content):contents_(content)
    {
        reset();
    }


    inline MessageID GetMessageID(){return contents_.messageID_;}
    inline InterfaceType GetInterfaceType(){return contents_.type_;}
    inline InterfaceDescription & GetSrc(){return contents_.src_;}
    inline InterfaceDescription & GetDest(){return contents_.dest_;}
    inline Contents & GetContents(){return contents_;}
    

    inline void SetDescription(InterfaceDescription & endDesc, LabelEndpoint endpoint){GetEndpoint(endpoint) = endDesc;}
    inline InterfaceID GetInterfaceID(LabelEndpoint endpoint){return GetEndpoint(endpoint).interfaceID_;}
    inline PortID GetPortID(LabelEndpoint endpoint){return GetEndpoint(endpoint).portID_;}
    inline PeerID GetPeerID(LabelEndpoint endpoint){return GetEndpoint(endpoint).peerID_;}
    inline void SetPortID(PortID portID, LabelEndpoint endpoint){GetEndpoint(endpoint).portID_ = portID;}
    inline void SetPeerID(PeerID peerID, LabelEndpoint endpoint){GetEndpoint(endpoint).peerID_ = peerID;}
    inline void SetInterfaceID(InterfaceID interfaceID, LabelEndpoint endpoint){GetEndpoint(endpoint).interfaceID_ = interfaceID;}

    inline void Swap()
    {
        auto temp = contents_.dest_;
        contents_.dest_ = contents_.src_;
        contents_.src_ = temp;
    }
protected:
    void reset()
    {
        contents_.src_.portID_ = PEER_PORT_ID_DEFAULT;
        contents_.src_.peerID_ = 0;
        contents_.src_.interfaceID_ = 0;
        contents_.dest_.portID_ = PEER_PORT_ID_DEFAULT;
        contents_.dest_.peerID_ = 0;
        contents_.dest_.interfaceID_ = 0;
    }
private:


    InterfaceDescription & GetEndpoint(LabelEndpoint endpoint)
    {
        switch(endpoint)
        {
            case LabelEndpoint::Src:
                return contents_.src_;
                break;
                
            case LabelEndpoint::Dest:
                return contents_.dest_;
                break;
                
            default:
                // TODO
                break;
        }

        throw; //TODO
    }

    Contents contents_;

};


}
