# SignomixCPP
Signomix client C++ library for connect your device with https://signomix.com/ via HTTP protocol. The only dependency are libcurl and libb64. So you must install it on your device. This is header-only library in **signomix.hpp**, so just include it to your project and add **-lcurl** and **-lb64** flags to your build system. Library supports sending and downloading data from Signomix service and it is based on HTTPS protocol, so your data will be safe.

The target are embedded devices with Linux operating system, such as Raspberry Pi or Beaglebone board. 

### Installation of dependencies
For Linux
```bash
sudo apt-get install libcurl4-gnutls-dev
sudo apt-get install libb64-dev
```

### Building and running the example
```bash
make
./example
```

### Usage
First step is creation of `HttpClient`. You must push to constructor four string values. Your account credentials, devce EUI number and your device secret key. These are values which you set on your account in Signomix platform. So the concept is: one HttpClient for each device. But you can change device, also in single client. You can even switch account, but this is a rather rare case.
```c++
signomix::HttpClient client("login", "password", "device-eui", "device-secret");

client.changeDevice("next-device-eui", "next-device-secret");
client.changeAccount("next-login", "next-password");
```
If you are working with your own instance of Signomix located for example on localhost or somewhere else. Use different constructor
```c++
signomix::HttpClient client("login", "password", "your-signomix-url", "device-eui", "device-secret");
```
Your HTTP client is alomst ready to start communication with Signomix platform.
Last thing you must do is create a user session and check if it started successfully.
```c++
bool session = client.createSession();
```
After that you can start operating with the data.

#### Sending data
Each request must be started with `newRequest()` function. It ensures that every earlier data fields and dependencies are cleared.
After that add your data fields and send the request. Field name is a string type, but field's value, can be number or text type ().
```c++
client.newRequest();

client.addData("temperature", 12.5);
client.addData("humidity", "48%");
client.addData("some_sensor", 0.0f);

signomix::Response response = client.sendData();

if (response.error)
{
    std::cout << "Error: " << response.description << std::endl;
}
```
From `Response` type you can get values: error, description, data, curlCode and httpCode. In simple case only check if error exists. `response.description` is filled when error appears. If no error, than request has been succesfully sent. When you set that response to provide you some data, they will be available under `response.data` variable. Which is representation of `std::string`. Full usage of response you can see in example **http_example.cpp**.

#### Getting data
Getting data is more simple, because it is only one method. Fields are passed into the function as a text and they must be separated by comas, without whitespaces.
The second parameter is not needed. You can put there amount of last records, you want to get. It is 1 by default.
```c++
client.newRequest();
response = client.getData("temperature,humidity", 5);

```