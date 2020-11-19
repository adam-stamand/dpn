
#include <gtest/gtest.h>
#include <functional>
#include <vector>
#include <ContextManager.hpp>
#include <DPNLogger.hpp>

using namespace dpn;


static void CreateContexts(ContextManager & manager, int first, int last)
{
    for (int i = first; i < last; i++)
    {
        EXPECT_NO_THROW(manager.AddContext(std::to_string(i), i, i * 2));
    }
}

class TestEnvironment : public ::testing::Environment {
 public:
//   virtual void SetUp() {Logger::InitLogger(); }
};

TEST(ContextManagerTest, CreateSingleContext) 
{
    ContextManager manager;
    EXPECT_NO_THROW(manager.AddContext("", 2));
    EXPECT_NO_THROW(manager.AddContext("a", 0));
    EXPECT_NO_THROW(manager.AddContext("b", 3));
    ASSERT_DEATH(manager.AddContext("c", -1), "rc == 0");
    EXPECT_NO_THROW(manager.AddContext("c", 0, 5));
    ASSERT_DEATH(manager.AddContext("d", 0, -3), "rc == 0");
    ASSERT_DEATH(manager.AddContext("d", 0, 0), "rc == 0");
    EXPECT_NO_THROW(manager.AddContext("d", 10, 29));
    EXPECT_THROW(manager.AddContext("a", 0), std::invalid_argument);
}

TEST(ContextManagerTest, CreateMultipleContexts) 
{
    ContextManager manager;
    
    CreateContexts(manager, 1, 1000);
}
 
TEST(ContextManagerTest, GetSingleContext) 
{
    ContextManager manager;

    CreateContexts(manager, 1, 5);

    EXPECT_NO_THROW(manager.GetContext("1"));
    EXPECT_NO_THROW(manager.GetContext("2"));
    EXPECT_NO_THROW(manager.GetContext("3"));
    EXPECT_NO_THROW(manager.GetContext("4"));
    EXPECT_THROW(manager.GetContext("5"), std::invalid_argument);
    EXPECT_THROW(manager.GetContext("0"), std::invalid_argument);
}

 
int main(int argc, char **argv) {
    testing::InitGoogleTest(&argc, argv);
    testing::AddGlobalTestEnvironment(new TestEnvironment);
    return RUN_ALL_TESTS();
}
