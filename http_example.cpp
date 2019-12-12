#include <iostream>
#include "signomix.hpp"

int main()
{
    std::cout << "Sending HTTP request POST" << std::endl;

    signomix::HttpConnector http;
    http.setUrl("http://localhost:8000");
    http.setFields("Initial POST method HTTP!");
    http.setMethod(signomix::Method::POST);
    
    auto response = http.send();

    std::cout << "Http POST send with response: " << response.code << std::endl;
    if(response.error)
    {
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}