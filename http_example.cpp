#include <iostream>

#include "signomix.hpp"

int main()
{
    std::cout << "Sending HTTP request POST" << std::endl;

    signomix::HttpClient ledClient("0000-0000-0000", "34d97xxx00112");
    ledClient.addField("led_green", 1);
    ledClient.addField("led_red", 0.0);
    ledClient.addField("led_yellow", 1.0);

    auto response = ledClient.sendPost();

    std::cout << "Response curl code: " << response.curlCode << std::endl;
    std::cout << "Response curl code description: " << response.description << std::endl;
    std::cout << "Response HTTP code: " << response.httpCode << std::endl;

    if(response.error)
    {
        std::cerr << "Exiting with failure." << std::endl;
        return EXIT_FAILURE;
    }

    // response.data usage example
    // initializing std::string with response.data (std::vector<char>)
    std::string responseString{response.data.begin(), response.data.end()};
    if (responseString == "hello!")
    {
        std::cout << "Led values has been changed!" << std::endl;
    }

    std::cout << "SUCCESS !" << std::endl;
    return EXIT_SUCCESS;
}