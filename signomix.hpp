#include <curl/curl.h>
#include <curl/easy.h>

#include <b64/encode.h>

#include <algorithm>
#include <string>
#include <vector>

namespace signomix
{

namespace
{
constexpr auto _EMPTY = 0;
constexpr auto _DEFALUT_CODE = 0;
constexpr auto _POST_AUTH_URL = "https://signomix.com/api/auth/";
constexpr auto _RECONNECT_LIMIT = 3;
}

constexpr auto CURL_NO_ERROR = "No error";
constexpr auto HTTP_OK = 200;
constexpr auto HTTP_CREATED = 201;
constexpr auto HTTP_UNAUTHORIZED = 403;

using ByteData = std::vector<char>;

size_t writeCallback(void *contents, size_t size, size_t nmemb, void *userp)
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

struct HttpResponse
{
    bool error;
    int curlCode;
    int httpCode;
    std::string description;
    std::string data;

    HttpResponse& operator=(const HttpResponse& other)
    {
        error = other.error;
        curlCode = other.curlCode;
        httpCode = other.httpCode;
        description = other.description;
        data = other.data;

        return *this;
    }
};

class HttpClient
{
public:
    /*
     * Default non-argument constructor has been deleted.
     * For proper usage please use one of two defined constructors.
     */
    HttpClient() = delete;

    /*
     * This is a standard constructor.
     * Conventoion is a one HttpClient per device, so you must pass your account and device credentials into it.
     * Of course you can simple switch accounts and devices inside object.
     * If you want do this, use functions like changeAccount() or change Device()
     */
    HttpClient(const std::string& login, const std::string& password,
               const std::string& eui, const std::string& secret)
        : login_(login)
        , password_(password)
        , postUrl_("https://signomix.com/api/i4t")
        , getUrl_("https://signomix.com/api/iot/device/")
        , eui_(eui)
        , secretKey_(secret)
    {
        curl_global_init(CURL_GLOBAL_ALL);
    }

    /*
     * This is a non-standard constructor.
     * If you have your own instance of Signomix server with different endpoints.
     * than you should use this constructor
     */
    HttpClient(const std::string& login, const std::string& password,
               const std::string& postUrl, const std::string& getUrl,
               const std::string& eui, const std::string& secret)
        : login_(login)
        , password_(password)
        , postUrl_(postUrl)
        , getUrl_(getUrl)
        , eui_(eui)
        , secretKey_(secret)
    {
        curl_global_init(CURL_GLOBAL_ALL);
    }

    ~HttpClient()
    {
        curl_global_cleanup();
    }

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

    template <typename ValueType>
    void addPostField(const std::string& fieldName, const ValueType& value)
    {
        fields_ += "&" + fieldName + "=" + std::to_string(value);
    }

    void addGetFields(const std::string& fields)
    {
        fields_.clear();
        fields_ += fields + "?query=last%20";
    }

    void clearRequest()
    {
        fields_.clear();
        faliedAuthCounter = 0;
    }

    HttpResponse sendPost()
    {
        HttpResponse response{false, _DEFALUT_CODE, _DEFALUT_CODE, "", {}};

        if (not checkFields(response))
        {
            return response;
        }

        if (curl_)
        {
            curl_ = curl_easy_init();
            curl_easy_setopt(curl_, CURLOPT_URL, postUrl_.c_str());

            struct curl_slist *headers = NULL;
            std::string authMess{"Authorization: " + secretKey_};

            headers = curl_slist_append(headers, authMess.c_str());
            headers = curl_slist_append(headers, "Accept: application/x-www-form-urlencoded");

            curl_easy_setopt(curl_, CURLOPT_HTTPHEADER, headers);

            std::string message = "eui=" + eui_ + fields_;

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
            response.description = "No connection!";
        }

        return response;
    }

    HttpResponse sendGet(int recordsNumber)
    {
        HttpResponse response{false, _DEFALUT_CODE, _DEFALUT_CODE, "", {}};

        if (not checkFields(response))
        {
            return response;
        }

        if (curl_)
        {
            curl_ = curl_easy_init();

            std::string getUrlWithFields{getUrl_ + eui_ + '/' + fields_ + std::to_string(recordsNumber)};
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
            response.description = "No connection!";
        }

        if (response.httpCode == HTTP_UNAUTHORIZED)
        {
            // Session token is expired. Trying to renew access token.
            createSession();
            if (faliedAuthCounter++ < _RECONNECT_LIMIT)
            {
                // Resending actual GET request with new token
                return sendGet(recordsNumber);
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

    bool createSession()
    {
        std::string credentials{login_ + ":" + password_ + "\n"};

        constexpr int max_len{100};
        base64::encoder encoder;
        char encoded[max_len];

        encoder.encode(credentials.c_str(), credentials.size(), encoded);
        std::string encodedCredentials{encoded};

        auto response = getSessionToken(encodedCredentials);
        if (response.error)
        {
            return false;
        }

        sessionToken_ = response.data;
        return true;
    }
private:
    HttpResponse getSessionToken(const std::string& encodedCredentials)
    {
        HttpResponse response{false, _DEFALUT_CODE, _DEFALUT_CODE, "", {}};

        if (curl_)
        {
            curl_ = curl_easy_init();
            curl_easy_setopt(curl_, CURLOPT_URL, _POST_AUTH_URL);

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

            curlCode_ = curl_easy_perform(curl_);

            responseFillAndExpectHttpCode(response, HTTP_OK);

            curl_easy_cleanup(curl_);
            curl_slist_free_all(headers);
        }

        return response;
    }

    void setCallback(HttpResponse& response)
    {
        curl_easy_setopt(curl_, CURLOPT_WRITEDATA, &response.data);
        curl_easy_setopt(curl_, CURLOPT_WRITEFUNCTION, writeCallback);
    }

    bool checkFields(HttpResponse& response)
    {
        if (fields_.empty())
        {
            response.error = true;
            response.curlCode = CURLE_COULDNT_CONNECT;
            response.description = "Empty fileds! GET request has not been sent.";
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
    std::string postUrl_;
    std::string getUrl_;
    std::string eui_;
    std::string secretKey_;
    std::string fields_;
    std::string sessionToken_;

    int faliedAuthCounter{0};
};

} // namespace signomix
