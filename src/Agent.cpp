#include <Agent.h>

Agent::Agent()
{
    this->apiToken = this->readFile(this->apiTokenLocation);
    this->userID = this->readFile(this->userIDLocation);
};

Agent::Agent(const Agent &obj)
{
    // Set the temp values in the new agent object
    this->clientId = obj.clientId;
    this->apiToken = obj.apiToken;
    this->userID = obj.userID;
    this->jobs = obj.jobs;
}

std::string Agent::readFile(const std::string &fileLocation)
{
    // Open the file stream using the given file location
    std::ifstream fileStream(fileLocation);

    if(fileStream)
    {
        // The place holder string to hold the contents of the file
        std::string fileText;

        // Seek to the end of the file
        fileStream.seekg(0, std::ios::end);

        // Reserve memory for the string using the new position in the file stream
        fileText.reserve(fileStream.tellg());

        // Move the position back to the start of the file
        fileStream.seekg(0, std::ios::beg);

        // Assign the file text using the iterator from the file stream
        fileText.assign((std::istreambuf_iterator<char>(fileStream)), std::istreambuf_iterator<char>());

        // Remove trailing new line
        if (!fileText.empty() && fileText[fileText.length()-1] == '\n')
        {
            fileText.erase(fileText.length()-1);
        }

        return fileText;
    }
    else
    {
        cerr << "Agent could not access file: " << fileLocation << endl;
        cerr << "Error reason: " << strerror(errno) << endl;
        cerr << "Try running the install script again to generate required files. If this does not work, please contact support" << endl;
        std::exit(1);
    }
}

string Agent::getBaseURL()
{
    return this->host + this->baseUrl + this->apiVersionBaseUrl + this->apiVersion;
}

string Agent::getClientId()
{
    return this->clientId;
}

string Agent::getApiToken()
{
    return this->apiToken;
}

string Agent::getUserID()
{
    return this->userID;
}

string Agent::getAgentVersion()
{
    return this->agentVersion;
}

int Agent::getWaitTime()
{
    return this->pollTime;
}

int Agent::getRetryTime() {
    return this->retryTime;
}

bool Agent::getJob(Debug &debug, int retryCounter, int retryMaxCount)
{
    // Get a job from Bakup
    Request job(this->getBaseURL(), this->getClientId(), this->getApiToken(), this->getAgentVersion(), debug);
    int jobStatusCode = job.getBakupJob();

    // Check if the JSON was valid
    if(job.isJsonValid())
    {
        // Check if the request was successful
        if(jobStatusCode == 200)
        {
            debug.success("Successful bakup job request");

            // Parse the response from Bakup to get the job list
            this->jobs = job.getVectoredResponse();

            // If debug mode is enabled
            if(!jobs.empty())
            {
                if(debug.getDebugMode())
                {
                    // Print received jobs
                    debug.info("Commands received:");
                    for(const command_t &jobStruct: jobs)
                    {
                        // Print the execution time of the job
                        debug.info("Job to execute at " + to_string(jobStruct.targetExecutionTime));

                        // Store the commands in a string for printing
                        string toPrint;
                        for(const string &command: jobStruct.commands)
                        {
                            toPrint.append(command + ", ");
                        }

                        // Store the clean up commands in the toPrint string for printing
                        for(const string &command: jobStruct.cleanUpCommands)
                        {
                            toPrint.append(command + ", ");
                        }

                        // Print out command string without last comma
                        debug.info(toPrint.substr(0, toPrint.size()-2));
                    }
                }

                return true;
            }
            else // No jobs were found
            {
                debug.info("No commands were found in the job");
                return false;
            }
        }
        else // If it was not successful
        {
            // Hand each error output to the handle error function
            this->handleError(debug, job.getResponse(), job.getError());

            // Check to see if the request failed due to SSL and safely report it
            SSLChecker sslChecker(job.getErrorCode());
            if(!sslChecker.checkSSLValid())
            {
                debug.error("Sending HTTP-safe SSL error report to Bakup");

                // Build the error to send to Bakup
                ResponseBuilder responseBuilder;
                responseBuilder.addErrorCode(ERROR_CODE_SSL_FAIL);
                responseBuilder.addErrorMessage("SSL Failed with: " + job.getErrorMessage());
                string sslErrorMessage = responseBuilder.build();

                // Send to Bakup without apiToken due to plaintext protocol
                Response response(this->getBaseURL(), this->clientId, this->apiToken, this->getAgentVersion());
                response.postSSLError(sslErrorMessage);
            }

            // If the maximum amount of retries has not been reached, try requesting job again
            if(retryCounter <= retryMaxCount)
            {
                // Print the error
                debug.error(
                        "Job request failed, will try again in " + to_string(this->getRetryTime()) + " seconds (Attempt " +
                        to_string(retryCounter) + " out of " + to_string(retryMaxCount) + ")");

                // Sleep until the job should be re-requested
                sleep(this->getRetryTime());

                // Try requesting the job again
                return this->getJob(debug, ++retryCounter, retryMaxCount);
            }
            else
            {
                return false;
            }
        }
    }
    else
    {
        // Build the response to send to Bakup
        ResponseBuilder responseBuilder;
        responseBuilder.addErrorCode(ERROR_CODE_JSON_FAIL);
        responseBuilder.addErrorMessage(job.getJson());
        string errorResponse = responseBuilder.build();

        // Send the built JSON response to bakup
        Response response(this->getBaseURL(), this->clientId, this->apiToken, this->getAgentVersion());
        response.postJobError(errorResponse);

        return false;
    }
}

