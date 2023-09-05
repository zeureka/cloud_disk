#ifndef HTTPSTATUS_H
#define HTTPSTATUS_H

#include <iostream>
#include <string>
#include <vector>
#include <map>

// Response status code
#define RES_SUCCESS                         "200"   // success

#define RES_ACCOUNT_EXIST                   "901"   // account exist
#define RES_FILE_EXIST                      "902"   // file exist
#define RES_ALREADY_SHARE_FILE              "903"   // someone else has already shared this file

#define RES_ENROLL_FAILED                   "001"   // enroll has failed
#define RES_LOGIN_FAILED                    "002"   // login has failed
#define RES_FAST_UPLOAD_FAILED              "003"   // fast transmission failed
#define RES_UPLOAD_FAILED                   "004"   // upload failed
#define RES_DOWNLOAD_FAILED                 "005"   // download failed
#define RES_SHARE_FAILED                    "006"   // sharing failed
#define RES_TOKEN_FAILED                    "007"   // token validation failed
#define RES_DELETE_FAILED                   "008"   // delete failed
#define RES_DOWNLOAD_FILE_DOWNLOADS_FAILED  "009"   // download file 'downloads' field processing failed
#define RES_CANCEL_FAILED                   "010"   // cancel failed
#define RES_DEALSHARE_SAVE_FAILED           "011"   // cloud drive save
#define RES_GET_SHARE_FILE_LIST_FAILED      "012"   // failed to obtain the list of shared files
#define RES_GET_ACCOUNT_FILE_LIST_FAILED    "013"   // failed to obtain the list of account files

// Response return the result
struct Response {
    std::string code;   // status code
    std::string msg;    // message
};

/**
 * @brief http status code
 */
class HttpStatus {
private:
    HttpStatus();
    ~HttpStatus() = default;
    HttpStatus(const HttpStatus&) = delete;
    HttpStatus& operator=(const HttpStatus&) = delete;

public:
    // get response information
    Response* getResponse(std::string code);
    static HttpStatus* getInstance();

private:
    std::map<std::string, Response*> m_responseMap;
};



#endif // HTTPSTATUS_H
