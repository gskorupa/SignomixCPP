#include <iostream>

#include "signomix.hpp"

int main()
{

    signomix::HttpClient ledClient("0000-0000-0000", "34d97xxx00112");

    /* POST REQUEST */

    std::cout << "Sending HTTP request POST" << std::endl;

    ledClient.addField("led_green", 1);
    ledClient.addField("led_red", 0.0);
    ledClient.addField("led_yellow", 1.0);

    //auto response = ledClient.sendPost();
    auto response = signomix::Response{};
    // TODO: Correctly clean curl object - second request always not working

    std::cout << "Response curl code: " << response.curlCode << std::endl;
    std::cout << "Response curl code description: " << response.description << std::endl;
    std::cout << "Response HTTP code: " << response.httpCode << std::endl;

    if(response.error)
    {
        std::cerr << "Exiting with failure." << std::endl;
        return EXIT_FAILURE;
    }

    std::cout << "Data size: " << response.data.size() << std::endl;
    std::string responseValue = response.getString();
    std::cout << "Response string length: " << responseValue.size() << std::endl;
    std::cout << responseValue << std::endl;

    std::cout << "POST Success!" << std::endl;

    ledClient.clearRequest();

    /* GET REQUEST */

    std::cout << "Sending HTTP request GET" << std::endl;

    std::cout << "Starting session with token = ";
    std::string token = ledClient.startSession("signomixcpp-test", "Test0000!");
    std::cout << token << std::endl;

    return EXIT_SUCCESS;
}

