#include "../src/Engine.h"
#include <gtest/gtest.h>

TEST(GreeterTest, GreetReturnsCorrectMessage) {
    Engine engine;
    EXPECT_EQ(engine.getStatus(), "Engine is ready!");
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}