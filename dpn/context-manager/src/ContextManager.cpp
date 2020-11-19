#include <ContextManager.hpp>
#include <Error.hpp>

#include <DPNLogger.hpp>


namespace dpn
{

zmq::context_t & ContextManager::AddContext(ContextName name, int io_threads, int max_sockets)
{
    // TODO Consider using smart pointers here
    zmq::context_t * context;

    // Create a new context
    try
    {
        context = new zmq::context_t(io_threads,max_sockets);
    }
    catch (std::exception& e)
    {
        DPN_SYS_WARN(
            "Context::{} failed to create with exception '{}' - io_threads = {} max_sockets = {}", 
            name, e.what(), io_threads, max_sockets);
        throw e;
    }

    // Check if context name is already in use
    if (contextMap_.find(name) != contextMap_.end())
    {
        DPN_SYS_WARN(
            "Context::{} context name is already in use - io_threads = {} max_sockets = {}", 
            name, io_threads, max_sockets);
        delete(context);
        throw std::invalid_argument("Invalid context name");    
    }

    // Insert the newly created context into a map for later reference
    auto rv = contextMap_.insert(std::make_pair(name, context));
    if (!rv.second)
    {
        DPN_SYS_WARN(
            "Context::{} failed to insert into context map - io_threads = {} max_sockets = {}", 
            name, io_threads, max_sockets);
        delete(context);
        throw std::runtime_error("Failed to insert into unordered_map");    
    }

    DPN_SYS_DEBUG(
        "Context::{} created - io_threads = {} max_sockets = {}", 
        name, io_threads, max_sockets);

    return *context;
}


zmq::context_t & ContextManager::GetContext(ContextName name)
{
    auto iter = contextMap_.find(name);
    if (iter == contextMap_.end())
    {
        DPN_SYS_WARN(
            "Context::{} could not be found", 
            name);
        throw std::invalid_argument("Invalid context name");
    }

    return *iter->second;
}

}