#include "RCloneVersionChecker.h"

RCloneVersionChecker::RCloneVersionChecker() {}

string RCloneVersionChecker::getRCloneVersion()
{
    // Store the output
    char buffer[128];
    string result = "";
    string version = "";

    // Open a pipe with the version command for rclone
    FILE *commandPipe;
    commandPipe = popen("/opt/bakupagent/rclone version", "r");

    // Check the pipe was successful
    if(commandPipe == nullptr)
    {
        std::cerr << "Unable to open pipe to check rClone version" << endl;
        return version;
    }

    // Read the output until EOF
    while (!feof(commandPipe))
    {
        // Use buffer to read output and add to result string
        if (fgets(buffer, 128, commandPipe) != nullptr)
            result += buffer;
    }

    // Close the pipe
    pclose(commandPipe);

    // Parse the result for the version
    if(result.substr(0, 8) == "rclone v")
    {
        // Find the position of the first newline
        size_t newlinePosition = result.find('\n', 8);

        // Start at position 7 (the 'v') and read the next n characters
        // Where n = position of newline minus the starting position of the 'v'
        version = result.substr(7, newlinePosition - 7);
    }

    // Return the version string, if empty there was an error
    return version;
}
