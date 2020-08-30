# SignomixCPP
Signomix client C++ library for connect your device with https://signomix.com/ (or different Signomix platform location) via HTTP protocol. 

The only dependency are libcurl and libb64, so you must install it on your device. This is header-only library in **signomix.hpp**, so just include it to your project and add **-lcurl** and **-lb64** flags to your build system. The library supports sending and downloading data from Signomix service using HTTP or HTTPS protocol.

The target are embedded devices with Linux operating system, such as Raspberry Pi or Beaglebone board. 

### Installation of dependencies
For Linux (Ubuntu based distros)
```bash
sudo apt-get install libcurl4-gnutls-dev
sudo apt-get install libb64-dev
```

### Building and running the examples
```bash
cd examples/http_example && make
cd ../thread_example && make
```
Now you can go to the directory which you are interested in and run an executable.

### Usage
This is header-only library, so copy `signomix.hpp` into your working dir.
First step is creation of `HttpClient`. If you are developing single-thread application use standard default constructor.
**For multi-thread apps see the example**. Next you have to invoke a `signIn()` function and push there four string values.
Your account credentials, devce EUI number and your device secret key. These are values which you set on your account in Signomix platform. So the concept is: one HttpClient for each device. But you can change device, also in single client.
You can even switch account, but this is a rather rare case.

```c++
#include "signomix.hpp"

...

signomix::HttpClient client;
signomix::HttpResponse response = client.signIn("login", "password", "device-eui", "device-secret");

client.changeDevice("next-device-eui", "next-device-secret");
client.changeAccount("next-login", "next-password");
```
If you are working with your own instance of Signomix located for example on localhost or somewhere else you need to pass one parameter more - your signomix url
```c++
client.signIn("login", "password", "your-signomix-url", "device-eui", "device-secret");
```
Your HTTP client is alomst ready to start communication with Signomix platform. Last thing you must do is to check if user session started successfully.The result is `HttpResponse` type and it is more described in "Sending data" section. For this moment, try to check only whether error occurs. If it exists, you can get more information from `HttpResponse` type.
```c++
auto sessionResponse = client.signIn("login", "password", "device-eui", "device-secret");
if (sessionResponse.error)
{
    // do something
}
```
After that you can start operating with the data.

#### Sending data
Each request must be started with `newRequest()` function. It ensures that every earlier data fields and dependencies are cleared.
After that add your data fields and send the request. Field name is a string type, but field's value can be only numeric type and it will be checked at compile time.
```c++
client.newRequest();

client.addData("temperature", 12.5);
client.addData("some_sensor", 0.0f);

signomix::HttpResponse response = client.sendData();

if (response.error)
{
    std::cout << "Error: " << response.description << std::endl;
}
```
From `HttpResponse` type you can get values: error, description, data, curlCode and httpCode. In simple case only check if error exists. `response.description` is filled when error appears. If no error, than request has been succesfully sent. When you set that response to provide you some data, they will be available under `response.data` variable. Which is representation of `std::string`. Full usage of response you can see in example **http_example.cpp**.

#### Getting data
Getting data is more simple, because it is only one method. Fields are passed into the function as a text and they must be separated by comas, without whitespaces.
The second parameter is not needed. You can put there number of most recent records you want to get. It is 1 by default which means that you receive the most recent data record saved.
```c++
client.newRequest();
response = client.getData("temperature,humidity", 5);

```
In `response.data` you will have a string in JSON format. Operating on JSON objects is not implemented in this library, beacuse it is not the purpose of this project.

#### SSL verfication
SSL verification is disabled by default. If you want to verify Signomix certificate you must use this function once.
```c++
client.setSSLverificationEnabled("path/to/cert");
```
