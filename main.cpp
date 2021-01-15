#include <iostream>
#include <string>
#include "HttpRequest.h"
#include "Base64.h"

class Test {
public:
    void Send();
};

void Test::Send() {
    CHttpRequest myRequest(this);
    // GET方式（示例使用HTTP）
    std::string url = "http://172.18.194.169:80/vb.htm?setlongsesn=8701000559165";
    std::string sHeaderSend;     // 定义http头
    std::string sHeaderReceive;  // 返回头
    std::string sMessage;
    // base64就是校验，可能是sessionid、用户名和密码的base64编码等等，由服务端规定
    char buf[512];
    memset(buf, 0, 512);
    sprintf_s(buf, "admin:12345");
    std::string strBase64 = base64Encode((unsigned char*)buf, strlen(buf));

    int i = myRequest.SendRequest(false, false, url, sHeaderSend,  // http
        sHeaderReceive, sMessage, strBase64);

    // POST方式（示例使用HTTPS）
    url = "https://test.sn.bitdog.com/api/device/get-new-sn";
    sHeaderSend = "{\"requestNo\":\"1563521551\",\"liveTime\":30,\"param\":{\"device_type\":\"IPC_0\",\
                    \"mo\":\"4651515\",\"pi\":"",\"need_verify\":\"1\"}}";
    strBase64 = "Cookie: BitDogID=glq3ltuhvpf3ggdflhiokdngu0";
    // 开始发送请求
    i = myRequest.SendRequest(TRUE, TRUE, url, sHeaderSend,
        sHeaderReceive, sMessage, strBase64);
}

int main() {
    Test test;
    test.Send();
    return 0;
}