#pragma once

#include <b64/encode.h>
#include <curl/curl.h>
#include <curl/easy.h>

#include <string>
#include <type_traits>

namespace signomix
{

constexpr auto CURL_NO_ERROR = "No error";
constexpr auto HTTP_OK = 200;
constexpr auto HTTP_CREATED = 201;
constexpr auto HTTP_UNAUTHORIZED = 403;

namespace
{

constexpr auto _DEFALUT_CODE = 0;
constexpr auto _EMPTY = 0;
constexpr auto _SERVICE_PATH = "https://signomix.com";
constexpr auto _GET_DATA_PATH = "/api/iot/device/";
constexpr auto _POST_AUTH_PATH = "/api/auth/";
constexpr auto _POST_DATA_PATH = "/api/i4t";
constexpr auto _RECONNECT_LIMIT = 3;

inline size_t writeCallback(void *contents, size_t size, size_t nmemb, void *userp)
{
    size_t realsize = size * nmemb;
    if (not contents or not userp)
    {
        return 0;
    }

    std::string* data = static_cast<std::string*>(userp);
    data->clear();

    char* bytes = static_cast<char*>(contents);
    std::string output{bytes};

    *data = output;

    return realsize;
}

} // namespace

inline CURL* initializeCurl()
{
    /*
     * CURL object initialization (NOT THREAD SAFE!)
     * Helper function for multi-thread applications.
     */

    CURL* curl;
    curl_global_init(CURL_GLOBAL_ALL);
    /* Ingnoring "curl uninitialized" warning for GCC. CURL object is initialized by above function.*/
#pragma GCC diagnostic ignored "-Wuninitialized"
    return curl;
}

struct HttpResponse
{
    /*
     * Structure for handling data from HTTP requests. Both POST and GET.
     * Data are handled in 'data' memeber. For POST it is a plain text,
     * but for GET it is a JSON format.
     */

    bool error;
    int curlCode;
    int httpCode;
    std::string description;
    std::string data;
};

class HttpClient
{
public:
    /*
     * This is default non-argument constructor.
     * If your application is single-thread use it.
     */
    HttpClient()
    {
        curl_global_init(CURL_GLOBAL_ALL);
    }

    /*
     * This is recommended for muliti-thread applications
     * CURL object initialization is not thread safe, so you must invoke
     * initializeCurl() function before you start any thread
     * Proposed usage:
     * HttpClient client(initializeCurl());
     */
    HttpClient(CURL* curl)
    : curl_(curl)
    {
    }

    /*
     * This is signIn function.
     * Conventoion is a one HttpClient per device, so you must pass your account and device credentials into it.
     * Of course you can simple switch accounts and devices inside object.
     * If you want do this, use functions like changeAccount() or change Device()
     */
    HttpResponse signIn(const std::string& login, const std::string& password,
               const std::string& eui, const std::string& secret)
    {
        login_ = login;
        password_= password;
        serviceUrl_= _SERVICE_PATH;
        eui_= eui;
        secretKey_= secret;

        return createSession();
    }

    /*
     * This is a non-standard signIn function..
     * If you have your own instance of Signomix server with different service location,
     * than you should use this version
     */
    HttpResponse signIn(const std::string& login, const std::string& password, const std::string& serviceUrl,
               const std::string& eui, const std::string& secret)
    {
        login_ = login;
        password_= password;
        serviceUrl_= serviceUrl;
        eui_= eui;
        secretKey_= secret;

        return createSession();
    }

    ~HttpClient()
    {
        curl_global_cleanup();
    }

    /*
     * Functions below allows you to switch account or used device.
     * After changing the account you must signIn() again.
     */
    void changeAccount(const std::string& login, const std::string& password)
    {
        login_ = login;
        password_ = password;
    }

    void changeDevice(const std::string& eui, const std::string& secret)
    {
        eui_ = eui;
        secretKey_ = secret;
    }

    /*
     * If you want to verify Signomix certificate, use this function with path to your Bundle of CA Root Certificates.
     * You can download it for example from cURL: https://curl.haxx.se/docs/caextract.html
     */
    void setSSLverificationEnabled(const std::string& cacert)
    {
        cacert_ = cacert;
    }

    /*
     * IMPORTANT: Use this function before each single request you send.
     */
    void newRequest()
    {
        fields_.clear();
        faliedAuthCounter = 0;
    }

