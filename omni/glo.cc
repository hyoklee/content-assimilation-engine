#include <iostream>
#include <string>
#include <chrono>    // For std::chrono::seconds
#include <thread>    // For std::this_thread::sleep_for
#include <sstream>   // For std::stringstream

// POCO Includes
#include <Poco/Net/HTTPSClientSession.h>
#include <Poco/Net/HTTPRequest.h>
#include <Poco/Net/HTTPResponse.h>
#include <Poco/URI.h>
#include <Poco/StreamCopier.h>
#include <Poco/Exception.h> // For POCO exceptions
#include <Poco/Net/Context.h> // For SSL context

// JSON Parsing
#include <nlohmann/json.hpp>

// Function to perform an HTTP GET request using POCO
std::string httpGet(const std::string& url, const std::string& accessToken) {
    try {
        Poco::URI uri(url);
        // Create an SSL Context for HTTPS
        // Using `TLSV12_CLIENT_USE` for a reasonable default, adjust as needed.
        // For production, you might want more robust certificate validation.
        Poco::Net::Context::Ptr context = new Poco::Net::Context(Poco::Net::Context::TLSV12_CLIENT_USE, "", "", "", Poco::Net::Context::VERIFY_NONE);

        Poco::Net::HTTPSClientSession session(uri.getHost(), uri.getPort(), context);
        Poco::Net::HTTPRequest request(Poco::Net::HTTPRequest::HTTP_GET, uri.getPathAndQuery(), Poco::Net::HTTPMessage::HTTP_1_1);

        request.set("Authorization", "Bearer " + accessToken);
        request.set("Accept", "application/json"); // Request JSON response

        session.sendRequest(request);

        Poco::Net::HTTPResponse response;
        std::istream& rs = session.receiveResponse(response);

        std::stringstream ss;
        Poco::StreamCopier::copyStream(rs, ss);

        if (response.getStatus() == Poco::Net::HTTPResponse::HTTP_OK) {
            return ss.str();
        } else {
            std::cerr << "HTTP GET request failed with status: " << response.getStatus()
                      << " " << response.getReason() << std::endl;
            std::cerr << "Response body: " << ss.str() << std::endl;
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

// Function to perform an HTTP POST request using POCO
std::string httpPost(const std::string& url, const std::string& accessToken, const std::string& jsonPayload) {
    try {
        Poco::URI uri(url);
        Poco::Net::Context::Ptr context = new Poco::Net::Context(Poco::Net::Context::TLSV12_CLIENT_USE, "", "", "", Poco::Net::Context::VERIFY_NONE);
        Poco::Net::HTTPSClientSession session(uri.getHost(), uri.getPort(), context);

        Poco::Net::HTTPRequest request(Poco::Net::HTTPRequest::HTTP_POST, uri.getPathAndQuery(), Poco::Net::HTTPMessage::HTTP_1_1);
        request.set("Authorization", "Bearer " + accessToken);
        request.set("Content-Type", "application/json");
        request.setContentLength(jsonPayload.length()); // Set content length for POST body

        std::ostream& os = session.sendRequest(request);
        os << jsonPayload; // Write the JSON payload to the request body

        Poco::Net::HTTPResponse response;
        std::istream& rs = session.receiveResponse(response);

        std::stringstream ss;
        Poco::StreamCopier::copyStream(rs, ss);

        if (response.getStatus() == Poco::Net::HTTPResponse::HTTP_OK || response.getStatus() == Poco::Net::HTTPResponse::HTTP_ACCEPTED) {
            return ss.str();
        } else {
            std::cerr << "HTTP POST request failed with status: " << response.getStatus()
                      << " " << response.getReason() << std::endl;
            std::cerr << "Response body: " << ss.str() << std::endl;
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


// Function to get Globus transfer status
std::string getGlobusTransferStatus(const std::string& transferTaskId, const std::string& accessToken) {
    std::string apiUrl = "https://transfer.api.globusonline.org/v0.10/task/" + transferTaskId;
    return httpGet(apiUrl, accessToken);
}

// Function to request a Globus data transfer
std::string requestGlobusTransfer(
    const std::string& sourceEndpointId,
    const std::string& destinationEndpointId,
    const std::string& sourcePath,
    const std::string& destinationPath,
    const std::string& accessToken,
    const std::string& transferLabel = "C++ Programmed Transfer"
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
    transferRequest["recursive"] = false; // Set to true if transferring directories
    transferRequest["files"] = nlohmann::json::array(); // Array to hold transfer items

    // Add the single file transfer item
    nlohmann::json fileItem;
    fileItem["DATA_TYPE"] = "transfer_item";
    fileItem["source_path"] = sourcePath;
    fileItem["destination_path"] = destinationPath;
    fileItem["recursive"] = false; // Must match the top-level 'recursive' for this item

    transferRequest["files"].push_back(fileItem);

    // Convert JSON payload to string
    std::string jsonPayload = transferRequest.dump();

    std::string apiUrl = "https://transfer.api.globusonline.org/v0.10/transfer";
    return httpPost(apiUrl, accessToken, jsonPayload);
}


int main() {
    // --- Configuration ---
    // Replace with your actual Globus transfer task ID and access token
    // You would typically obtain the access token through an OAuth2 flow.
    // For testing, you might manually generate one or use a client credential flow.
    std::string globusTransferTaskId = "YOUR_EXISTING_GLOBUS_TRANSFER_TASK_ID"; // Example: "b876a3b0-2c97-11eb-a5f1-0a6311b8b60e"
    const std::string globusAccessToken = "YOUR_GLOBUS_ACCESS_TOKEN"; // This is a sensitive credential!

    // --- Configuration for New Transfer Request ---
    const std::string sourceEndpointId = "YOUR_SOURCE_ENDPOINT_ID";       // e.g., "d34ad5a0-2c87-11eb-a5f1-0a6311b8b60e"
    const std::string destinationEndpointId = "YOUR_DESTINATION_ENDPOINT_ID"; // e.g., "d34ad5a0-2c87-11eb-a5f1-0a6311b8b60f"
    const std::string sourcePath = "/~/source_file.txt";                   // Path on the source endpoint
    const std::string destinationPath = "/~/destination_folder/copied_file.txt"; // Path on the destination endpoint

    if (globusAccessToken == "YOUR_GLOBUS_ACCESS_TOKEN") {
        std::cerr << "Please update 'globusAccessToken' in the source code." << std::endl;
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
        std::string jsonResponse = getGlobusTransferStatus(globusTransferTaskId, globusAccessToken);

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
