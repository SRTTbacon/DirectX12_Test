#pragma once

//.fbxファイルを独自フォーマット(.hcs)に変換
//必要最低限の情報しか保存しないためキャラクターを変換した場合、ロード時間1/4、メモリ使用量3/4ほどになるはず
//ファイル構造が単純なため、暗号化などが必要な場合コードを変更してください
//Zstandardを使用して圧縮をしています (ロード時間に支障がでない範囲で)

#include "..\\Character.h"
#include "..\\..\\Core\\BinaryFile\\BinaryCompression.h"

constexpr const float SAVE_SHAPE_DELTA = 0.001f;
constexpr const int COMPRESSION_LEVEL = 10;		//圧縮レベル (1～22) ロード時間と相談

enum ConvertResult
{
	Success,			//変換成功
	FileNotExist,		//fbxファイルが存在しない
	CantWriteFile,		//出力先に書き込めない
	Nullptr,			//モデルが読み込まれていない
	FbxFormatExeption	//fbxファイルが破損している
};

class ConvertFromFBX
{
public:
	//.fbxから読み込まれたキャラクターを.hcsファイルに変換
	//引数 :	1 - Character* 既にロードされているCharacterクラスのポインタ
	//	2 - std::string Characterのロードに使用したfbxファイル
	//	3 - std::string 出力先のファイルパス(拡張子はぶっちゃけ何でも良い)
	ConvertResult ConvertFromCharacter(Character* pCharacter, std::string fromFilePath, std::string toFilePath);

private:

	BinaryWriter bw;	//バイナリデータとしてファイルに書き込むためのクラス
	std::unordered_map<std::string, UINT> m_boneMapping;    //ボーン名からインデックスを取得

	void ProcessMesh(aiMesh* pMesh);
	void ProcessBone(Character* pCharacter);
	void ProcessHumanMesh(Character* pCharacter);
	void WriteCompression(std::vector<char>& originalBuffer);
};