    /*
     * Adding a simngle POST field. As value you should treat every primitive numeric type which can be put into std::to_string function.
     * It will be checked at compile time, so bad type won't compile.
     */
    template <class ValueType, typename std::enable_if<std::is_arithmetic<ValueType>::value, ValueType>::type* = nullptr>
    void addData(const std::string& fieldName, ValueType value)
    {
        fields_ += "&" + fieldName + "=" + std::to_string(value);
    }

    /*
     * Function for sending POST request to update or create your device data on Signomix platform.
     *
     * IMPORTANT: Remeber that post fields must be filled before.
     */
    HttpResponse sendData()
    {
        HttpResponse response{false, _DEFALUT_CODE, _DEFALUT_CODE, "", {}};

        if (not checkFields(fields_, response))
        {
            return response;
        }

        if (curl_)
        {
            curl_ = curl_easy_init();
            std::string postUrl{serviceUrl_ + _POST_DATA_PATH};
            curl_easy_setopt(curl_, CURLOPT_URL, postUrl.c_str());

            struct curl_slist *headers = NULL;
            std::string authMess{"Authorization: " + secretKey_};

            headers = curl_slist_append(headers, authMess.c_str());
            headers = curl_slist_append(headers, "Accept: application/x-www-form-urlencoded");

            curl_easy_setopt(curl_, CURLOPT_HTTPHEADER, headers);

            std::string message = "eui=" + eui_ + fields_;

            if (cacert_.empty())
            {
                curl_easy_setopt(curl_, CURLOPT_SSL_VERIFYPEER, 0L);
                curl_easy_setopt(curl_, CURLOPT_SSL_VERIFYHOST, 0L);
            }
            else
            {
                curl_easy_setopt(curl_, CURLOPT_SSL_VERIFYPEER, 1L);
                curl_easy_setopt(curl_, CURLOPT_SSL_VERIFYHOST, 2L);
                curl_easy_setopt(curl_, CURLOPT_CAINFO, cacert_.c_str());
            }

            curl_easy_setopt(curl_, CURLOPT_POST, 1);
            curl_easy_setopt(curl_, CURLOPT_POSTFIELDS, message.c_str());
            curl_easy_setopt(curl_, CURLOPT_POSTFIELDSIZE, static_cast<long>(message.size()));

            setCallback(response);

            curlCode_ = curl_easy_perform(curl_);

            responseFillAndExpectHttpCode(response, HTTP_CREATED);

            curl_easy_cleanup(curl_);
            curl_slist_free_all(headers);
        }
        else
        {
            response.error = true;
            response.curlCode = CURLE_COULDNT_CONNECT;
            response.description = "No connection or some CURL library internal issue!";
        }

        return response;
    }

    /*
     * Function for sending GET request to get your device data from Signomix platform.
     * To first parameter put fileds as one string with fileds separeted with comas. No whitespaces!
     * example: addGetFields("temperature,humidity")
     * If you are interested in more number of last records of data, put number as a second function parameter.
     * It is 1 by default.
     *
     * IMPORTANT: Remeber that get fields must be filled before.
     */
    HttpResponse getData(const std::string& fields, int recordsNumber = 1)
    {
        HttpResponse response{false, _DEFALUT_CODE, _DEFALUT_CODE, "", {}};

        if (not checkFields(fields, response))
        {
            return response;
        }

        if (curl_)
        {
            curl_ = curl_easy_init();

            std::string getUrlWithFields{serviceUrl_ + _GET_DATA_PATH + eui_ + '/' + fields + "?query=last%20" + std::to_string(recordsNumber)};
            curl_easy_setopt(curl_, CURLOPT_URL, getUrlWithFields.c_str());

            struct curl_slist *headers = NULL;
            std::string authMess{"Authentication: " + sessionToken_};

            headers = curl_slist_append(headers, authMess.c_str());
            headers = curl_slist_append(headers, "Accept: application/json");
            curl_easy_setopt(curl_, CURLOPT_HTTPHEADER, headers);

            setCallback(response);

            curlCode_ = curl_easy_perform(curl_);

            responseFillAndExpectHttpCode(response, HTTP_OK);

            curl_easy_cleanup(curl_);
            curl_slist_free_all(headers);
        }
        else
        {
            response.error = true;
            response.curlCode = CURLE_COULDNT_CONNECT;
            response.description = "No connection or some CURL library internal issue!";
        }

        if (response.httpCode == HTTP_UNAUTHORIZED)
        {
            // Session token is expired. Trying to renew access token.
            createSession();
            if (faliedAuthCounter++ < _RECONNECT_LIMIT)
            {
                // Resending actual GET request with new token
                return getData(fields, recordsNumber);
            }
            else
            {
                // Reconnection limit has been exceeded. Probably bad account credentials was given.
                return response;
            }
        }

        faliedAuthCounter = 0;
        return response;
    }

private:
    /*
     * This is the first function that must be run at the begining of HttpClient usage.
     * This is automaticly invoked by signIn() function
     *
     * It creates you user session and automaticly recreated when it is needed.
     * One invoke per object. It doesn't located in HttpClient constructor,
     * because some needed objects are reachable only after constructor end.
     */
    HttpResponse createSession()
    {
        std::string credentials{login_ + ":" + password_ + "\n"};

        constexpr int max_len{100};
        base64::encoder encoder;
        char encoded[max_len];

        encoder.encode(credentials.c_str(), credentials.size(), encoded);
        std::string encodedCredentials{encoded};

        auto response = getSessionToken(encodedCredentials);
        sessionToken_ = response.data;
        return response;
    }

