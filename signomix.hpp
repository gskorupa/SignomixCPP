#include <curl/curl.h>
#include <curl/easy.h>
#include <vector>

namespace signomix
{

enum class Method
{
    POST,
    GET
};

struct Response
{
    bool error;
    int code;
    std::string description;
    std::vector<uint8_t> data;
};

class HttpConnector
{
public:
    HttpConnector()
    {
        curl_global_init(CURL_GLOBAL_ALL);
        curl_ = curl_easy_init();
    }

    ~HttpConnector()
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

    void setFields(const std::string& fileds)
    {
        fileds_ = fileds;
    }

    Response send()
    {
        Response response{false, CURLE_OK, "", {}};
        if (not url_.empty() and not fileds_.empty() /* and not method empty */)
        {
            if (curl_)
            {
                curl_easy_setopt(curl_, CURLOPT_URL, url_.c_str());
                /* Now specify the POST data */ 

                curl_easy_setopt(curl_, CURLOPT_POSTFIELDS, fileds_.c_str());
            
                response_ = curl_easy_perform(curl_);
         
                if(response_ != CURLE_OK)
                {
                    response.error = true;
                    response.code = response_;
                }
                response.description = curl_easy_strerror(response_);

                /* always cleanup */ 
                curl_easy_cleanup(curl_);
            }
            else
            {
                std::cerr << "[ERROR] Connection lost!" << std::endl;
                response.error = true;
                response.code = CURLE_COULDNT_CONNECT;
            }
        }

        return response;
    }

private:
    CURL *curl_;
    CURLcode response_;

    std::string url_;
    std::string fileds_;
    Method method_;
};

} // namespace signomix