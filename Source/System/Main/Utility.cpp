#include "Utility.h"

using namespace std;

string UTF8ToShiftJIS(string utf8Str)
{
    //Unicodeへ変換後の文字列長を得る
    int lenghtUnicode = MultiByteToWideChar(CP_UTF8, 0, utf8Str.c_str(), (int)(utf8Str.size()) + 1, NULL, 0);

    //必要な分だけUnicode文字列のバッファを確保
    wchar_t* bufUnicode = new wchar_t[lenghtUnicode];

    //UTF8からUnicodeへ変換
    MultiByteToWideChar(CP_UTF8, 0, utf8Str.c_str(), static_cast<int>((utf8Str.size())) + 1, bufUnicode, lenghtUnicode);

    //ShiftJISへ変換後の文字列長を得る
    int lengthSJis = WideCharToMultiByte(CP_THREAD_ACP, 0, bufUnicode, -1, NULL, 0, NULL, NULL);

    //必要な分だけShiftJIS文字列のバッファを確保
    char* bufShiftJis = new char[lengthSJis];

    //UnicodeからShiftJISへ変換
    WideCharToMultiByte(CP_THREAD_ACP, 0, bufUnicode, lenghtUnicode + 1, bufShiftJis, lengthSJis, NULL, NULL);

    string strSJis(bufShiftJis);

    delete[] bufUnicode;
    delete[] bufShiftJis;

    return strSJis;
}

//指定した文字列特有のIDを生成
//超極まれに別の文字列から同じIDが生成される (数億分の1)
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

//XMFLOAT3の線形補間
//引数 : 補間元のXMFLOAT3, 補間先のXMFLOAT3, 補間地点(0.0f～1.0f)
DirectX::XMFLOAT3 Lerp(const DirectX::XMFLOAT3& a, const DirectX::XMFLOAT3& b, float t)
{
    DirectX::XMFLOAT3 temp{};
    temp.x = lerp(a.x, b.x, t);
    temp.y = lerp(a.y, b.y, t);
    temp.z = lerp(a.z, b.z, t);

    return temp;
}

//XMFLOAT4の線形補間
//引数 : 補間元のXMFLOAT4, 補間先のXMFLOAT4, 補間地点(0.0f～1.0f)
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
    //ファイルパス内で最後のドットを検索
    size_t dotPos = filePath.find_last_of(".");

    //ドットが見つからない場合、または最後にない場合は空文字を返す
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
    //XXH32の状態を作成
    XXH32_state_t* state = XXH32_createState();
    if (state == nullptr) {
        printf("XXH32の初期化に失敗しました。\n");
        return 0;
    }

    //シード値を設定
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

    //RGBAの順番で32ビット値にエンコード
    return (alpha << 24) | (blue << 16) | (green << 8) | red;
}
