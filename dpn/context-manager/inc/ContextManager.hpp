#pragma once

#include <zmq.hpp>
#include <unordered_map>
#include <iostream>

namespace dpn
    
{
class ContextManager
{
public:
    using ContextName = std::string;
    zmq::context_t & AddContext(ContextName name, int io_threads_, int max_sockets_ = ZMQ_MAX_SOCKETS_DFLT);
    zmq::context_t & GetContext(ContextName name);
private:
    std::unordered_map<ContextName,zmq::context_t*> contextMap_;
};

}