#include <gtest/gtest.h>
#include <Agent.h>
#include <Request.h>

class RequestTest : public ::testing::Test
{
    protected:
        RequestTest()
        {

        }

    public:
        Agent agent;
        string response;
};

TEST_F(RequestTest, MakeWebRequest)
{
    Debug debug(true, this->agent.getAgentVersion());
    Request request(this->agent.getBaseURL(), this->agent.getClientId(), this->agent.getApiToken(), this->agent.getAgentVersion(), debug);
    int statusCode = request.getBakupJob();
    ASSERT_GE(statusCode, 200);
    ASSERT_LT(statusCode, 500);
}