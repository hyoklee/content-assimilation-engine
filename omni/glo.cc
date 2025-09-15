#include <iostream>
#include <string>
#include <chrono>    // For std::chrono::seconds
#include <thread>    // For std::this_thread::sleep_for
#include <sstream>   // For std::stringstream

#ifdef USE_POCO
// POCO Includes
#include <Poco/Net/HTTPSClientSession.h>
#include <Poco/Net/HTTPRequest.h>
#include <Poco/Net/HTTPResponse.h>
#include <Poco/URI.h>
#include <Poco/StreamCopier.h>
#include <Poco/Exception.h> // For POCO exceptions
#include <Poco/Net/Context.h> // For SSL context
#endif

// JSON Parsing
#include <nlohmann/json.hpp>

// Globus utilities
#include "format/globus_utils.h"

// Forward declarations
std::string getGlobusTransferStatus(const std::string& transferTaskId, const std::string& accessToken);
std::string requestGlobusTransfer(
    const std::string& sourceEndpointId,
    const std::string& destinationEndpointId,
    const std::string& sourcePath,
    const std::string& destinationPath,
    const std::string& accessToken,
    const std::string& transferLabel
);

#ifdef USE_POCO
// Function to perform an HTTP GET request using POCO
std::string httpGet(const std::string& url, const std::string& accessToken) {
    try {
        Poco::URI uri(url);
        // Create an SSL Context for HTTPS
        Poco::Net::Context::Ptr context = new Poco::Net::Context(
            Poco::Net::Context::CLIENT_USE, 
            "", "", "", 
            Poco::Net::Context::VERIFY_NONE, 
            9, // Default options
            false, // Don't load default CAs
            "ALL:!ADH:!LOW:!EXP:!MD5:@STRENGTH" // Cipher list
        );

        // Set up the session with timeout
        Poco::Net::HTTPSClientSession session(uri.getHost(), uri.getPort(), context);
        session.setTimeout(Poco::Timespan(30, 0)); // 30 second timeout

        // Prepare the request
        std::string path = uri.getPathAndQuery();
        if (path.empty()) {
            path = "/";
        }

        Poco::Net::HTTPRequest request(Poco::Net::HTTPRequest::HTTP_GET, path, Poco::Net::HTTPMessage::HTTP_1_1);
        request.set("Authorization", "Bearer " + accessToken);
        request.set("Accept", "application/json");
        request.set("User-Agent", "OMNI-Globus-Client/1.0");

        // Send the request
        session.sendRequest(request);

        // Get the response
        Poco::Net::HTTPResponse response;
        std::istream& rs = session.receiveResponse(response);
        std::stringstream responseBody;
        Poco::StreamCopier::copyStream(rs, responseBody);
        std::string responseStr = responseBody.str();

        // Debug output
        std::cout << "HTTP GET Response Status: " << response.getStatus() << " " << response.getReason() << std::endl;
        std::cout << "Response Body: " << responseStr << std::endl;

        if (response.getStatus() == Poco::Net::HTTPResponse::HTTP_OK) {
            return responseStr;
        } else {
            std::cerr << "HTTP GET request failed with status: " << response.getStatus()
                     << " " << response.getReason() << std::endl;
            std::cerr << "Response body: " << responseStr << std::endl;
            return "";
        }
    } catch (Poco::Exception& e) {
        std::cerr << "POCO Exception in httpGet: " << e.displayText() << std::endl;
        return "";
    } catch (std::exception& e) {
        std::cerr << "Standard Exception in httpGet: " << e.what() << std::endl;
        return "";
    }
}
#else
// Stub implementation when POCO is disabled
std::string httpGet(const std::string& url, const std::string& accessToken) {
    std::cerr << "Error: HTTP GET not supported - POCO is disabled" << std::endl;
    return "";
}
#endif

