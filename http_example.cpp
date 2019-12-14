#include <iostream>
#include "signomix.hpp"

int main()
{
    std::cout << "Sending HTTP request POST" << std::endl;

    signomix::HttpClient http;
    http.setUrl("https://signomix.com/api/i4t");
    http.setEui("0000-0000-0000");
    http.setSecret("34d97xxx00112");
    http.addField("led_green", 1);
    http.addField("led_red", 0.0f);
    http.addField("led_yellow", 1.0);
    http.setMethod(signomix::Method::POST);

    auto response = http.send();

    std::cout << "Response curl code: " << response.curlCode << std::endl;
    std::cout << "Response curl code description: " << response.description << std::endl;
    std::cout << "Response HTTP code: " << response.httpCode << std::endl;
    if(response.error)
    {
        std::cerr << "Exiting with failure." << std::endl;
        return EXIT_FAILURE;
    }

    std::cout << "SUCCESS !" << std::endl;
    return EXIT_SUCCESS;
}