#include "Job.h"

Job::Job(Debug &debug, command_t &job, string baseUrl, string clientId, string apiToken, string agentVersion, string userID, bool autoExecute) :
        debug(ref(debug)),
        job(std::move(job)),
        baseURL(std::move(baseUrl)),
        clientId(std::move(clientId)),
        apiToken(std::move(apiToken)),
        agentVersion(std::move(agentVersion)),
        userID(std::move(userID))
{
    if(autoExecute)
    {
        // Start the job processing
        this->process();
    }
}

int Job::process(bool autoReportResults, string shell)
{
    if(job.jobType == "update" || job.jobType == "uninstall")
    {
        const char* cmd = job.commands[0].c_str();
        return this->runOrphanedCommand(cmd);
    }

    // Start a new command instance
    Command command(debug, shell);

    // Store the exit status of the overall job
    int exitStatus = 0;

    // Set up the command execution environment
    if(!command.setupEnvironment())
    {
        // Build error response and send to Bakup.io
        ResponseBuilder responseBuilder;
        responseBuilder.addSendAttempt(1);
        responseBuilder.addJobId(this->job.id);
        responseBuilder.addJobType(this->job.jobType);
        responseBuilder.addErrorCode(ERROR_CODE_JOB_FAIL);
        responseBuilder.addErrorMessage("Could not open a child process to run the job");
        this->jobOutput = responseBuilder.build();

        // Set the exit status
        exitStatus = ERROR_CODE_JOB_FAIL;

        // Report the results to Bakup
        this->reportResults(1, 5);

        // Kill the bash child
        command.killChild();

        // Wait for the child's status to change and detach from it
        int childExitStatus;
        waitpid(command.getChildPid(), &childExitStatus, 0);

        return exitStatus;
    }

    // Inject runuser commands
    string comm = "runuser --login $(cat /etc/passwd | grep 1000 | cut -d: -f1)";
    command.runCommand(comm);

    // Check the job vector isn't empty
    if(!empty(this->job.commands))
    {
        // If the desired execution time is in the future, sleep until then
        if(this->job.targetExecutionTime > time(NULL))
        {
            debug.info("Waiting " + to_string(this->job.targetExecutionTime - time(NULL)) + " seconds until desired execution time of command");
            sleep(this->job.targetExecutionTime - time(NULL));
        }

        // Create a responseBuilder object to hold the output of this job
        ResponseBuilder responseBuilder;

        // Add the send attempt element
        responseBuilder.addSendAttempt(1);

        // Add the Job ID
        responseBuilder.addJobId(this->job.id);

        // Add the job type
        responseBuilder.addJobType(this->job.jobType);

        // Hold all the command's output in this vector
        vector<commandOutput> commandsOutput;

        // Loop through all of the jobs
        for(int i = 0; i < this->job.commands.size(); i++)
        {
            // Store this commands output
            commandOutput tempCommandOutput;

            // Run the command and get the exit code and result
            pair<string, exit_status_t> result = command.runCommand(this->job.commands[i]);
            string output = result.first;
            exit_status_t commandStatusCode = result.second;

            // Store the information in the struct
            tempCommandOutput.command = this->job.commands[i];
            tempCommandOutput.statusCode = commandStatusCode;
            tempCommandOutput.result.append(output);

            commandsOutput.push_back(tempCommandOutput);

            // If the command didn't execute properly
            if(commandStatusCode != EXIT_SUCCESS)
            {
                exitStatus = 1;
                int statusCode = ERROR_CODE_JOB_FAIL;

                // Check if the exit status can be converted to a ResponseBuilder error
                switch(commandStatusCode)
                {
                    case exit_status_t::ES_READ_FAILED:
                        statusCode = ERROR_CODE_READ_PIPE_FAIL;
                        break;
                    case exit_status_t::ES_WRITE_FAILED:
                        statusCode = ERROR_CODE_WRITE_PIPE_FAIL;
                        break;
                    case exit_status_t::ES_EXIT_STATUS_NOT_FOUND:
                        statusCode = ERROR_CODE_JOB_FAIL;
                        break;
                }

                // Add the error code
                responseBuilder.addErrorCode(statusCode);

                // Add the latest job output as the error
                responseBuilder.addErrorMessage(this->job.commands[i]);
                break;
            }
        }

        // Check if the clean up commands array is empty or not
        if(!empty(this->job.cleanUpCommands))
        {
            /*
             * Loop through the clean up commands. For each command, it is executed and the output is stored
             * in the main output struct. If a command fails, it will keep running to make sure everything
             * is cleaned up.
             */
            for(int i = 0; i < this->job.cleanUpCommands.size(); i++)
            {
                // Store this clean up commands output
                commandOutput tempCleanUpCommandOutput;

                // Run the clean up command
                pair<string, exit_status_t> result = command.runCommand(this->job.cleanUpCommands[i]);
                string output = result.first;
                exit_status_t commandStatusCode = result.second;

                // Store the command output
                tempCleanUpCommandOutput.command = this->job.cleanUpCommands[i];
                tempCleanUpCommandOutput.statusCode = commandStatusCode;
                tempCleanUpCommandOutput.result.append(output);

                // Add it to the command output array and don't break on errors as all the clean up commands need to run
                commandsOutput.push_back(tempCleanUpCommandOutput);

                // If the command didn't execute properly
                if(commandStatusCode != EXIT_SUCCESS)
                {
                    /*
                     * Check to see if an error has already been set by the main set of commands as we don't want to
                     * overwrite a more important command error output. The error output will still be returned with
                     * the command status output.
                     */
                    if(exitStatus == 0)
                    {
                        exitStatus = 1;
                        int statusCode = ERROR_CODE_JOB_FAIL;

                        // Check if the exit status can be converted to a ResponseBuilder error
                        switch(commandStatusCode)
                        {
                            case exit_status_t::ES_READ_FAILED:
                                statusCode = ERROR_CODE_READ_PIPE_FAIL;
                                break;
                            case exit_status_t::ES_WRITE_FAILED:
                                statusCode = ERROR_CODE_WRITE_PIPE_FAIL;
                                break;
                            case exit_status_t::ES_EXIT_STATUS_NOT_FOUND:
                                statusCode = ERROR_CODE_JOB_FAIL;
                                break;
                        }

                        // Add the error code, checking that an error hasn't already been set
                        responseBuilder.addErrorCode(statusCode, true);

                        // Add the latest job output as the error, checking that an error hasn't already been set
                        responseBuilder.addErrorMessage(this->job.commands[i], true);
                    }
                }
            }
        }

        // If the command ran successfully
        if(exitStatus == EXIT_SUCCESS)
        {
            // Add the success error code
            responseBuilder.addErrorCode(SUCCESS_CODE);
        }

        // Add the vector of outputs
        responseBuilder.addCommandOutputs(commandsOutput);

        // Build the response and get the string
        this->jobOutput = responseBuilder.build();

        debug.info(this->jobOutput);

        // If the autoReportResults is set
        if(autoReportResults)
        {
            // Report the results to bakup
            this->reportResults(1, 5);
        }
    }

    // Kill the bash child
    command.killChild();

    // Wait for the child's status to change and detach from it
    int childExitStatus;
    waitpid(command.getChildPid(), &childExitStatus, 0);

    return exitStatus;
}