#ifdef USE_POCO
// Function to perform an HTTP POST request using POCO
std::string httpPost(const std::string& url, const std::string& accessToken, const std::string& jsonPayload) {
    try {
        Poco::URI uri(url);
        Poco::Net::Context::Ptr context = new Poco::Net::Context(Poco::Net::Context::CLIENT_USE, "", "", "", Poco::Net::Context::VERIFY_NONE);
        
        // Set up the session
        Poco::Net::HTTPSClientSession session(uri.getHost(), uri.getPort(), context);
        
        // Prepare the request
        std::string path = uri.getPathAndQuery();
        if (path.empty()) {
            path = "/";
        }
        
        Poco::Net::HTTPRequest request(Poco::Net::HTTPRequest::HTTP_POST, path, Poco::Net::HTTPMessage::HTTP_1_1);
        request.setContentType("application/json");
        request.set("Authorization", "Bearer " + accessToken);
        request.set("Accept", "application/json");
        
        // Set content length
        request.setContentLength(jsonPayload.length());
        
        // Send the request
        std::ostream& os = session.sendRequest(request);
        os << jsonPayload;
        
        // Get the response
        Poco::Net::HTTPResponse response;
        std::istream& responseStream = session.receiveResponse(response);
        std::stringstream responseBody;
        Poco::StreamCopier::copyStream(responseStream, responseBody);
        std::string responseStr = responseBody.str();
        
        // Debug output
        std::cout << "HTTP POST Response Status: " << response.getStatus() 
                 << " " << response.getReason() << std::endl;
        std::cout << "Response Body: " << responseStr << std::endl;
        
        // Check for success status codes
        if (response.getStatus() == Poco::Net::HTTPResponse::HTTP_OK || 
            response.getStatus() == Poco::Net::HTTPResponse::HTTP_ACCEPTED) {
            return responseStr;
        } else {
            std::cerr << "HTTP POST request failed with status: " 
                     << response.getStatus() << " " << response.getReason() << std::endl;
            std::cerr << "Response body: " << responseStr << std::endl;
            return "";
        }
    } catch (Poco::Exception& e) {
        std::cerr << "POCO Exception in httpPost: " << e.displayText() << std::endl;
        return "";
    } catch (std::exception& e) {
        std::cerr << "Standard Exception in httpPost: " << e.what() << std::endl;
        return "";
    }
}
#else
// Stub implementation when POCO is disabled
std::string httpPost(const std::string& url, const std::string& accessToken, const std::string& jsonPayload) {
    std::cerr << "Error: HTTP POST not supported - POCO is disabled" << std::endl;
    return "";
}
#endif

// Function to get Globus transfer status
std::string getGlobusTransferStatus(const std::string& transferTaskId, const std::string& accessToken) {
    std::string apiUrl = "https://transfer.api.globusonline.org/v0.10/task/" + transferTaskId;
    return httpGet(apiUrl, accessToken);
}

