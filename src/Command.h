#ifndef BAKUP_AGENT_COMMAND_H
#define BAKUP_AGENT_COMMAND_H

#include <Debug.h>
#include <ResponseBuilder.h>

#include <string>
#include <array>
#include <unistd.h>
#include <cstdio>
#include <cstdlib>
#include <utility>
#include <algorithm>
#include <iostream>
#include <random>
#include <sstream>
#include <sys/types.h>
#include <fcntl.h>
#include <csignal>
#include <wait.h>

using namespace std;

// Stores the exit code from a job or these error codes
enum exit_status_t {
    ES_WRITE_FAILED = 256,
    ES_READ_FAILED,
    ES_EXIT_STATUS_NOT_FOUND
};

class Command
{
    private:
        // Debug class for output
        Debug debug;

        // Store file descriptors that will be used throughout the jobs life
        int inPipeFD[2];
        int outPipeFD[2];

        // For convenience of reading which are the parent pipes for writing and reading
        int out;
        int in;

        // The process id of the child to be opened
        pid_t pid = -1;

        // Store executable to run for bash shell, only for testing
        string shell = "/bin/bash";

        // Store the user ID
        string userID;

    public:
        // Constructor
        Command(Debug &debug, string shell = "/bin/bash");

        // Destructor
        ~Command();

        // Generate delimiter strings to append to command to know when they're finished
        string generateDelimiter();

        // Setup the child process and pipes
        bool setupEnvironment(string bashTestCommand = "echo");

        // Send a command to the bash process and return the output and an exit status
        std::pair<string, exit_status_t> runCommand(string command);

        // Setter for shell variable
        bool setShell(string &shell);

        // Getter for the child pid
        pid_t getChildPid();

        // Run SIGKILL on child process
        void killChild();
};

#endif //BAKUP_AGENT_COMMAND_H
