#include <gtest/gtest.h>
#define TESTING
#include <Job.h>
#include <Agent.h>
#include <Debug.h>
#include <Request.h>
#include <ctime>

class JobTest : public ::testing::Test
{
    protected:
        JobTest()
        {
        }

        ~JobTest()
        {
        }

    public:
        Agent agent;
};

TEST_F(JobTest, ProcessTest)
{
    // Create the command struct
    command_t job;
    job.id = "1";
    job.targetExecutionTime = time(NULL);
    job.commands.emplace_back("ls");
    job.cleanUpCommands.emplace_back("ls");

    // Construct the debug library for output
    Debug debug(true, agent.getAgentVersion());

    // Start the job process
    Job jobObj(debug, job, agent.getBaseURL(), agent.getClientId(), agent.getApiToken(), agent.getAgentVersion(), false);
    ASSERT_EQ(jobObj.process(false), EXIT_SUCCESS);
}

TEST_F(JobTest, FailProcessTest)
{
    // Create the command struct with an invalid command
    command_t job;
    job.id = "1";
    job.targetExecutionTime = time(NULL);
    job.commands.emplace_back("notAValidCommand");
    job.cleanUpCommands.emplace_back("ls");

    // Construct the debug library for output
    Debug debug(true, agent.getAgentVersion());

    // Start the job process
    Job jobObj(debug, job, agent.getBaseURL(), agent.getClientId(), agent.getApiToken(), agent.getAgentVersion(), false);
    ASSERT_GT(jobObj.process(false), EXIT_SUCCESS);
}

TEST_F(JobTest, SuccessfulProcessFailCleanUpProcessTest)
{
    // Create the command struct with an invalid command
    command_t job;
    job.id = "1";
    job.targetExecutionTime = time(NULL);
    job.commands.emplace_back("ls");
    job.cleanUpCommands.emplace_back("notAValidCommand");

    // Construct the debug library for output
    Debug debug(true, agent.getAgentVersion());

    // Start the job process
    Job jobObj(debug, job, agent.getBaseURL(), agent.getClientId(), agent.getApiToken(), agent.getAgentVersion(), false);
    ASSERT_GT(jobObj.process(false), EXIT_SUCCESS);
}

TEST_F(JobTest, FailProcessFailCleanUpProcessTest)
{
    // Create the command struct with an invalid command
    command_t job;
    job.id = "1";
    job.targetExecutionTime = time(NULL);
    job.commands.emplace_back("notAValidCommand");
    job.cleanUpCommands.emplace_back("alsoNotAValidCommand");

    // Construct the debug library for output
    Debug debug(true, agent.getAgentVersion());

    // Start the job process
    Job jobObj(debug, job, agent.getBaseURL(), agent.getClientId(), agent.getApiToken(), agent.getAgentVersion(), false);
    jobObj.process(false);

    // Check that the error message is equal to that of the main command error and not the cleanup command error
    ASSERT_NE(jobObj.jobOutput.find("\"error_message\":\"notAValidCommand\""), std::string::npos);
}

TEST_F(JobTest, ReportResults)
{
    // Create the command struct
    command_t job;
    job.id = "1";
    job.targetExecutionTime = time(NULL);
    job.commands.emplace_back("ls");

    // Construct the debug library for output
    Debug debug(true, agent.getAgentVersion());

    // Start the job process
    Job jobObj(debug, job, agent.getBakupJobConfirmationURL(), agent.getAuthToken(), false);
    jobObj.process(false);
    ASSERT_EQ(jobObj.reportResults(1, 1), true);
}

TEST_F(JobTest, HandleErrors)
{
    // Create the command struct
    command_t job;
    job.id = "1";
    job.targetExecutionTime = time(NULL);
    job.commands.emplace_back("ls");

    // Construct the debug library for output
    Debug debug(true, agent.getAgentVersion());

    // Start the job process
    Job jobObj(debug, job, agent.getBaseURL(), agent.getClientId(), agent.getApiToken(), agent.getAgentVersion(), false);
    cpr::Error error = cpr::Error();
    string httpError = "HTTP ERROR";
    ASSERT_TRUE(jobObj.handleError(httpError, error));
}