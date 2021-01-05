#include <main.h>

using namespace std;
using namespace rapidjson;

// The main function that handles the program loop
int main(int argc, char *argv[])
{
    // Initialise the agent class
    Agent agent;

    // Change user identity to given user ID
    int uid = stoi(agent.getUserID());
    setuid(uid);

    // Get the program loop wait time
    int waitTime = agent.getWaitTime();

    // Set the program to non-debug mode
    Debug debug(false, agent.getAgentVersion());

    // If there is a command line argument
    if(argc == 2)
    {
        // If the -d flag is passed
        if(strcmp(argv[1], "-d") == 0)
        {
            // Set the debug class to true, to log to command line
            debug.setDebugMode(true);
        }
    }
    // If there is more than one command line argument
    else if(argc > 2)
    {
        return EXIT_FAILURE;
    }

    // Main program loop
    while(true)
    {
        // Get a job from Bakup, second arg = attempt number, third arg = max attempts
        if(agent.getJob(debug, 1, 5))
        {
            // If there are jobs, process them asynchronously
            agent.processJobs(debug);
        }

        // Reset temporary variables in agent
        agent.resetJob(debug);

        // Check if the loop should wait before executing
        if(agent.skipNextPollTime)
        {
            // If so, don't wait and reset the value
            agent.skipNextPollTime = false;
        }
        else
        {
            // Otherwise, wait before asking for another job
            sleep(waitTime);
        }
    }

    return EXIT_SUCCESS;
}
