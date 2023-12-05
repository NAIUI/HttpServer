#ifndef HTTP_REQUEST_H
#define HTTP_REQUEST_H

#include <unordered_map>
#include <unordered_set>
#include <string>
#include <regex>

#include "buffer.h"

class HTTPrequest
{
public:
    enum PARSE_STATE{       // 当前解析的状态
        REQUEST_LINE,
        HEADERS,
        BODY,
        FINISH,
    };

    enum HTTP_CODE {        // HTTTP的解析结果
        NO_REQUEST = 0,
        GET_REQUEST,
        BAD_REQUEST,
        NO_RESOURSE,
        FORBIDDENT_REQUEST,
        FILE_REQUEST,
        INTERNAL_ERROR,
        CLOSED_CONNECTION,
    };

    HTTPrequest() {init();};
    ~HTTPrequest()=default;

    void init();
    bool parse(Buffer& buff);                               // 解析HTTP请求

    //获取HTTP信息      提供给上层的服务接口，获取路径、HTTP 方式、版本
    std::string path() const;
    std::string& path();
    std::string method() const;
    std::string version() const;
    std::string getPost(const std::string& key) const;
    std::string getPost(const char* key) const;

    bool isKeepAlive() const;

private:
    bool parseRequestLine_(const std::string& line);        // 解析请求行
    void parseRequestHeader_(const std::string& line);      // 解析请求头
    void parseDataBody_(const std::string& line);           // 解析数据体

    void parsePath_();                                      // 在解析请求行的时候，会解析出路径信息
    void parsePost_();                                      // POST请求

    static int convertHex(char ch);

    PARSE_STATE state_;                                                 // 自动状态机
    std::string method_, path_, version_, body_;                        // HTTP 方式、路径、版本和数据体
    std::unordered_map<std::string,std::string> header_;                // 请求头
    std::unordered_map<std::string,std::string> post_;                  // post 已经解析出来的信息

    static const std::unordered_set<std::string> DEFAULT_HTML;          // 默认的网页名称

};

#endif  //HTTP_REQUEST_H