#ifndef BAKUP_AGENT_RESPONSE_H
#define BAKUP_AGENT_RESPONSE_H

#include <string>
#include <cpr/cpr.h>
#include <cpr/error.h>
#include <vector>
#include <rapidjson/document.h>
#include <rapidjson/writer.h>
#include <rapidjson/stringbuffer.h>
#include <RCloneVersionChecker.h>

using namespace std;
using namespace rapidjson;

class Response
{
    private:
        // Client Id
        const string clientId;

        // Api Token
        const string apiToken;

        // Insecure protocol
        const string insecureProtocol = "http://";

        // Secure protocol
        const string secureProtocol = "https://";

        // URL to access
        const string baseUrl;

        // Url for job confirmations
        const string bakupJobConfirmationUrl = "/job/confirm";

        // Url for job errors
        const string bakupJobErrorUrl = "/error";

        // Url for SSL error
        const string bakupSSLError = "/error/ssl";

        // URL to send first initialisation ping
        const string initialisationUrl = "/initialisation";

        // Store errors
        cpr::Error error;

        // Response from bakup
        string response;

        // Post data to a URL
        int apiPostData(string &url, cpr::Header &headers, string &postData, string &postResponse);

        // Hold the agent's version
        string agentVersion;

        // Generate required headers for a response sent to bakup
        cpr::Header getDefaultHeaders(const map<string, string> &extraHeaders = {});

    public:
        // Construct the class
        Response(string baseUrl, string clientId, string apiToken, string agentVersion);

        // Send job confirmation information back to bakup
        int postJobConfirmation(string &postData);

        // Post an error
        int postJobError(string &postData);

        // Post an SSL error via insecure methods
        int postSSLError(string &postData);

        // Send initialisation ping and os information
        int postInitialisationPing(string &postData);

        // Get response data from server in case of error
        string getResponse();

        // Get error status
        cpr::Error getError();

        // Get error code
        cpr::ErrorCode getErrorCode();

        // Get error message
        string getErrorMessage();
};

#endif //BAKUP_AGENT_RESPONSE_H
