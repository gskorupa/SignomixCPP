#include <curl/curl.h>
#include <curl/easy.h>
#include <string>
#include <vector>
#include <stdio.h>
#include <string.h>

namespace signomix
{

constexpr auto __DEFALUT_CODE = 0;
constexpr auto HTTP_CREATED = 201;

size_t write_data(void *buffer, size_t len, size_t nmemb, void *userp)
{
    // to handle data in the future

    (void)len;
    (void)buffer;
    (void)userp;

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
    std::vector<uint8_t> data;
};

class HttpClient
{
public:
    HttpClient()
    {
        curl_global_init(CURL_GLOBAL_ALL);
        curl_ = curl_easy_init();
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
                curl_easy_setopt(curl_, CURLOPT_URL, url_.c_str());

                struct curl_slist *headers = NULL;
                std::string authMess{"Authorization: " + secretKey_};

                /***
                 * for debbuging purposes */
                    std::cout << authMess << std::endl;
                /*
                ***/

                headers = curl_slist_append(headers, authMess.c_str());
                headers = curl_slist_append(headers, "Accept: application/x-www-form-urlencoded");

                curl_easy_setopt(curl_, CURLOPT_HTTPHEADER, headers);

                std::string message = eui_ + fields_;

                /***
                 * for debbuging purposes */
                    std::cout << message << std::endl;
                /*
                ***/

                curl_easy_setopt(curl_, CURLOPT_POST, 1);
                curl_easy_setopt(curl_, CURLOPT_POSTFIELDS, message.c_str());
                curl_easy_setopt(curl_, CURLOPT_POSTFIELDSIZE, (long)strlen(message.c_str()));

                curl_easy_setopt(curl_, CURLOPT_WRITEFUNCTION, write_data);


                curlCode_ = curl_easy_perform(curl_);

                // getting HTTP code from response
                curl_easy_getinfo (curl_, CURLINFO_RESPONSE_CODE, &response.httpCode);

                if(curlCode_ != CURLE_OK or response.httpCode != HTTP_CREATED)
                {
                    response.error = true;
                    response.curlCode = curlCode_;
                }
                response.description = curl_easy_strerror(curlCode_);

                /* always cleanup */ 
                curl_easy_cleanup(curl_);
            }
            else
            {
                response.error = true;
                response.curlCode = CURLE_COULDNT_CONNECT;
                response.description = "Connection error!";
            }
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