#include <iostream>
#include <string>

std::string base64Encode(unsigned char* data, int dataByte);             // base64 ����
std::string base64Decode(const char* data, int dataByte, int& outByte);  // base64 ����,dataByte:��������ݳ���,outByte:��������ݳ���