#include "Job.h"

Job::Job(Debug &debug, command_t &job, string jobConfirmationURL, string clientId, string authToken, bool autoExecute) :
        debug(ref(debug)),
        job(std::move(job)),
        jobConfirmationURL(std::move(jobConfirmationURL)),
        authToken(std::move(authToken)),
        clientId(std::move(clientId))
{
    if(autoExecute)
    {
        // Start the job processing
        this->process();
    }
}

int Job::process(bool autoReportResults)
{
    // Store the exit status of the overall job
    int exitStatus = 0;

    // Check the job vector isn't empty
    if(!empty(this->job.commands))
    {
        // If the desired execution time is in the future, sleep until then
        // Bakup should return jobs chronologically, so jobs won't execute late
        if(this->job.targetExecutionTime > time(NULL))
        {
            debug.info("Waiting " + to_string(this->job.targetExecutionTime - time(NULL)) + " seconds until desired execution time of command");
            sleep(this->job.targetExecutionTime - time(NULL));
        }

        // Create a string buffer and writer for creating a JSON string
        StringBuffer s;
        Writer<StringBuffer> writer(s);
        writer.StartObject();
        writer.Key("send_attempt");
        writer.Int(1);
        writer.Key("command_output");
        writer.StartArray();

        for(int i = 0; i < this->job.commands.size(); i++)
        {
            // Start a new object within the outer JSON object
            writer.StartObject();

            // Set up the command and working directory
            Command command(this->job.commands[i]);
            // Run the command and get the exit code
            int commandStatusCode = command.process();
            // Get the output of the command
            string result = command.getOutput();

            // Write the output and status of the command to the JSON object
            writer.Key("command");
            writer.String(this->job.commands[i].c_str());
            writer.Key("status_code");
            writer.Int(commandStatusCode);
            writer.Key("result");
            writer.String(result.c_str());
            writer.EndObject();

            // If the command didn't execute properly
            if(commandStatusCode != EXIT_SUCCESS)
            {
                exitStatus = 1;
                break;
            }
        }

        // End the JSON string
        writer.EndArray();
        writer.EndObject();

        // Convert the JSON object to a string
        this->jobOutput = s.GetString();

        // If the autoReportResults is set
        if(autoReportResults)
        {
            // Report the results to bakup
            this->reportResults(1, 5);
        }
    }

    return exitStatus;
}

bool Job::reportResults(int retryCounter, int maxRetry)
{
    // Check if the maximum number of retries has been met
    if(retryCounter <= maxRetry)
    {
        // Build the response object to send command output back to Bakup
        Response response(this->jobConfirmationURL, this->clientId, this->authToken);

        // Execute and get the status
        int jobConfStatus = response.postJobConfirmation(this->jobOutput);
        string jobConfOutput = response.getResponse();

        // If the job failed
        if(jobConfStatus != 200)
        {
            // Handle reporting the error to Bakup
            this->handleError(jobConfOutput, response.getError());
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