    HttpResponse getSessionToken(const std::string& encodedCredentials)
    {
        HttpResponse response{false, _DEFALUT_CODE, _DEFALUT_CODE, "", {}};

        if (curl_)
        {
            curl_ = curl_easy_init();

            std::string authUrl{serviceUrl_ + _POST_AUTH_PATH};
            curl_easy_setopt(curl_, CURLOPT_URL, authUrl.c_str());

            struct curl_slist *headers = NULL;
            std::string authMess{"Authentication: Basic " + encodedCredentials};

            headers = curl_slist_append(headers, authMess.c_str());
            headers = curl_slist_append(headers, "Accept: text/plain");
            curl_easy_setopt(curl_, CURLOPT_HTTPHEADER, headers);

            setCallback(response);

            // Setting empty POST fields for authentication

            curl_easy_setopt(curl_, CURLOPT_POST, 1);
            curl_easy_setopt(curl_, CURLOPT_POSTFIELDS, "");
            curl_easy_setopt(curl_, CURLOPT_POSTFIELDSIZE, static_cast<long>(_EMPTY));

            if (cacert_.empty())
            {
                curl_easy_setopt(curl_, CURLOPT_SSL_VERIFYPEER, 0L);
                curl_easy_setopt(curl_, CURLOPT_SSL_VERIFYHOST, 0L);
            }
            else
            {
                curl_easy_setopt(curl_, CURLOPT_SSL_VERIFYPEER, 1L);
                curl_easy_setopt(curl_, CURLOPT_SSL_VERIFYHOST, 2L);
                curl_easy_setopt(curl_, CURLOPT_CAINFO, cacert_.c_str());
            }

            curlCode_ = curl_easy_perform(curl_);

            responseFillAndExpectHttpCode(response, HTTP_OK);

            curl_easy_cleanup(curl_);
            curl_slist_free_all(headers);
        }
        else
        {
            response.error = true;
            response.curlCode = CURLE_COULDNT_CONNECT;
            response.description = "No connection or some CURL library internal issue!";
        }

        return response;
    }

    void setCallback(HttpResponse& response)
    {
        curl_easy_setopt(curl_, CURLOPT_WRITEDATA, &response.data);
        curl_easy_setopt(curl_, CURLOPT_WRITEFUNCTION, writeCallback);
    }

    bool checkFields(const std::string& fields, HttpResponse& response)
    {
        if (fields.empty())
        {
            response.error = true;
            response.curlCode = CURLE_COULDNT_CONNECT;
            response.description = "Empty fileds! Request has not been sent.";
            return false;
        }

        return true;
    }

    void responseFillAndExpectHttpCode(HttpResponse& response, int expextedHttpCode)
    {
        curl_easy_getinfo(curl_, CURLINFO_RESPONSE_CODE, &response.httpCode);

        if(curlCode_ != CURLE_OK or response.httpCode != expextedHttpCode)
        {
            response.error = true;
            response.curlCode = curlCode_;
        }
        response.description = curl_easy_strerror(curlCode_);

        if (response.error and response.description == CURL_NO_ERROR)
        {
            response.description = "HTTP error " + std::to_string(response.httpCode);
        }
    }

    /*
     * Private members
     */

    CURL* curl_;
    CURLcode curlCode_;

    std::string login_;
    std::string password_;
    std::string serviceUrl_;
    std::string eui_;
    std::string secretKey_;
    std::string fields_;
    std::string sessionToken_;
    std::string cacert_;

    int faliedAuthCounter{0};
};

} // namespace signomix
