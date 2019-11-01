#include "../IPTestAdministrator.h"

#include <gtest/gtest.h>
#include <core/core.h>

using namespace WPEFramework;
using namespace WPEFramework::Core;

TEST(test_messageException, simple_messageException)
{
    std::string msg = "Testing the message exception.";
    MessageException exception(msg.c_str(),false);
    EXPECT_STREQ(exception.Message(),msg.c_str());
    
    MessageException exception1(msg.c_str(),true);
    char buffer[50];
    sprintf(buffer,"%s: File exists",msg.c_str());
    EXPECT_STREQ(exception1.Message(),buffer);
}