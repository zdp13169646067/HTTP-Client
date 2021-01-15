#include <iostream>
#include <string>

std::string base64Encode(unsigned char* data, int dataByte);             // base64 编码
std::string base64Decode(const char* data, int dataByte, int& outByte);  // base64 解码,dataByte:输入的数据长度,outByte:输出的数据长度