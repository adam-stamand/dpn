
<br />
<p align="center">
  <h3 align="center">Distributed Peer Network</h3>
  <h4 align="center">(DPN)</h4>
</p>


<!-- TABLE OF CONTENTS -->
## Table of Contents

* [About the Project](#about-the-project)
  * [Built With](#built-with)
* [Getting Started](#getting-started)
  * [Prerequisites](#prerequisites)
  * [Building/Running](#building/running)
* [Documentation](#documentation)
    * [Interfaces](#interfaces)
    * [Port](#port)
    * [Peer](#peer)
    * [Hub](#hub)
    * [Messages](#messages)
    * [Logging](#logging)
    * [Context Manager](#context-manager)
* [Roadmap](#roadmap)



<!-- ABOUT THE PROJECT -->
## About The Project

The Distributed Peer Network (DPN) is a Message-Oriented Middleware [MOM](https://en.wikipedia.org/wiki/Message-oriented_middleware) built on top of [ZMQ](https://zeromq.org/).

In short, DPN enables users to get aligned message buffers from one peer to another with a flexible, extensible and easy-to-use API. The peers may be different threads within a process, different processes within a processor, or 
even different processors within a distributed system.

The message buffers can be exchanged utilizing any of the following patterns:
* Publish/Subscribe
* Push/Pull
* Request/Reply




### Built With
Below are frameworks/tools that DPN is built using:
* [libzmq](https://github.com/zeromq/libzmq)
* [cppzmq](https://github.com/zeromq/cppzmq)
* [spdlog](https://github.com/gabime/spdlog)
* [GoogleTest](https://github.com/google/googletest)



<!-- GETTING STARTED -->
## Getting Started

### Prerequisites

Below is software necessary to build and run DPN:
* [Python3](https://www.python.org/downloads/)
* [Meson](https://mesonbuild.com/Quick-guide.html)


### Building/Running

1. Configure project
```sh
make config.linux
```
2. Build project
```sh
make build.linux
```
3. Run tests
```sh
make test.linux
```



<!-- USAGE EXAMPLES -->
## Documentation


### Port

Ports are the fundamental entity used for exchanging messages. Each port has a PortName, a globally unique string used to identify the port.

### Hub

Hubs just contain a number of ports. They provide an interface to manage/access multiple ports.

### Peer

A peer is the main object of DPN. Each peer contains a Hub, which gives the peer access to any of the hub's ports. 

Peers communicate to each other through an Interface mechanism. Each peer is capable of calling another peer's interfaces. Each peer is also capable of defining their own interfaces.
Calls and Interfaces each have types. These types map one-to-one in the following way:
* Push Calls -> Pull Interfaces
* Request Calls -> Reply Interfaces
* Pubish Calls -> Subscription Interfaces
* Subscribe Calls -> Topic Interfaces

For example, let's say Peer 0 wants to subscribe to a topic on Peer 1. The following would acheive this effect:
1. Peer 1 implements a Topic Interface
2. Peer 0 implements a Subscription Interface
3. Peer 0 makes a Subscribe Call to Peer 1 Topic Interface
4. Peer 1 makes a Publish Call (using the topic ID from its Topic Interface)
5. Peer 0 receives a message on its Subscription Interface


### Interfaces

An interface is callable by a remote peer. There are four types of interfaces (though users can create their own as well):
* Subscription
* Pull
* Reply
* Topic



### Messages

Messages provide an aligned buffer (statically allocated at runtime). This allows users to cast the buffer as arbitrary types. The resize() method should be used to set the size of the message after
data is written to the buffer.

Messages are managed by the user. Along with an offset option, this allows users to easily achieve zero-copy messaging.

### Context Manager

ZMQ relies on a concept of contexts. The Context Manager provides an easy way to create, name, and get existing contexts.

### Logging

spdlog is used as the logging system. Each Peer has its own logger, allowing fine control of log levels/filtering. The underlying DPN code uses the "SYS" logger, allowing only coarse control of logging for system logs.

<!-- ROADMAP -->
## Roadmap

See the [open issues](https://gitlab.com/loft-orbital/products/payload-hub/picu/flight-sw/frameworks/dpn/-/issues) for a list of proposed features (and known issues).
