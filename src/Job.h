#ifndef BAKUP_AGENT_JOB_H
#define BAKUP_AGENT_JOB_H

#include <Request.h>
#include <Command.h>
#include <Response.h>
#include <ResponseBuilder.h>
#include <Debug.h>
#include <SSLChecker.h>
#include <future>
#include <rapidjson/document.h>
#include <rapidjson/writer.h>
#include <rapidjson/stringbuffer.h>
#include <sys/wait.h>
#include <curl/curl.h>
#ifdef TESTING
#include <gtest/gtest.h>
#endif

using namespace std;

class Job
{
    private:
        // Struct to hold the job information
        command_t job;

        // Copy of the debug class to print information
        Debug &debug;

        // Hold the job confirmation URL
        string baseURL;

        // Hold the client Id
        string clientId;

        // Hold the authentication token
        string apiToken;

        // Hold the version of the agent
        string agentVersion;

        // Store the user's ID
        string userID;

#ifdef TESTING
        // Friend class for testing FailProcessFailCleanUpProcessTest
        FRIEND_TEST(JobTest, FailProcessFailCleanUpProcessTest);
#endif

        protected:
            // Store the job's output
            string jobOutput;
public:
        // Constructor that must be given a reference to debug object and the job to be done
        Job(Debug &debug, command_t &job, string baseUrl, string clientId, string apiToken, string agentVersion, string userID, bool autoExecute = true);

        // Process the commands in the job
        int process(bool autoReportResults = true, string shell = "/bin/bash");

        // Report the results to Bakup
        bool reportResults(int retryCounter, int maxRetry);

        // Handle printing errors
        bool handleError(string &httpResponse, cpr::Error error);

        // Run a command that will be orphaned, this is good for running scripts that will effect the agent
        int runOrphanedCommand(const char* cmd);
};


#endif //BAKUP_AGENT_JOB_H