#ifdef USE_POCO
// Implementation of the Globus file transfer function
// This is called by the wrapper in globus_utils.cpp
extern "C" bool transfer_globus_file_impl(
    const std::string& source_uri,
    const std::string& dest_uri,
    const std::string& access_token,
    const std::string& transfer_label
) {
    std::cout << "Starting Globus transfer with the following parameters:" << std::endl;
    std::cout << "  Source URI: " << source_uri << std::endl;
    std::cout << "  Destination URI: " << dest_uri << std::endl;
    std::cout << "  Transfer label: " << transfer_label << std::endl;
    
    if (access_token.empty()) {
        std::cerr << "Error: Access token is empty" << std::endl;
        return false;
    }
    
    std::string src_endpoint, src_path, dst_endpoint, dst_path;
    
    // Parse source URI
    std::cout << "Parsing source URI..." << std::endl;
    if (!parse_globus_uri(source_uri, src_endpoint, src_path)) {
        std::cerr << "Error: Failed to parse source Globus URI: " << source_uri << std::endl;
        return false;
    }
    
    // Parse destination URI
    std::cout << "Parsing destination URI..." << std::endl;
    if (!parse_globus_uri(dest_uri, dst_endpoint, dst_path)) {
        std::cerr << "Error: Failed to parse destination Globus URI: " << dest_uri << std::endl;
        return false;
    }
    
    std::cout << "Initiating Globus transfer..." << std::endl;
    std::cout << "  Source Endpoint: " << src_endpoint << std::endl;
    std::cout << "  Source Path:     " << src_path << std::endl;
    std::cout << "  Dest Endpoint:   " << dst_endpoint << std::endl;
    std::cout << "  Dest Path:       " << dst_path << std::endl;
    
    // Request the transfer
    std::string response = requestGlobusTransfer(
        src_endpoint, 
        dst_endpoint,
        src_path,
        dst_path,
        access_token,
        transfer_label
    );
    
    if (response.empty()) {
        std::cerr << "Error: Failed to initiate Globus transfer" << std::endl;
        return false;
    }
    
    try {
        // Parse the JSON response
        nlohmann::json jsonResponse = nlohmann::json::parse(response);
        
        // Check if the transfer was successfully submitted
        if (jsonResponse.contains("code") && jsonResponse["code"] == "Accepted") {
            std::string taskId = jsonResponse["task_id"];
            std::cout << "Transfer submitted successfully. Task ID: " << taskId << std::endl;
            
            // Poll for transfer completion
            const int max_attempts = 30; // 30 attempts with 10s delay = 5 minutes max
            const int delay_seconds = 10;
            
            for (int attempt = 1; attempt <= max_attempts; ++attempt) {
                std::cout << "Checking transfer status (attempt " << attempt << "/" << max_attempts << ")..." << std::endl;
                
                std::string statusResponse = getGlobusTransferStatus(taskId, access_token);
                if (!statusResponse.empty()) {
                    nlohmann::json statusJson = nlohmann::json::parse(statusResponse);
                    std::string status = statusJson["status"];
                    
                    std::cout << "  Status: " << status << std::endl;
                    
                    if (status == "SUCCEEDED") {
                        std::cout << "Transfer completed successfully!" << std::endl;
                        return true;
                    } else if (status == "FAILED" || status == "INACTIVE") {
                        std::cerr << "Transfer failed with status: " << status << std::endl;
                        if (statusJson.contains("details")) {
                            std::cerr << "Details: " << statusJson["details"] << std::endl;
                        }
                        return false;
                    }
                }
                
                // Wait before polling again
                std::this_thread::sleep_for(std::chrono::seconds(delay_seconds));
            }
            
            std::cerr << "Transfer did not complete within the expected time." << std::endl;
            return false;
        } else {
            std::cerr << "Error submitting transfer: " << response << std::endl;
            return false;
        }
    } catch (const std::exception& e) {
        std::cerr << "Error processing transfer response: " << e.what() << std::endl;
        return false;
    }
}
#else
// Stub implementation when POCO is disabled
extern "C" bool transfer_globus_file_impl(
    const std::string& source_uri,
    const std::string& dest_uri,
    const std::string& access_token,
    const std::string& transfer_label
) {
    std::cerr << "Error: Globus transfer not supported - POCO is disabled" << std::endl;
    return false;
}
#endif

#ifdef USE_POCO
// Function to request a Globus data transfer
std::string requestGlobusTransfer(
    const std::string& sourceEndpointId,
    const std::string& destinationEndpointId,
    const std::string& sourcePath,
    const std::string& destinationPath,
    const std::string& accessToken,
    const std::string& transferLabel
) {
    // Create the JSON payload for the transfer request
    nlohmann::json transferRequest;
    transferRequest["DATA_TYPE"] = "transfer";
    transferRequest["submission_id"] = ""; // Globus requires this, but we leave it empty for their server to generate
    transferRequest["source_endpoint"] = sourceEndpointId;
    transferRequest["destination_endpoint"] = destinationEndpointId;
    transferRequest["label"] = transferLabel;
    transferRequest["sync_level"] = 0; // 0 for no sync (default), 1 for checksum, 2 for size, 3 for mtime
    transferRequest["verify_checksum"] = true; // Recommend verifying checksums
    
    // Create the DATA array for transfer items
    nlohmann::json transferItem;
    transferItem["DATA_TYPE"] = "transfer_item";
    transferItem["source_path"] = sourcePath;
    transferItem["destination_path"] = destinationPath;
    transferItem["recursive"] = false;
    
    // Add the transfer item to the DATA array
    nlohmann::json dataArray = nlohmann::json::array();
    dataArray.push_back(transferItem);
    
    // Add the DATA array to the request (Globus API v0.10 expects 'DATA' in uppercase)
    transferRequest["DATA"] = dataArray;

    // Convert JSON payload to string with pretty printing for debugging
    std::string jsonPayload = transferRequest.dump(2); // Indent with 2 spaces for readability
    
    // Debug output
    std::cout << "\nSending transfer request to Globus API:" << std::endl;
    std::cout << "URL: https://transfer.api.globusonline.org/v0.10/transfer" << std::endl;
    std::cout << "Payload:\n" << jsonPayload << std::endl;

    // First, get a submission ID
    std::string submissionUrl = "https://transfer.api.globus.org/v0.10/submission_id";
    std::string submissionResponse = httpGet(submissionUrl, accessToken);
    
    try {
        nlohmann::json submissionJson = nlohmann::json::parse(submissionResponse);
        if (submissionJson.contains("value")) {
            std::string submissionId = submissionJson["value"];
            std::cout << "Obtained submission ID: " << submissionId << std::endl;
            
            // Update the transfer request with the submission ID
            transferRequest["submission_id"] = submissionId;
            
            // Update the JSON payload with the new submission ID
            jsonPayload = transferRequest.dump(2);
            std::cout << "Sending transfer request with payload:\n" << jsonPayload << std::endl;
            
            // Now submit the transfer request
            std::string transferUrl = "https://transfer.api.globus.org/v0.10/transfer";
            std::string response = httpPost(transferUrl, accessToken, jsonPayload);
            
            // Debug output for response
            std::cout << "\nReceived response from Globus API:" << std::endl;
            std::cout << response << std::endl;
            
            try {
                // Try to parse the response to check for success
                auto responseJson = nlohmann::json::parse(response);
                if (responseJson.contains("code") && responseJson["code"] == "Accepted") {
                    std::cout << "Transfer submitted successfully!" << std::endl;
                    std::cout << "Task ID: " << responseJson.value("task_id", "unknown") << std::endl;
                }
            } catch (const std::exception& e) {
                std::cerr << "Warning: Could not parse API response: " << e.what() << std::endl;
            }
            
            return response;
        } else {
            std::cerr << "Error: Failed to get submission ID from Globus API" << std::endl;
            return "";
        }
    } catch (const std::exception& e) {
        std::cerr << "Error parsing submission ID response: " << e.what() << std::endl;
        return "";
    }
}
#else
// Stub implementation when POCO is disabled
std::string requestGlobusTransfer(
    const std::string& sourceEndpointId,
    const std::string& destinationEndpointId,
    const std::string& sourcePath,
    const std::string& destinationPath,
    const std::string& accessToken,
    const std::string& transferLabel
) {
    std::cerr << "Error: Globus transfer request not supported - POCO is disabled" << std::endl;
    return "";
}
#endif