bool Agent::handleError(Debug &debug, string httpResponse, cpr::Error error)
{
    debug.error("Requesting job failed");

    // If the HTTP error is not empty
    if(httpResponse.length() != 0)
    {
        // Print the HTTP error response
        debug.error("Backup job request failed, printing error:");
        debug.error(httpResponse);
    }

    // If the error code is none zero
    if(static_cast<bool>(error))
    {
        // Get the libcurl error for printing
        string errorMessage = curl_easy_strerror(static_cast<CURLcode>(error.code));
        int errorCode = static_cast<CURLcode>(error.code);

        // Print the libcurl errors
        debug.error("libcurl error: " + to_string(errorCode) + ": " + errorMessage);
    }

    // Check if cpr error exists
    if(error.message.length() != 0)
    {
        // Print the CPR error
        debug.error("CPR error: " + error.message);
    }

    return true;
}

int Agent::getNumberOfJobs() {
    return this->jobs.size();
}

bool Agent::processJobs(Debug &debug)
{
    // If the received job is a agent credential change, run synchronously
    if(this->jobs[0].refreshAgentCredentials)
    {
        Job newJob(debug, this->jobs[0], this->getBaseURL(), this->getClientId(), this->getApiToken(), this->getAgentVersion(), this->getUserID());
        this->refreshAgentCredentials(debug);
        this->skipNextPollTime = true;
    }
    else
    {
        // For each job in the jobs vector
        for(command_t job: this->jobs)
        {
            // Check if the job requires a privileged user
            if(job.jobType == "update" || job.jobType == "uninstall")
            {
                if(!changeEUID(0, job))
                {
                    // If the agent cannot switch back to root, exit
                    return false;
                }
            }

            // Create a new thread with the job class constructor, passing in the job
            thread newJob([](Debug &debug, command_t &&job, string &&jobConfirmationURL, string &&clientId, string &&apiToken, string &&agentVersion, string &&userID)
                          {
                              Job newJob(debug, job, jobConfirmationURL, clientId, apiToken, agentVersion, userID);
                          },
                          ref(debug), job, this->getBaseURL(), this->getClientId(), this->getApiToken(), this->getAgentVersion(), this->getUserID());

            // Detach from the thread so that the main thread can continue running
            newJob.detach();

            // Check if the job requires a privilege deescalation
            if(job.jobType == "update" || job.jobType == "uninstall")
            {
                if(!changeEUID(stoi(this->getUserID()), job))
                {
                    // If the agent cannot switch back to the desired user, exit
                    return false;
                }
            }
        }
    }

    return true;
}

void Agent::refreshAgentCredentials(Debug &debug)
{
    this->clientId = this->readFile(this->clientIdLocation);
    this->apiToken = this->readFile(this->apiTokenLocation);
    debug.success("Agent credentials successfully updated");
}

bool Agent::checkFirstRunAndPing(Debug &debug)
{
    // Check if the file can be read, if a null pointer is returned; the file does not exist
    ifstream initFile;
    initFile.open(this->initialisedLocation);
    if(initFile)
    {
        // The file already exists, this isn't the first setup
        initFile.close();

        return true;
    }
    else
    {
        // Close the read file pointer
        initFile.close();

        // Setup the response object
        Response response(this->getBaseURL(), this->getClientId(), this->getApiToken(), this->getAgentVersion());

        // Get OS Information
        string osInformation = this->readFile(this->osReleaseFile);

        // If that didn't work, use the uname library to get basic os information
        if(osInformation.empty())
        {
            utsname unameData{};

            uname(&unameData);

            osInformation = unameData.sysname;
        }

        // If we couldn't find OS information, exit
        if(!osInformation.empty())
        {
            // If the request was successful
            if(response.postInitialisationPing(osInformation) == 200)
            {
                // Create the initialisation file locally
                ofstream createInitFile(this->initialisedLocation);
                createInitFile << "DO NOT DELETE - this is used to indicate that your agent has been registered" << endl;
                createInitFile.close();

                // Log and return true
                debug.success("First startup - sent initialisation ping and created initialisation file");
                return true;
            }
        }

        // Log and return false
        debug.error("First startup - attempted to send initialisation ping, but it failed, please rerun the agent");
        return false;
    }
}

bool Agent::changeEUID(int uid, command_t &job)
{
    int result = seteuid(uid);
    if(result < 0)
    {
        // Get the exit status
        int exitStatus = errno;

        // Build error response and send to Bakup.io
        ResponseBuilder responseBuilder;
        responseBuilder.addSendAttempt(1);
        responseBuilder.addJobId(job.id);
        responseBuilder.addJobType(job.jobType);
        responseBuilder.addErrorCode(exitStatus);
        responseBuilder.addErrorMessage(strerror(exitStatus));
        string jobOutput = responseBuilder.build();

        Response response(this->getBaseURL(), this->getClientId(), this->getApiToken(), this->getAgentVersion());

        response.postJobConfirmation(jobOutput);

        return false;
    }

    return true;
}
