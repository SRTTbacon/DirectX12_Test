#include "Utility.h"

 std::string UTF8ToShiftJIS(std::string utf8Str)
{
    //Unicodeへ変換後の文字列長を得る
    int lenghtUnicode = MultiByteToWideChar(CP_UTF8, 0, utf8Str.c_str(), (int)(utf8Str.size()) + 1, NULL, 0);

    //必要な分だけUnicode文字列のバッファを確保
    wchar_t* bufUnicode = new wchar_t[lenghtUnicode];

    //UTF8からUnicodeへ変換
    MultiByteToWideChar(CP_UTF8, 0, utf8Str.c_str(), (int)(utf8Str.size()) + 1, bufUnicode, lenghtUnicode);

    //ShiftJISへ変換後の文字列長を得る
    int lengthSJis = WideCharToMultiByte(CP_THREAD_ACP, 0, bufUnicode, -1, NULL, 0, NULL, NULL);

    //必要な分だけShiftJIS文字列のバッファを確保
    char* bufShiftJis = new char[lengthSJis];

    //UnicodeからShiftJISへ変換
    WideCharToMultiByte(CP_THREAD_ACP, 0, bufUnicode, lenghtUnicode + 1, bufShiftJis, lengthSJis, NULL, NULL);

    std::string strSJis(bufShiftJis);

    delete[] bufUnicode;
    delete[] bufShiftJis;

    return strSJis;
}

//指定した文字列特有のIDを生成
//超極まれに別の文字列から同じIDが生成される (数億分の1)
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

//XMFLOAT3の線形補間
//引数 : 補間元のXMFLOAT3, 補間先のXMFLOAT3, 補間地点(0.0f～1.0f)
 DirectX::XMFLOAT3 Lerp(const DirectX::XMFLOAT3& a, const DirectX::XMFLOAT3& b, float t)
 {
     DirectX::XMFLOAT3 temp{};
     temp.x = std::lerp(a.x, b.x, t);
     temp.y = std::lerp(a.y, b.y, t);
     temp.z = std::lerp(a.z, b.z, t);

     return temp;
 }

//XMFLOAT4の線形補間
//引数 : 補間元のXMFLOAT4, 補間先のXMFLOAT4, 補間地点(0.0f～1.0f)
 DirectX::XMFLOAT4 Lerp(const DirectX::XMFLOAT4& a, const DirectX::XMFLOAT4& b, float t)
 {
     DirectX::XMFLOAT4 temp{};
     temp.x = std::lerp(a.x, b.x, t);
     temp.y = std::lerp(a.y, b.y, t);
     temp.z = std::lerp(a.z, b.z, t);
     temp.w = std::lerp(a.w, b.w, t);

     return temp;
 }
