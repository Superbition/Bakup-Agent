#include <gtest/gtest.h>
#include <Agent.h>
#include <Response.h>

class ResponseTest : public ::testing::Test
{
    protected:
        ResponseTest()
        {

        }

    public:
        Agent agent;
};

TEST_F(ResponseTest, PostJobConfirmation)
{
    string jobStatus = "[{\"test\": true}]";
    Response response(this->agent.getBakupJobConfirmationURL(), this->agent.getAuthToken());
    int statusCode = response.postJobConfirmation(jobStatus);
    ASSERT_GE(statusCode, 200);
    ASSERT_LT(statusCode, 500);
}