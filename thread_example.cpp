#include <iostream>
#include <chrono>
#include <thread>
#include <memory>
#include <mutex>

#include "signomix.hpp"

std::mutex globalMutex;

void sendDataToSignomix(std::shared_ptr<signomix::HttpClient> deviceClient, const std::string& logPrefix)
{
    std::lock_guard<std::mutex> lock(globalMutex);

    // If you will have your proper credentials, move signIn function to main and invoke it once
    auto response = deviceClient->signIn("myLogin", "myPassword", "myDeviceEUI", "myDeviceKey");
    std::cout << std::boolalpha << logPrefix << "Starting session = " << !response.error << std::endl;

    if (response.error)
    {
        std::cout << logPrefix << "Failure code: " << response.httpCode << std::endl;
        std::cout << logPrefix << "Response description: " << response.description << std::endl;
        return;
    }
    // This is here only for example printing purposes. For "Unauthorized" async prints.

    deviceClient->newRequest();
    deviceClient->addData("temperature", 12.5);

    response = deviceClient->sendData();
    std::cout << logPrefix << "Response HTTP code: " << response.httpCode << std::endl;
    std::cout << logPrefix << "Response description: " << response.description << std::endl;

    std::string responseString = response.data;
    std::cout << logPrefix << responseString << std::endl;

    if (!response.error)
    {
        std::cout << logPrefix << "Data has been successfully sent parrallely" << std::endl;
    }
}

class ThreadRunner
{
public:
    ThreadRunner(std::shared_ptr<signomix::HttpClient> deviceClient)
    {
        auto asyncFunction = [deviceClient](){
            for (int i = 0; i < 5; i++)
            {
                sendDataToSignomix(deviceClient, "[ThreadRunner] ");
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }};

        thread = std::thread(asyncFunction);
    }

    ~ThreadRunner()
    {
        if (thread.joinable())
        { 
            thread.join();
        }
    }

private:
    std::thread thread;
};

int main()
{
    /* For multithread apps CURL object should be initialized that way, when only one thread  exists(main thread). */
    CURL* curl = signomix::initializeCurl();
    std::shared_ptr<signomix::HttpClient> deviceClient = std::make_shared<signomix::HttpClient>(curl);
    /*  You can use this function directly in constructor to simplify code.*/
    //auto deviceClient = std::make_shared<signomix::HttpClient>(signomix::initializeCurl());

    ThreadRunner asyncSender(deviceClient);

    for (int i = 0; i < 5; i++)
    {
        sendDataToSignomix(deviceClient, "[main] ");
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}