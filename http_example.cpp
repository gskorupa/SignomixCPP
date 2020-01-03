#include <iostream>

#include "signomix.hpp"

int main()
{
    std::cout << "Sending HTTP request POST" << std::endl;

    signomix::HttpClient http("0000-0000-0000", "34d97xxx00112");
    http.addField("led_green", 1);
    http.addField("led_red", 0.0);
    http.addField("led_yellow", 1.0);

    auto response = http.sendPost();

    std::cout << "Response curl code: " << response.curlCode << std::endl;
    std::cout << "Response curl code description: " << response.description << std::endl;
    std::cout << "Response HTTP code: " << response.httpCode << std::endl;
    std::cout << "Response data: ";

    if (not response.data.empty())
    {
        std::copy(response.data.begin(), response.data.end(), std::ostreambuf_iterator<char>(std::cout));
        std::cout << " [size = " << response.data.size() << "]";
    }
    std::cout << std::endl;

    if(response.error)
    {
        std::cerr << "Exiting with failure." << std::endl;
        return EXIT_FAILURE;
    }

    std::cout << "SUCCESS !" << std::endl;
    return EXIT_SUCCESS;
}