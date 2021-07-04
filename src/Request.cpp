#include <Request.h>

int Request::getBakupJob()
{
    // No parameters are required for this request, so create a blank variable
    cpr::Parameters parameters = cpr::Parameters{};

    // Add the authorisation token to the headers
    cpr::Header headers = this->getDefaultHeaders();

    // Variable to store content inside
    string http_content;

    // Build the URL to get a job
    string bakupJobUrl = this->secureProtocol + this->baseUrl + this->bakupRequestUrl;

    // Make the request to bakup
    int responseCode = this->apiGetRequest(bakupJobUrl, parameters, headers, http_content);

    // Set the content that is returned from the api get request function
    this->response = http_content;

    // Check the response code to see if the request was successful
    if(responseCode == 200)
    {
        // If so, parse the response
        this->vectorResponse = parseBakupResponse(this->response);
    }

    // Return the response
    return responseCode;
}

int Request::apiGetRequest(string &url, cpr::Parameters &parameters, cpr::Header &headers, string &content)
{
    // Make the request to Bakup
    auto r = cpr::Get(cpr::Url{url},
                        parameters,
                        headers);

    // Set the returned content
    content = r.text;

    // Get error class
    this->error = r.error;

    // return the status code
    return r.status_code;
}

Request::Request(string baseUrl, string clientId, string apiToken, string agentVersion, Debug &debug)
        : baseUrl(std::move(baseUrl)), clientId(std::move(clientId)), apiToken(std::move(apiToken)), agentVersion(std::move(agentVersion)), debug(ref(debug)) {}

vector<command_t> Request::parseBakupResponse(string &jsonString)
{
    // Create  the vector to return
    vector<command_t> commands;

    // Initiate a document to hold the json values from the response
    Document bakupResponse;

    // Parse response and check if the JSON is valid
    if(!bakupResponse.Parse(jsonString.c_str()).HasParseError())
    {
        // For each job command in the json object
        for (auto &job : bakupResponse["jobs"].GetArray())
        {
            command_t temp;
            temp.id = job["id"].GetString();
            temp.jobType = job["job_type"].GetString();

            // Check for a target execution time
            if(job.HasMember("target_execution_time"))
            {
                temp.targetExecutionTime = job["target_execution_time"].GetInt();
            }
            else // If there is no specified execution time, execute now
            {
                temp.targetExecutionTime = 0;
            }

            // Get the jobs
            for(auto &command : job["job_commands"].GetArray())
            {
                temp.commands.emplace_back(command.GetString());
            }

            // Get the clean up jobs
            for(auto &command : job["clean_up_commands"].GetArray())
            {
                temp.cleanUpCommands.emplace_back(command.GetString());
            }

            // Check for refresh agent credentials setting
            if(job.HasMember("refresh_agent_credentials"))
            {
                temp.refreshAgentCredentials = job["refresh_agent_credentials"].GetBool();
            }

            // Add it to the returned vector
            commands.emplace_back(temp);
        }
    }
    else
    {
        // Print the error locally
        this->json = jsonString;
        this->debug.error("Invalid JSON received from Bakup");
        this->debug.info(this->json);
        this->JsonValid = false;
    }

    // Return the values
    return commands;
}

string Request::getResponse()
{
    return this->response;
}

vector<command_t> Request::getVectoredResponse()
{
    return this->vectorResponse;
}

cpr::Error Request::getError()
{
    return this->error;
}

string Request::getErrorMessage()
{
    return this->error.message;
}

cpr::ErrorCode Request::getErrorCode()
{
    return this->error.code;
}

bool Request::isJsonValid()
{
    return this->JsonValid;
}

string Request::getJson()
{
    return this->json;
}

cpr::Header Request::getDefaultHeaders(const map<string, string> &extraHeaders)
{
    cpr::Header defaultHeaders = {
            {"ClientID", this->clientId},
            {"Authorization", "Bearer " + this->apiToken},
            {"bakup-agent-version", this->agentVersion},
            {"bakup-rclone-version", RCloneVersionChecker::getRCloneVersion()}
    };

    for(auto const &[key, value] : extraHeaders)
    {
        defaultHeaders.emplace(key, value);
    }

    return defaultHeaders;
}
