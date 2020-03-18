#include <iostream>

#include "signomix.hpp"

int main()
{
    signomix::HttpClient deviceClient("myLogin", "myPassword", "myDeviceEUI", "myDeviceKey");

    auto sessionResponse = deviceClient.createSession();
    std::cout << std::boolalpha << "Starting session = " << !sessionResponse.error << std::endl;

    if (sessionResponse.error)
    {
        std::cout << "Failure code: " << sessionResponse.httpCode << std::endl;
        std::cout << "Response description: " << sessionResponse.description << std::endl;
        std::cout << "Response data received: " << sessionResponse.data << std::endl;
        return EXIT_FAILURE;
    }

    /* SENDING DATA */

    std::cout << "Sending data via HTTP request POST" << std::endl;

    deviceClient.newRequest();
    deviceClient.addData("temperature", 12.5);
    deviceClient.addData("humidity", 30);

    auto response = deviceClient.sendData();

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

    deviceClient.newRequest();
    response = deviceClient.getData("temperature,humidity");

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
