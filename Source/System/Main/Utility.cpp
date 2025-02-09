#include "Utility.h"

using namespace std;

string UTF8ToShiftJIS(string utf8Str)
{
    //Unicode�֕ϊ���̕����񒷂𓾂�
    int lenghtUnicode = MultiByteToWideChar(CP_UTF8, 0, utf8Str.c_str(), (int)(utf8Str.size()) + 1, NULL, 0);

    //�K�v�ȕ�����Unicode������̃o�b�t�@���m��
    wchar_t* bufUnicode = new wchar_t[lenghtUnicode];

    //UTF8����Unicode�֕ϊ�
    MultiByteToWideChar(CP_UTF8, 0, utf8Str.c_str(), static_cast<int>((utf8Str.size())) + 1, bufUnicode, lenghtUnicode);

    //ShiftJIS�֕ϊ���̕����񒷂𓾂�
    int lengthSJis = WideCharToMultiByte(CP_THREAD_ACP, 0, bufUnicode, -1, NULL, 0, NULL, NULL);

    //�K�v�ȕ�����ShiftJIS������̃o�b�t�@���m��
    char* bufShiftJis = new char[lengthSJis];

    //Unicode����ShiftJIS�֕ϊ�
    WideCharToMultiByte(CP_THREAD_ACP, 0, bufUnicode, lenghtUnicode + 1, bufShiftJis, lengthSJis, NULL, NULL);

    string strSJis(bufShiftJis);

    delete[] bufUnicode;
    delete[] bufShiftJis;

    return strSJis;
}

//�w�肵����������L��ID�𐶐�
//���ɂ܂�ɕʂ̕����񂩂瓯��ID����������� (��������1)
UINT HashString(const string& str, bool use32bits)
{
    string lowerName = str;
    transform(lowerName.begin(), lowerName.end(), lowerName.begin(), [](unsigned char c) { return tolower(c); });
    vector<BYTE> bytes(lowerName.begin(), lowerName.end());

    const UINT prime = 16777619;
    const UINT offset = 2166136261;
    const UINT mask = 1073741823;
    UINT hash = offset;
    for (BYTE byte : bytes) {
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
    temp.x = lerp(a.x, b.x, t);
    temp.y = lerp(a.y, b.y, t);
    temp.z = lerp(a.z, b.z, t);

    return temp;
}

//XMFLOAT4�̐��`���
//���� : ��Ԍ���XMFLOAT4, ��Ԑ��XMFLOAT4, ��Ԓn�_(0.0f�`1.0f)
DirectX::XMFLOAT4 Lerp(const DirectX::XMFLOAT4& a, const DirectX::XMFLOAT4& b, float t)
{
    DirectX::XMFLOAT4 temp{};
    temp.x = lerp(a.x, b.x, t);
    temp.y = lerp(a.y, b.y, t);
    temp.z = lerp(a.z, b.z, t);
    temp.w = lerp(a.w, b.w, t);

    return temp;
}

string GetFileExtension(const string& filePath)
{
    //�t�@�C���p�X���ōŌ�̃h�b�g������
    size_t dotPos = filePath.find_last_of(".");

    //�h�b�g��������Ȃ��ꍇ�A�܂��͍Ō�ɂȂ��ꍇ�͋󕶎���Ԃ�
    if (dotPos == string::npos || dotPos == filePath.length() - 1) {
        return "";
    }

    return filePath.substr(dotPos);
}

wstring GetWideString(const string& str)
{
    int num1 = MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED | MB_ERR_INVALID_CHARS, str.c_str(), -1, nullptr, 0);

    wstring wstr;
    wstr.resize(num1);

    int num2 = MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED | MB_ERR_INVALID_CHARS, str.c_str(), -1, &wstr[0], num1);

    assert(num1 == num2);
    return wstr;
}

string GetNarrowString(const wstring& str)
{
    int num1 = WideCharToMultiByte(CP_ACP, 0, str.c_str(), -1, nullptr, 0, nullptr, nullptr);

    string narrowStr;
    narrowStr.resize(num1);

    int num2 = WideCharToMultiByte(CP_ACP, 0, str.c_str(), -1, &narrowStr[0], num1, nullptr, nullptr);

    assert(num1 == num2);
    return narrowStr;
}

vector<string> GetSprits(string& str, char delim)
{
    vector<string> elems;
    string item;
    for (char ch : str) {
        if (ch == delim) {
            if (!item.empty())
                elems.push_back(item);
            item.clear();
        }
        else {
            item += ch;
        }
    }
    if (!item.empty())
        elems.push_back(item);
    return elems;
}

UINT GenerateIDFromFile(const string& filePath)
{
    char buffer[_MAX_PATH];
    if (!_fullpath(buffer, filePath.c_str(), _MAX_PATH)) {
        return 0;
    }

    return HashString(buffer);
}

UINT GenerateIDFromFile(const wstring& filePath)
{
    return GenerateIDFromFile(GetNarrowString(filePath));
}

UINT GenerateIDFromFile(const vector<char>& buffer)
{
    //XXH32�̏�Ԃ��쐬
    XXH32_state_t* state = XXH32_createState();
    if (state == nullptr) {
        printf("XXH32�̏������Ɏ��s���܂����B\n");
        return 0;
    }

    //�V�[�h�l��ݒ�
    XXH32_reset(state, 0);

    size_t step = buffer.size() / HASH_SAMPLERATE;
    if (step == 0) step = 1;

    vector<char> sampledData;
    for (size_t i = 0; i < buffer.size(); i += step) {
        sampledData.push_back(buffer[i]);
    }

    return XXH32(sampledData.data(), sampledData.size(), 0);
}

UINT ColorToUINT(float r, float g, float b, float a)
{
    BYTE red = static_cast<BYTE>(clamp(r, 0.0f, 1.0f) * 255.0f);
    BYTE green = static_cast<BYTE>(clamp(g, 0.0f, 1.0f) * 255.0f);
    BYTE blue = static_cast<BYTE>(clamp(b, 0.0f, 1.0f) * 255.0f);
    BYTE alpha = static_cast<BYTE>(clamp(a, 0.0f, 1.0f) * 255.0f);

    //RGBA�̏��Ԃ�32�r�b�g�l�ɃG���R�[�h
    return (alpha << 24) | (blue << 16) | (green << 8) | red;
}
