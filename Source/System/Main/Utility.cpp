#include "Utility.h"

 std::string UTF8ToShiftJIS(std::string utf8Str)
{
    //Unicode�֕ϊ���̕����񒷂𓾂�
    int lenghtUnicode = MultiByteToWideChar(CP_UTF8, 0, utf8Str.c_str(), (int)(utf8Str.size()) + 1, NULL, 0);

    //�K�v�ȕ�����Unicode������̃o�b�t�@���m��
    wchar_t* bufUnicode = new wchar_t[lenghtUnicode];

    //UTF8����Unicode�֕ϊ�
    MultiByteToWideChar(CP_UTF8, 0, utf8Str.c_str(), (int)(utf8Str.size()) + 1, bufUnicode, lenghtUnicode);

    //ShiftJIS�֕ϊ���̕����񒷂𓾂�
    int lengthSJis = WideCharToMultiByte(CP_THREAD_ACP, 0, bufUnicode, -1, NULL, 0, NULL, NULL);

    //�K�v�ȕ�����ShiftJIS������̃o�b�t�@���m��
    char* bufShiftJis = new char[lengthSJis];

    //Unicode����ShiftJIS�֕ϊ�
    WideCharToMultiByte(CP_THREAD_ACP, 0, bufUnicode, lenghtUnicode + 1, bufShiftJis, lengthSJis, NULL, NULL);

    std::string strSJis(bufShiftJis);

    delete[] bufUnicode;
    delete[] bufShiftJis;

    return strSJis;
}

 uint32_t HashString(const std::string& str)
 {
     std::string lowerName = str;
     std::transform(lowerName.begin(), lowerName.end(), lowerName.begin(),
         [](unsigned char c) { return std::tolower(c); });
     std::vector<uint8_t> bytes(lowerName.begin(), lowerName.end());
     return FnvHash(bytes, true);
 }

 uint32_t FnvHash(const std::vector<uint8_t>& input, bool use32bits)
 {
     const uint32_t prime = 16777619;
     const uint32_t offset = 2166136261;
     const uint32_t mask = 1073741823;
     uint32_t hash = offset;
     for (uint8_t byte : input) {
         hash *= prime;
         hash ^= byte;
     }
     if (use32bits)
         return hash;
     else
         return (hash >> 30) ^ (hash & mask);
 }