#ifdef USE_POCO
int test_globus() {
    // --- Configuration ---
    // Replace with your actual Globus transfer task ID and access token
    // You would typically obtain the transfer token through an OAuth2 flow.
    // For testing, you might manually generate one or use a client credential flow.
    std::string globusTransferTaskId = "YOUR_EXISTING_GLOBUS_TRANSFER_TASK_ID"; // Example: "b876a3b0-2c97-11eb-a5f1-0a6311b8b60e"
    const std::string globusTransferToken = "YOUR_GLOBUS_TRANSFER_TOKEN"; // This is a sensitive credential!

    // --- Configuration for New Transfer Request ---
    const std::string sourceEndpointId = "YOUR_SOURCE_ENDPOINT_ID";       // e.g., "d34ad5a0-2c87-11eb-a5f1-0a6311b8b60e"
    const std::string destinationEndpointId = "YOUR_DESTINATION_ENDPOINT_ID"; // e.g., "d34ad5a0-2c87-11eb-a5f1-0a6311b8b60f"
    const std::string sourcePath = "/~/source_file.txt";                   // Path on the source endpoint
    const std::string destinationPath = "/~/destination_folder/copied_file.txt"; // Path on the destination endpoint

    if (globusTransferToken == "YOUR_GLOBUS_TRANSFER_TOKEN") {
        std::cerr << "Please update 'globusTransferToken' in the source code." << std::endl;
        std::cerr << "Also update endpoint IDs and paths if you wish to request a new transfer." << std::endl;
        return 1;
    }

    // --- Part 1: Request a New Transfer (Optional - Uncomment to enable) ---
    // Make sure to set proper source/destination endpoint IDs and paths
    /*
    std::cout << "Attempting to request a new Globus transfer..." << std::endl;
    std::string transferResponse = requestGlobusTransfer(
        sourceEndpointId,
        destinationEndpointId,
        sourcePath,
        destinationPath,
        globusAccessToken,
        "My C++ Transfer Example (POCO)"
    );

    if (!transferResponse.empty()) {
        try {
            nlohmann::json json = nlohmann::json::parse(transferResponse);
            std::string message = json.value("message", "No message");
            std::string taskId = json.value("task_id", "No Task ID");
            std::cout << "\n--- Transfer Request Response ---" << std::endl;
            std::cout << "Message: " << message << std::endl;
            std::cout << "New Task ID: " << taskId << std::endl;
            // If you successfully started a new transfer, update globusTransferTaskId
            // with the new taskId to check its status below.
            globusTransferTaskId = taskId;
        } catch (const nlohmann::json::parse_error& e) {
            std::cerr << "JSON parsing error for transfer request response: " << e.what() << std::endl;
            std::cerr << "Received response: " << transferResponse << std::endl;
            return 1; // Exit if transfer request failed to parse
        } catch (const nlohmann::json::exception& e) {
            std::cerr << "JSON access error for transfer request response: " << e.what() << std::endl;
            std::cerr << "Received response: " << transferResponse << std::endl;
            return 1; // Exit if transfer request failed to access JSON elements
        }
    } else {
        std::cerr << "Failed to request a new Globus transfer." << std::endl;
        return 1; // Exit if transfer request failed
    }
    */

    std::cout << "\n--- Part 2: Checking Status of an Existing Transfer ---" << std::endl;

    if (globusTransferTaskId == "YOUR_EXISTING_GLOBUS_TRANSFER_TASK_ID") {
         std::cerr << "Please provide a valid 'globusTransferTaskId' to check status, or uncomment the transfer request part AND update its configuration." << std::endl;
         return 1;
    }

    std::cout << "Monitoring status for Globus Transfer Task ID: " << globusTransferTaskId << std::endl;

    std::string status = "ACTIVE"; // Initial status to enter the loop

    // Loop until the transfer is complete (SUCCEEDED or FAILED)
    while (status != "SUCCEEDED" && status != "FAILED") {
        std::string jsonResponse = getGlobusTransferStatus(globusTransferTaskId, globusTransferToken);

        if (!jsonResponse.empty()) {
            try {
                nlohmann::json json = nlohmann::json::parse(jsonResponse);
                status = json.value("status", "unknown"); // Update status
                std::string label = json.value("label", "No Label");
                int filesTransferred = json.value("files_transferred", 0);
                int filesTotal = json.value("files_total", 0);
                std::string startTime = json.value("request_time", "N/A");
                std::string endTime = json.value("completion_time", "N/A");
                std::string deadline = json.value("deadline", "N/A");


                std::cout << "\n--- Transfer Details (Current Status: " << status << ") ---" << std::endl;
                std::cout << "Task Label: " << label << std::endl;
                std::cout << "Files Transferred: " << filesTransferred << std::endl;
                std::cout << "Total Files: " << filesTotal << std::endl;
                std::cout << "Start Time: " << startTime << std::endl;
                std::cout << "Completion Time: " << endTime << std::endl;
                std::cout << "Deadline: " << deadline << std::endl;


                if (status == "SUCCEEDED") {
                    std::cout << "\nGlobus transfer request is COMPLETE and SUCCEEDED." << std::endl;
                } else if (status == "FAILED") {
                    std::cout << "\nGlobus transfer request is COMPLETE but FAILED." << std::endl;
                    std::string fatalError = json.value("fatal_error", "No specific error message.");
                    std::cout << "Fatal Error: " << fatalError << std::endl;
                } else {
                    std::cout << "Transfer still " << status << ". Waiting 10 seconds before checking again..." << std::endl;
                    std::this_thread::sleep_for(std::chrono::seconds(10)); // Wait for 10 seconds
                }

            } catch (const nlohmann::json::parse_error& e) {
                std::cerr << "JSON parsing error: " << e.what() << std::endl;
                std::cerr << "Received response: " << jsonResponse << std::endl;
                status = "FAILED"; // Mark as failed to exit loop on parsing error
            } catch (const nlohmann::json::exception& e) {
                std::cerr << "JSON access error: " << e.what() << std::endl;
                std::cerr << "Received response: " << jsonResponse << std::endl;
                status = "FAILED"; // Mark as failed to exit loop on access error
            }
        } else {
            std::cerr << "Failed to retrieve Globus transfer status. Retrying in 10 seconds..." << std::endl;
            std::this_thread::sleep_for(std::chrono::seconds(10)); // Wait before retrying
            // Don't set status to FAILED immediately to allow for transient network issues
        }
    }

    return 0;
}
#else
// Stub implementation when POCO is disabled
int test_globus() {
    std::cerr << "Error: Globus test not supported - POCO is disabled" << std::endl;
    return 1;
}
#endif
