#include <iostream>

#include "signomix.hpp"

int main()
{
    signomix::HttpClient ledClient("signomixcpp-test", "Test0000!", "0000-0000-0000", "34d97xxx00112");

    bool isSessionCreated = ledClient.createSession();
    std::cout << std::boolalpha << "Starting session = " << isSessionCreated << std::endl;

    if (not isSessionCreated)
    {
        std::cerr << "Exiting with failure." << std::endl;
        return EXIT_FAILURE;
    }

    /* SENDING DATA */

    std::cout << "Sending data via HTTP request POST" << std::endl;

    ledClient.newRequest();
    ledClient.addData("led_green", 0.0);
    ledClient.addData("led_red", 1);
    ledClient.addData("led_yellow", 0.0f);

    auto response = ledClient.sendData();

    std::cout << "Response HTTP code: " << response.httpCode << std::endl;
    std::cout << "Response description: " << response.description << std::endl;

    std::string responseString = response.data;
    std::cout << responseString << std::endl;

    if(response.error)
    {
        std::cerr << "Exiting with failure." << std::endl;
        return EXIT_FAILURE;
    }

    std::cout << "\nSending data success!\n" << std::endl;

    /* GET REQUEST */

    std::cout << "Sending HTTP request GET" << std::endl;

    ledClient.newRequest();
    response = ledClient.getData("led_green,led_red,led_yellow");

    std::cout << "Response HTTP code: " << response.httpCode << std::endl;
    std::cout << "Response curl code description: " << response.description << std::endl;
    responseString = response.data;
    std::cout << responseString << std::endl;

    if(response.error)
    {
        std::cerr << "Exiting with failure." << std::endl;
        return EXIT_FAILURE;
    }

    std::cout << "\nGetting data success!" << std::endl;
    return EXIT_SUCCESS;
}
