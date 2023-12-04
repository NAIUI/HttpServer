#include "HTTPrequest.h"

const std::unordered_set<std::string> HTTPrequest::DEFAULT_HTML{
            "/index", "/welcome", "/video", "/picture"};

void HTTPrequest::init() {
    method_ = path_ = version_ = body_ = "";
    state_ = REQUEST_LINE;
    header_.clear();
    post_.clear();
}

bool HTTPrequest::isKeepAlive() const {
    if(header_.count("Connection") == 1) {
        return header_.find("Connection")->second == "keep-alive" && version_ == "1.1";
    }
    return false;
}

bool HTTPrequest::parse(Buffer& buff) {
    const char CRLF[] = "\r\n";
    if(buff.readableBytes() <= 0) {
        return false;
    }
    while(buff.readableBytes() && state_ != FINISH) {
        // 在字符串中查找 "\r\n" ，得到的结果表示一行的末尾位置+1
        const char* lineEnd = std::search(buff.curReadPtr(), buff.curWritePtrConst(), CRLF, CRLF + 2);
        std::string line(buff.curReadPtr(), lineEnd);
        switch(state_)
        {
        case REQUEST_LINE:
            if(!parseRequestLine_(line)) {
                return false;
            }
            parsePath_();
            break;    
        case HEADERS:
            parseRequestHeader_(line);
            if(buff.readableBytes() <= 2) {     // 没有 body 数据
                state_ = FINISH;
            }
            break;
        case BODY:
            parseDataBody_(line);
            break;
        default:
            break;
        }
        if(lineEnd == buff.curWritePtr()) { break; }    // buffer末尾
        buff.updateReadPtrUntilEnd(lineEnd + 2);        // 移动 "\r\n"
    }
    return true;
}


// 组建成 HTML 后缀文件
void HTTPrequest::parsePath_() {
    if(path_ == "/") {
        path_ = "/index.html"; 
    }
    else {
        for(auto &item: DEFAULT_HTML) {
            if(item == path_) {
                path_ += ".html";
                break;
            }
        }
    }
}

bool HTTPrequest::parseRequestLine_(const std::string& line) {
    // 创建一个正则表达式对象，用于匹配 HTTP 请求行的格式
    // "^([^ ]*)" 从字符串的开头开始匹配并捕获所有非空格字符，直到遇到第一个空格或字符串结束
    // "([^ ]*)" 用于匹配零个或多个非空格字符
    std::regex patten("^([^ ]*) ([^ ]*) HTTP/([^ ]*)$");
    // 存储正则表达式的匹配结果。
    std::smatch subMatch;
    if(regex_match(line, subMatch, patten)) {   
        method_ = subMatch[1];
        path_ = subMatch[2];
        version_ = subMatch[3];
        state_ = HEADERS;
        return true;
    }
    return false;
}

void HTTPrequest::parseRequestHeader_(const std::string& line) {
    // 匹配形如 "key: value" 的字符串s
    std::regex patten("^([^:]*): ?(.*)$");
    std::smatch subMatch;
    if(regex_match(line, subMatch, patten)) {
        header_[subMatch[1]] = subMatch[2];
    }
    else {
        state_ = BODY;
    }
}

void HTTPrequest::parseDataBody_(const std::string& line) {
    body_ = line;
    parsePost_();
    state_ = FINISH;
}

int HTTPrequest::convertHex(char ch) {
    if(ch >= 'A' && ch <= 'F') return ch -'A' + 10;
    if(ch >= 'a' && ch <= 'f') return ch -'a' + 10;
    return ch;
}

void HTTPrequest::parsePost_() {
    if(method_ == "POST" && header_["Content-Type"] == "application/x-www-form-urlencoded") {
        if(body_.size() == 0) { return; }

        std::string key, value;
        int num = 0;
        int n = body_.size();
        int i = 0, j = 0;

        for(; i < n; i++)
        {
            char ch = body_[i];
            switch (ch) 
            {
                case '=':
                    key = body_.substr(j, i - j);
                    j = i + 1;
                    break;
                case '+':
                    body_[i] = ' ';
                    break;
                case '%':
                    num = convertHex(body_[i + 1]) * 16 + convertHex(body_[i + 2]);
                    body_[i + 2] = num % 10 + '0';
                    body_[i + 1] = num / 10 + '0';
                    i += 2;
                    break;
                case '&':
                    value = body_.substr(j, i - j);
                    j = i + 1;
                    post_[key] = value;
                    break;
                default:
                    break;
            }
        }
        assert(j <= i);
        if(post_.count(key) == 0 && j < i) 
        {
            value = body_.substr(j, i - j);
            post_[key] = value;
        }
    }   
}

std::string HTTPrequest::path() const{
    return path_;
}

std::string& HTTPrequest::path(){
    return path_;
}
std::string HTTPrequest::method() const {
    return method_;
}

std::string HTTPrequest::version() const {
    return version_;
}

std::string HTTPrequest::getPost(const std::string& key) const {
    assert(key != "");
    if(post_.count(key) == 1) {
        return post_.find(key)->second;
    }
    return "";
}

std::string HTTPrequest::getPost(const char* key) const {
    assert(key != nullptr);
    if(post_.count(key) == 1) {
        return post_.find(key)->second;
    }
    return "";
}