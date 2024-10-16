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

//�w�肵����������L��ID�𐶐�
//���ɂ܂�ɕʂ̕����񂩂瓯��ID����������� (��������1)
 uint32_t HashString(const std::string& str, bool use32bits)
 {
     std::string lowerName = str;
     std::transform(lowerName.begin(), lowerName.end(), lowerName.begin(), [](unsigned char c) { return std::tolower(c); });
     std::vector<uint8_t> bytes(lowerName.begin(), lowerName.end());

     const uint32_t prime = 16777619;
     const uint32_t offset = 2166136261;
     const uint32_t mask = 1073741823;
     uint32_t hash = offset;
     for (uint8_t byte : bytes) {
         hash *= prime;
         hash ^= byte;
     }
     if (use32bits)
         return hash;
     else
         return (hash >> 30) ^ (hash & mask);
 }

//XMFLOAT3�̐��`���
//���� : ��Ԍ���XMFLOAT3, ��Ԑ��XMFLOAT3, ��Ԓn�_(0.0f�`1.0f)
 DirectX::XMFLOAT3 Lerp(const DirectX::XMFLOAT3& a, const DirectX::XMFLOAT3& b, float t)
 {
     DirectX::XMFLOAT3 temp{};
     temp.x = std::lerp(a.x, b.x, t);
     temp.y = std::lerp(a.y, b.y, t);
     temp.z = std::lerp(a.z, b.z, t);

     return temp;
 }

//XMFLOAT4�̐��`���
//���� : ��Ԍ���XMFLOAT4, ��Ԑ��XMFLOAT4, ��Ԓn�_(0.0f�`1.0f)
 DirectX::XMFLOAT4 Lerp(const DirectX::XMFLOAT4& a, const DirectX::XMFLOAT4& b, float t)
 {
     DirectX::XMFLOAT4 temp{};
     temp.x = std::lerp(a.x, b.x, t);
     temp.y = std::lerp(a.y, b.y, t);
     temp.z = std::lerp(a.z, b.z, t);
     temp.w = std::lerp(a.w, b.w, t);

     return temp;
 }