bool Job::reportResults(int retryCounter, int maxRetry)
{
    // Check if the maximum number of retries has been met
    if(retryCounter <= maxRetry)
    {
        // Build the response object to send command output back to Bakup
        Response response(this->baseURL, this->clientId, this->apiToken, this->agentVersion);

        // Execute and get the status
        int jobConfStatus = response.postJobConfirmation(this->jobOutput);
        string jobConfOutput = response.getResponse();

        // If the job failed
        if(jobConfStatus != 200)
        {
            // Handle reporting the error to Bakup
            this->handleError(jobConfOutput, response.getError());

            // Check to see if the request failed due to SSL and safely report it
            SSLChecker sslChecker(response.getErrorCode());
            if(!sslChecker.checkSSLValid())
            {
                debug.error("Sending HTTP-safe SSL error report to Bakup");

                // Build the error to send to Bakup
                ResponseBuilder responseBuilder;
                responseBuilder.addErrorCode(ERROR_CODE_SSL_FAIL);
                responseBuilder.addErrorMessage("SSL Failed with: " + response.getErrorMessage());
                string sslErrorMessage = responseBuilder.build();

                // Send to Bakup without apiToken due to plaintext protocol
                Response sslResponse(this->baseURL, this->clientId, this->apiToken, this->agentVersion);
                sslResponse.postSSLError(sslErrorMessage);
            }

            // Retry sending the result to Bakup
            this->reportResults(++retryCounter, maxRetry);
            return false;
        }
        else
        {
            debug.success("Successfully sent job confirmation");
            debug.info("Job confirmation response: " + to_string(jobConfStatus) + ": " + jobConfOutput);
        }
    }

    return true;
}

bool Job::handleError(string &httpResponse, cpr::Error error)
{
    this->debug.error("Sending job confirmation failed");

    // If the HTTP error is not empty
    if(httpResponse.length() != 0)
    {
        // Print the HTTP error response
        this->debug.error("Backup job request failed, printing error:");
        this->debug.error(httpResponse);
    }

    // If the error code is none zero
    if(static_cast<bool>(error))
    {
        // Get the libcurl error for printing
        string errorMessage = curl_easy_strerror(static_cast<CURLcode>(error.code));
        int errorCode = static_cast<CURLcode>(error.code);

        // Print the libcurl errors
        this->debug.error("libcurl error: " + to_string(errorCode) + ": " + errorMessage);
    }

    // Check if cpr error exists
    if(error.message.length() != 0)
    {
        // Print the CPR error
        this->debug.error("CPR error: " + error.message);
    }

    return true;
}

/*
 * If we want to run a job outside of the agent's current control group, we need to run an orphaned command. This is
 * useful in the cases of an update and uninstall job, where the agent's service is stopped before any actions can be
 * performed. Any commands ran using this function will persist after the agent has died.
 */
int Job::runOrphanedCommand(const char* cmd)
{
    // Store the pid of the parent/child
    int pid;

    // Store the status of the child
    int status;

    // Fork here to create a child
    pid = fork();

    // If this is the child process, run this block
    if(pid == 0)
    {
        // Replace the current thread with an image of a bash shell running the given command
        execl("/bin/bash", "bash", "-c", cmd, NULL);
    }

    // Acknowledge the child so that it can exit without becoming a zombie
    waitpid(pid, &status, 0);

    return 0;
}
