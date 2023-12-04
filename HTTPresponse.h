#ifndef HTTP_RESPONSE_H
#define HTTP_RESPONSE_H

#include <unordered_map>
#include <fcntl.h>  //open
#include <unistd.h> //close
#include <sys/stat.h> //stat
#include <sys/mman.h> //mmap,munmap
#include <assert.h>

#include "buffer.h"

class HTTPresponse
{
public:
    HTTPresponse();
    ~HTTPresponse();

    void init(const std::string& srcDir,std::string& path,bool isKeepAlive=false,int code=-1);
    void makeResponse(Buffer& buffer);                          // 生成响应报文
    void unmapFile_();                                          // 共享内存的扫尾函数
    void errorContent(Buffer& buffer,std::string message);      // 如果所请求的文件打不开，我们需要返回相应的错误信息

    // 上层接口
    int code() const {return code_;}                            // 返回状态码的函数
    // 返回共享内存信息的函数：
    char* file();
    size_t fileLen() const;


private:
    // 生成请求行，请求头和数据体
    void addStateLine_(Buffer& buffer);
    void addResponseHeader_(Buffer& buffer);
    void addResponseContent_(Buffer& buffer);

    void errorHTML_();                  // 4XX 的状态码
    std::string getFileType_();         // 文件类型信息

    int code_;                          // HTTP 的状态
    bool isKeepAlive_;                  // HTTP 连接是否处于 KeepAlive 状态

    std::string path_;                  // 解析得到的路径
    std::string srcDir_;                // 根目录

    // 共享内存
    char* mmFile_;
    struct stat mmFileStat_;

    static const std::unordered_map<std::string, std::string> SUFFIX_TYPE;          // 后缀名到文件类型的映射关系
    static const std::unordered_map<int, std::string> CODE_STATUS;                  // 状态码到相应状态 (字符串类型) 的映射
    static const std::unordered_map<int, std::string> CODE_PATH;
    
};



#endif //HTTP_RESPONSE_H