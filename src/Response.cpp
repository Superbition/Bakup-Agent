#include <Response.h>

// Set the initial class variables
Response::Response(string baseUrl, string clientId, string apiToken, string agentVersion)
: baseUrl(std::move(baseUrl)), clientId(std::move(clientId)), apiToken(std::move(apiToken)), agentVersion(std::move(agentVersion)) {}

// Post data to a URL
int Response::apiPostData(string &url, cpr::Header &headers, string &postData, string &postResponse)
{
    // Make the post to Bakup
    cpr::Response r = cpr::Post(cpr::Url{url},
                                cpr::Header{headers},
                                cpr::Body{postData});

    // Set the returned content
    postResponse = r.text;

    // Get error class
    this->error = r.error;

    // return the status code
    return r.status_code;
}

// Send job confirmation information back to bakup
int Response::postJobConfirmation(string &postData)
{
    // Add the authorisation token to the headers
    const map<string, string> extraHeaders = {{"Content-Type", "text/json"}};
    cpr::Header headers = this->getDefaultHeaders(extraHeaders);

    // Variable to store response data inside
    string responseData;

    // Build the job confirmation url
    string url = this->secureProtocol + this->baseUrl + this->bakupJobConfirmationUrl;

    // Post the data
    int responseCode = this->apiPostData(url, headers, postData, responseData);

    // Set the response data that is returned from Bakup
    this->response = responseData;

    // Return the response code
    return responseCode;
}

// Send job confirmation information back to bakup
int Response::postJobError(string &postData)
{
    // Add the authorisation token to the headers
    const map<string, string> extraHeaders = {{"Content-Type", "text/json"}};
    cpr::Header headers = this->getDefaultHeaders(extraHeaders);

    string url = this->secureProtocol + this->baseUrl + this->bakupJobErrorUrl;

    // Variable to store response data inside
    string responseData;

    // Post the data
    int responseCode = this->apiPostData(url, headers, postData, responseData);

    // Set the response data that is returned from Bakup
    this->response = responseData;

    // Return the response code
    return responseCode;
}

int Response::postSSLError(string &postData)
{
    // Add the authorisation token to the headers
    cpr::Header headers = cpr::Header{{"ClientID", this->clientId}, {"Content-Type", "text/json"}};
    string url = this->insecureProtocol + this->baseUrl + this->bakupSSLError;

    // Variable to store response data inside
    string responseData;

    // Post the data
    int responseCode = this->apiPostData(url, headers, postData, responseData);

    // Set the response data that is returned from Bakup
    this->response = responseData;

    // Return the response code
    return responseCode;
}

int Response::postInitialisationPing(string &postData)
{
    // No parameters are required for this request, so create a blank variable
    cpr::Parameters parameters = cpr::Parameters{};

    // Add the authorisation token to the headers
    const map<string, string> extraHeaders = {{"Content-Type", "text"}};
    cpr::Header headers = this->getDefaultHeaders(extraHeaders);

    string initialisationPingUrl = this->secureProtocol + this->baseUrl + this->initialisationUrl;

    // Variable to store content inside
    string responseData;

    // Make the request
    int responseCode = this->apiPostData(initialisationPingUrl, headers, postData, responseData);

    // Set the http content
    this->response = responseData;

    // Return response code
    return responseCode;
}

string Response::getResponse()
{
    return this->response;
}

cpr::Error Response::getError()
{
    return this->error;
}

cpr::ErrorCode Response::getErrorCode()
{
    return this->error.code;
}

string Response::getErrorMessage()
{
    return this->error.message;
}

cpr::Header Response::getDefaultHeaders(const map<string, string> &extraHeaders)
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
