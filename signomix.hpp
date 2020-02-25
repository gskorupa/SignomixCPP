#include <curl/curl.h>
#include <curl/easy.h>

#include <b64/encode.h>

#include <string>
#include <vector>

namespace signomix
{

constexpr auto __DEFALUT_CODE = 0;
constexpr auto HTTP_OK = 200;
constexpr auto HTTP_CREATED = 201;
constexpr auto CURL_NO_ERROR = "No error";
constexpr auto POST_AUTH_URL = "https://signomix.com/api/auth";

using ByteData = std::vector<char>;

size_t writeCallback(void *contents, size_t size, size_t nmemb, void *userp)
{
    size_t realsize = size * nmemb;
    if (not contents or not userp)
    {
        std::cerr << "No data in response\n";
    }

    ByteData* data = static_cast<ByteData*>(userp);
    data->clear();

    char* bytes = static_cast<char*>(contents);
    for (size_t i = 0; i < realsize; i++)
    {
        data->push_back(bytes[i]);
    }

    return realsize;
}

struct Response
{
    bool error;
    int curlCode;
    int httpCode;
    std::string description;
    ByteData data;

    std::string getString() const
    {
        return std::string{data.begin(), data.end()};
    }
};

class HttpClient
{
public:
    HttpClient() = delete;

    HttpClient(const std::string& eui, const std::string& secret)
        : postUrl_("https://signomix.com/api/i4t")
        , getUrl_("https://signomix.com/api/iot/")
        , eui_("eui=" + eui)
        , secretKey_(secret)
    {
        curl_global_init(CURL_GLOBAL_ALL);
    }

    HttpClient(const std::string& url, const std::string& eui, const std::string& secret)
        : postUrl_(url)
        , getUrl_("https://signomix.com/api/iot/")
        , eui_("eui=" + eui)
        , secretKey_(secret)
    {
        curl_global_init(CURL_GLOBAL_ALL);
    }

    ~HttpClient()
    {
        curl_global_cleanup();
    }

    void changeDevice(const std::string& eui, const std::string& secret)
    {
        eui_ = "eui=" + eui;
        secretKey_ = secret;
    }

    template <typename ValueType>
    void addField(const std::string& fieldName, const ValueType& value)
    {
        fields_ += "&" + fieldName + "=" + std::to_string(value);
    }

    void clearRequest()
    {
        fields_.clear();
    }

    Response sendPost()
    {
        Response response{false, __DEFALUT_CODE, __DEFALUT_CODE, "", {}};

        if (fields_.empty())
        {
            response.error = true;
            response.curlCode = CURLE_COULDNT_CONNECT;
            response.description = "Empty fileds! POST request has not been sent.";
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

            std::string message = eui_ + fields_;

            curl_easy_setopt(curl_, CURLOPT_POST, 1);
            curl_easy_setopt(curl_, CURLOPT_POSTFIELDS, message.c_str());
            curl_easy_setopt(curl_, CURLOPT_POSTFIELDSIZE, static_cast<long>(message.size()));

            // ***** DATA TO CALLBACK ********
            curl_easy_setopt(curl_, CURLOPT_WRITEDATA, &response.data);
            curl_easy_setopt(curl_, CURLOPT_WRITEFUNCTION, writeCallback);

            // ***** DATA TO FILE ********
            // FILE * file;
            // file = fopen("response.txt","w");
            // curl_easy_setopt(curl_, CURLOPT_WRITEDATA, file);

            curlCode_ = curl_easy_perform(curl_);

            // getting HTTP code from response
            curl_easy_getinfo(curl_, CURLINFO_RESPONSE_CODE, &response.httpCode);

            if(curlCode_ != CURLE_OK or response.httpCode != HTTP_CREATED)
            {
                response.error = true;
                response.curlCode = curlCode_;
            }
            response.description = curl_easy_strerror(curlCode_);
        
            if (response.error and response.description == CURL_NO_ERROR)
            {
                response.description = "HTTP error " + std::to_string(response.httpCode);
            }

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

    Response sendGet()
    {
        Response response{false, __DEFALUT_CODE, __DEFALUT_CODE, "", {}};

        if (fields_.empty())
        {
            response.error = true;
            response.curlCode = CURLE_COULDNT_CONNECT;
            response.description = "Empty fileds! GET request has not been sent.";
            return response;
        }

        if (curl_)
        {
            curl_ = curl_easy_init();
            curl_easy_setopt(curl_, CURLOPT_URL, getUrl_.c_str());

            // GET Request

            curl_easy_cleanup(curl_);
        }
    }

    std::string startSession(const std::string& login, const std::string& password)
    {
        std::string credentials{login + ":" + password + "\n"};

        constexpr int max_len{100};
        base64::encoder encoder;
        char encoded[max_len];

        encoder.encode(credentials.c_str(), credentials.size(), encoded);
        std::string encodedCredentials{encoded};

        return getSessionToken(encodedCredentials);
    }

private:
    std::string getSessionToken(const std::string& encodedCredentials)
    {
        std::string token{};
        
        Response response{false, __DEFALUT_CODE, __DEFALUT_CODE, "", {}};

        if (curl_)
        {
            curl_ = curl_easy_init();
            curl_easy_setopt(curl_, CURLOPT_URL, POST_AUTH_URL);

            struct curl_slist *headers = NULL;
            std::string authMess{"Authentication: Basic " + encodedCredentials};

            headers = curl_slist_append(headers, authMess.c_str());
            headers = curl_slist_append(headers, "Accept: text/plain");
            curl_easy_setopt(curl_, CURLOPT_HTTPHEADER, headers);

            // ***** DATA TO CALLBACK ********
            curl_easy_setopt(curl_, CURLOPT_WRITEDATA, &response.data);
            curl_easy_setopt(curl_, CURLOPT_WRITEFUNCTION, writeCallback);

            curl_easy_setopt(curl_, CURLOPT_POST, 1);
            curl_easy_setopt(curl_, CURLOPT_POSTFIELDS, "");
            curl_easy_setopt(curl_, CURLOPT_POSTFIELDSIZE, static_cast<long>(0));

            curlCode_ = curl_easy_perform(curl_);

            curl_easy_getinfo(curl_, CURLINFO_RESPONSE_CODE, &response.httpCode);
            if(curlCode_ != CURLE_OK or response.httpCode != HTTP_OK)
            {
                response.error = true;
                response.curlCode = curlCode_;
            }
            response.description = curl_easy_strerror(curlCode_);
        
            if (response.error and response.description == CURL_NO_ERROR)
            {
                response.description = "HTTP error " + std::to_string(response.httpCode);
            }

            if (response.error)
            {
                token = "[ERROR] " + response.description;
            }
            else
            {
                token = response.getString();
            }

            curl_easy_cleanup(curl_);
            curl_slist_free_all(headers);
        }

        return token;
    }

    CURL* curl_;
    CURLcode curlCode_;

    std::string postUrl_;
    std::string getUrl_;
    std::string eui_;
    std::string secretKey_;
    std::string fields_;
};

} // namespace signomix