# SignomixCPP
Signomix client C++ library for connect your device with https://signomix.com/ via HTTP protocol. The only dependency are libcurl and libb64. So you must install it on your device. This is header-only, so just add include it to your project and add **-lcurl** and **-lb64** flags to your build system. Library supports POST and GET methods.
**For that moment only POST is supported!**

### Installation of libcurl and libb64
For Linux
```bash
sudo apt-get install libcurl4-gnutls-dev
sudo apt-get install libb64-dev
```
For Windows follow https://curl.haxx.se/windows/

### Building and running the example
```bash
make
./signomixcpp
```

### Usage
First step is creation of `HttpClient`. You must push to constructor two string values. First is your devce EUI number, second is your device secret key. These are values which you set on your account in Signomix platform. So the concept is: one HttpClient for each device. But you can change device, also in single client.
```c++
signomix::HttpClient client("device-eui", "device-secret");

client.changeDevice("next-device-eui", "next-device-secret");
```
If you are working with your own instance of Signomix on localhost or somewhere else. Use different constructor
```c++
signomix::HttpClient client("your-url", "device-eui", "device-secret");
```
The next step is HTTP fields creation. They are different for POST and GET method, just keep it in your mind.

#### POST
For this method add a field name as string and second paramter is a value, which can be number or text type.
```c++
client.addField("field_1", 1);
client.addField("field_2", 5.0);
```
After that you can send your POST request.
```c++
signomix::Response response = client.sendPost();

if (response.error)
{
    std::cout << "Error: " << response.description << std::endl;
}
```
From `Response` type you can get values: error, description, data, curlCode and httpCode. In simple case only check if error exists. `response.description` is filled when error appears. If no error, than request has been succesfully sent. When you set that response to provide you some data, they will be available under `response.data` variable. Which is representation of `std::vector<char>`.  Response is always a string. You can easy get it via `resonse.getString()` function. Full usage of response you can see in example. If you want to send another request (POST or GET) just clear fields and you can use your client again!
```c++
client.clearRequest();
```

#### GET
GET method is not supported yet.
