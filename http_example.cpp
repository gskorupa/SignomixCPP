#include <iostream>

#include "signomix.hpp"

int main()
{

    signomix::HttpClient ledClient("signomixcpp-test", "Test0000!", "0000-0000-0000", "34d97xxx00112");

    bool isSessionCreated = ledClient.createSession();
    std::cout << std::boolalpha << "Starting session = " << isSessionCreated << std::endl;

    // /* POST REQUEST */

    std::cout << "Sending HTTP request POST" << std::endl;

    ledClient.addPostField("led_green", 1);
    ledClient.addPostField("led_red", 0.0);
    ledClient.addPostField("led_yellow", 1.0);

    auto response = ledClient.sendPost();

    std::cout << "Response curl code: " << response.curlCode << std::endl;
    std::cout << "Response curl code description: " << response.description << std::endl;
    std::cout << "Response HTTP code: " << response.httpCode << std::endl;

    std::cout << "Data size: " << response.data.size() << std::endl;
    std::string responsePostValue = response.data;
    std::cout << "Response string length: " << responsePostValue.size() << std::endl;
    std::cout << responsePostValue << std::endl;

    if(response.error)
    {
        std::cerr << "Exiting with failure." << std::endl;
        return EXIT_FAILURE;
    }

    std::cout << "\nPOST Success!\n" << std::endl;

    ledClient.clearRequest();

    /* GET REQUEST */

    std::cout << "Sending HTTP request GET" << std::endl;

    ledClient.addGetFields("led_green,led_red,led_yellow");

    response = ledClient.sendGet(1);

    std::cout << "Response curl code: " << response.curlCode << std::endl;
    std::cout << "Response curl code description: " << response.description << std::endl;
    std::cout << "Response HTTP code: " << response.httpCode << std::endl;
    std::cout << "Data size: " << response.data.size() << std::endl;
    std::string responseGetValue = response.data;
    std::cout << responseGetValue << std::endl;

    if(response.error)
    {
        std::cerr << "Exiting with failure." << std::endl;
        return EXIT_FAILURE;
    }

    std::cout << "\nGET Success!" << std::endl;
    return EXIT_SUCCESS;
}
