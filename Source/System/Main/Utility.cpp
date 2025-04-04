#include "Utility.h"

using namespace std;
using namespace DirectX;

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

std::wstring GetFileExtension(const std::wstring& filePath)
{
    //�t�@�C���p�X���ōŌ�̃h�b�g������
    size_t dotPos = filePath.find_last_of(L'.');
    size_t slashPos = filePath.find_last_of(L"/\\");

    //�h�b�g��������Ȃ��ꍇ�A�܂��͍Ō�ɂȂ��ꍇ�͋󕶎���Ԃ�
    //�܂��A�h�b�g���p�X��؂蕶���̌��ɂȂ��ꍇ������
    if (dotPos == std::wstring::npos || dotPos <= slashPos || dotPos == filePath.length() - 1) {
        return L"";
    }

    //�g���q��Ԃ�
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

std::u16string GetU16String(const std::string& str)
{
    //UTF-8��UTF-16�ɕϊ�
    int len = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, nullptr, 0);
    if (len == 0) return u"";

    std::u16string result(len - 1, u'\0'); //�I�[null���������� -1
    MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, reinterpret_cast<wchar_t*>(&result[0]), len);
    return result;
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

std::string GetStringFromU16(const std::u16string& str)
{
    // UTF-16��UTF-8�ɕϊ�
    int len = WideCharToMultiByte(CP_UTF8, 0, reinterpret_cast<const wchar_t*>(str.c_str()), -1, nullptr, 0, nullptr, nullptr);
    if (len == 0) return "";

    std::string result(len - 1, '\0');
    WideCharToMultiByte(CP_UTF8, 0, reinterpret_cast<const wchar_t*>(str.c_str()), -1, &result[0], len, nullptr, nullptr);
    return result;
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
    return GenerateIDFromFile(buffer.data(), buffer.size());
}

UINT GenerateIDFromFile(const char* data, size_t size)
{
    //XXH32�̏�Ԃ��쐬
    XXH32_state_t* state = XXH32_createState();
    if (state == nullptr) {
        printf("XXH32�̏������Ɏ��s���܂����B\n");
        return 0;
    }

    //�V�[�h�l��ݒ�
    XXH32_reset(state, 0);

    size_t step = size / HASH_SAMPLERATE;
    if (step == 0) step = 1;

    vector<char> sampledData;
    for (size_t i = 0; i < size; i += step) {
        sampledData.push_back(data[i]);
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

DirectX::XMVECTOR ExtractEulerAngles(const DirectX::XMMATRIX& matrix)
{
    float pitch, yaw, roll;

    //Y�������̉�]�iyaw�j
    yaw = asinf(-matrix.r[2].m128_f32[0]);

    if (cosf(yaw) > 0.0001f) {
        //X����Z�������̉�]�ipitch �� roll�j
        pitch = atan2f(matrix.r[2].m128_f32[1], matrix.r[2].m128_f32[2]);
        roll = atan2f(matrix.r[1].m128_f32[0], matrix.r[0].m128_f32[0]);
    }
    else {
        //Y���� 90�x�܂��� -90�x�̎��́AGimbal Lock�ɑΉ�
        pitch = atan2f(-matrix.r[0].m128_f32[2], matrix.r[1].m128_f32[1]);
        roll = 0.0f;
    }

    return XMVectorSet(pitch, yaw, roll, 0.0f); //���W�A���p�ŏo��
}
