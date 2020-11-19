
#include <gtest/gtest.h>
#include <zmq.hpp>
#include <Runner.hpp>
#include <Entity.hpp>
#include <GenericPackager.hpp>

using namespace molle;

struct OmniPrintPayload 
{
    unsigned int dummy;
} __attribute__ ((packed));
class OmniPrint : public Package<OmniPrintPayload>
{
public:
    OmniPrint(Deployment::EntityID src, OmniPrintPayload & payload, size_t argLen):
    Package(src, Deployment::EntityID::Omni,PackageHeader::PackageType::Request, Deployment::Service::Service1, &payload, sizeof(payload), argLen){}

    OmniPrint() = default;
};

class TestEntity : public Entity
{
public:
    TestEntity(Deployment::EntityID localEntityID, Deployment::EntityID remoteEntityID, EntityPortsInit portInit): 
    Entity(localEntityID, portInit), remoteEntityID_(remoteEntityID),counter(0){}
    
    void EntityRun()
    {
        Deployment::EntityID remoteEntityID;
        
        unsigned int temp = 0x12341234;
        OmniPrintPayload payload = {.dummy = 0x12345678};
        
        OmniPrint pkg(static_cast<Deployment::EntityID>(0), payload, sizeof(temp));
        
        auto args = pkg.GetPayload();
        memcpy(args->data,&temp, sizeof(temp));

        SendPackage(remoteEntityID_, pkg);
        OS_TaskDelay(500);

        Terminate();
    }

private:
    Deployment::EntityID remoteEntityID_;
    int counter;

};


TEST(EntityTest, TwoEntity)
{
    Runner runner;
    runner.AddContext("inproc", 0);
    zmq::context_t &context = runner.GetContext("inproc");
    
    TestEntity entity0(static_cast<Deployment::EntityID>(0),static_cast<Deployment::EntityID>(1),
    {
        {.transport=Transport::inproc,.remoteComponents={static_cast<Deployment::EntityID>(1)}},
    });


    TestEntity entity1(static_cast<Deployment::EntityID>(1),static_cast<Deployment::EntityID>(0),
    {
        {.transport=Transport::inproc,.remoteComponents={static_cast<Deployment::EntityID>(0)}},
    });

    entity0.Start(context);
    entity1.Start(context);

    OmniPrint pkg;
    Deployment::EntityID remoteEntityID_;
    entity0.ReceivePackage(remoteEntityID_, pkg);
    auto args = pkg.GetPayload();
    std::cout << remoteEntityID_ << " - " << pkg << args->dummy << "\n";

}




 
int main(int argc, char **argv) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
