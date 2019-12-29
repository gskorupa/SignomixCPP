#include <curl/curl.h>
#include <curl/easy.h>

#include <string>
#include <vector>

namespace signomix
{

constexpr auto __DEFALUT_CODE = 0;
constexpr auto HTTP_CREATED = 201;
constexpr auto CURL_NO_ERROR = "No error";

struct ByteData
{
  std::vector<char> memory;
};

size_t writeCallback(void *buffer, size_t len, size_t nmemb, void *userp)
{
    std::cout << "Callback run!" << std::endl;

    if (not buffer or not userp)
    {
        std::cerr << "No data in response" << std::endl;
    }

    ByteData* data = static_cast<ByteData*>(userp);
    char* buff = static_cast<char*>(buffer);
    data->memory.push_back(*buff);

    return nmemb;
}

enum class Method
{
    POST,
    GET  // not supported !
};

struct Response
{
    bool error;
    int curlCode;
    int httpCode;
    std::string description;
    ByteData data;
};

class HttpClient
{
public:
    HttpClient()
    {
        curl_global_init(CURL_GLOBAL_ALL);
    }

    ~HttpClient()
    {
        curl_global_cleanup();
    }

    void setUrl(const std::string& url)
    {
        url_ = url;
    }

    void setMethod(const Method& method)
    {
        method_ = method;
    }

    template <typename ValueType>
    void addField(const std::string& fieldName, const ValueType& value)
    {
        fields_ += "&" + fieldName + "=" + std::to_string(value);
    }

    void setEui(const std::string& eui)
    {
        eui_ = "eui=" + eui;
    }

    void setSecret(const std::string& secret)
    {
        secretKey_ = secret;
    }

    Response send()
    {
        Response response{false, __DEFALUT_CODE, __DEFALUT_CODE, "", {}};

        if (method_ != Method::POST)
        {
            /* for this moment only POST method is supported! */
            response.error = true;
            response.curlCode = CURLE_COULDNT_CONNECT;
            response.description = "Only POST method is supported!";
            return response;
        }

        if (not url_.empty()
            and not fields_.empty()
            and not secretKey_.empty()
            and not eui_.empty())
        {
            if (curl_)
            {
                curl_ = curl_easy_init();
                curl_easy_setopt(curl_, CURLOPT_URL, url_.c_str());

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
                // file = fopen("file_data.txt","w");
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
            }
            else
            {
                response.error = true;
                response.curlCode = CURLE_COULDNT_CONNECT;
                response.description = "No connection!";
            }
        }
        else
        {
            response.error = true;
            response.curlCode = CURLE_COULDNT_CONNECT;
            response.description = "Make sure that you set method, eui, secret and fields. " \
                "These data are necessarily for push a request!";
        }

        return response;
    }

private:
    CURL *curl_;
    CURLcode curlCode_;

    std::string eui_;
    std::string secretKey_;
    std::string url_;
    std::string fields_;
    Method method_;
};

} // namespace signomix