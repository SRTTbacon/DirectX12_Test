﻿//*****************************************************************************
//
// DxSystem例外
//
// DxSystemException.cpp
//
// K_Yamaoka
//
// 2016/11/18
//
//*****************************************************************************

#include "DxSystemException.h"

#include <cassert>

//=============================================================================
// コンストラクタ
//=============================================================================
DxSystemException::DxSystemException(const OriginalMessage message)
	: m_message(message)
{

}

//=============================================================================
// オリジナルエラーメッセージの表示
// 引　数：const OutputTarget　　エラーメッセージの出力先（デフォルトはメッセージボックス）
//=============================================================================
void DxSystemException::ShowOriginalMessage(const OutputTarget outputTarget)
{
	switch (outputTarget) {

	case OUT_MESSAGEBOX:
		MessageBox(FindWindow(WINDOW_CLASS_NAME, nullptr), SearchOriginalMessage(m_message), WINDOW_CLASS_NAME, MB_OK);
		break;

	case OUT_OUTPUTWINDOW:
	{
		std::wstring strMessage;
		strMessage = SearchOriginalMessage(m_message);
		strMessage += L"\n";
		OutputDebugString(strMessage.c_str());
	}
	break;

	default:
		assert(0);
	}
}




//wmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmw
// private関数
//wmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmwmw


//-----------------------------------------------------------------------------
// オリジナルエラーメッセージの取得
// 戻り値：エラーメッセージ文字列
// 引　数：const OriginalMessage オリジナルエラーメッセージ
//-----------------------------------------------------------------------------
const wchar_t*  DxSystemException::SearchOriginalMessage(const OriginalMessage message)
{
	static const wchar_t* OriginalErrorMessage[] = {
		L"予期せぬエラーが発生",
		L"ウィンドウクラスの初期化に失敗",
		L"ゲームクラスの初期化に失敗",
		L"Ｄｉｒｅｃｔ３Ｄ本体の作成に失敗",
		L"デバイスの作成に失敗",
		L"スプライトの作成に失敗",
		L"テクスチャ管理クラスの領域確保に失敗",
		L"テクスチャ管理クラスの初期化に失敗",
		L"画面のクリアに失敗",
		L"描画の開始に失敗",
		L"アルファブレンドの設定に失敗",
		L"転送元アルファの設定に失敗",
		L"転送先アルファの設定に失敗",
		L"Ｚバッファへの書き込み禁止に失敗",
		L"スプライトの描画開始に失敗",
		L"スプライトの描画終了に失敗",
		L"Ｚバッファへの書き込み許可に失敗",
		L"アルファブレンドの無効化に失敗",
		L"描画の終了に失敗",
		L"フロントバッファへの転送に失敗",
		L"バックバッファの取得に失敗",
		L"バックバッファのロック",
		L"テクスチャの読み込みに失敗",
		L"テクスチャクラスの領域確保に失敗",
		//L"テクスチャの登録に失敗",
		L"デバイスの能力取得に失敗",
		L"テクスチャの幅が最大値を超えています",
		L"テクスチャの高さが最大値を超えています",
		L"テクスチャの幅と高さが違います",
		L"テクスチャのサイズが２の累乗ではありません",
		//L"テクスチャの解放に失敗",
		L"コマンドライン引数は使用できません",
		L"ウィンドウの登録に失敗",
		L"ウィンドウの作成に失敗",
		L"テクスチャが見つかりません",
		L"スプライトの描画に失敗",
		//L"フォントの再作成に失敗",
		L"フォント管理クラスの領域確保に失敗",
		L"フォントの作成に失敗",
		L"フォントの領域確保に失敗",
		//L"フォントの登録に失敗",
		//L"フォントの解放に失敗",
		L"文字列の表示に失敗",
		L"ＦＰＳ管理クラスの領域確保に失敗",
		L"フォントが見つかりません",
		//L"メッセージクラスの領域確保に失敗",
		//L"メッセージ用ファイルが見つかりません",
		L"オフスクリーンサーフェス作成に失敗",
		L"フロントバッファの取得に失敗",
		L"スクリーンショットの保存に失敗",
		L"ＢＧＭファイル名の領域取得に失敗",
		L"グラフビルダーの作成に失敗",
		L"メディアコントロールの作成に失敗",
		L"メディアイベントの作成に失敗",
		L"メディアシーキングの作成に失敗",
		L"フィルタグラフの作成に失敗",
		L"ベーシックオーディオの作成に失敗",
		L"ＢＧＭの再生に失敗",
		L"ＢＧＭの停止に失敗",
		L"ＢＧＭ管理クラスの領域確保に失敗",
		L"ＣＯＭの初期化に失敗",
		L"ＢＧＭ管理クラスの初期化に失敗",
		//L"クラスの初期化が行われていません",
		L"パフォーマンスの作成に失敗",
		L"オーディオの初期化に失敗",
		L"ローダーの作成に失敗",
		L"カレントディレクトリの取得に失敗",
		L"ローダーへのディレクトリ登録に失敗",
		L"ＳＥのロードに失敗",
		L"バンドのダウンロードに失敗",
		L"ＳＥクラスの領域確保に失敗",
		L"DirectSoundの作成に失敗",
		L"協調レベルの設定に失敗",
		L"スピーカーの設定に失敗",
		L"プライマリバッファの作成に失敗",
		L"Weveファイル名用領域確保に失敗",
		L"Weveファイルのオープンに失敗",
		L"データチャンクへの進入に失敗",
		L"セカンダリバッファの作成に失敗",
		L"ＳＥの再生に失敗",
		L"ＳＥの停止に失敗",
		L"ＳＥ管理クラスの領域確保に失敗",
		L"ＳＥ管理クラスの初期化に失敗",
		L"表示したい桁数より数値が大きいです",
		L"桁別数値格納領域の確保に失敗",
		L"バウンディングボックス用頂点バッファの作成に失敗",
		L"バウンディングボックス用インデックスバッファの作成に失敗",
		L"バウンディングボックス用頂点バッファのロックに失敗",
		L"バウンディングボックス用インデックスバッファのロックに失敗",
		L"バウンディングスフィア用頂点バッファの作成に失敗",
		L"バウンディングスフィア用インデックスバッファの作成に失敗",
		L"バウンディングスフィア用頂点バッファのロックに失敗",
		L"バウンディングスフィア用インデックスバッファのロックに失敗",
		L"アンビエントライトのデバイス登録に失敗",
		//L"ディレクショナルライトのデバイス登録に失敗",
		L"ディレクショナルライトの消灯に失敗",
		L"ディレクショナルライトの点灯に失敗",
		L"ポイントライトのデバイス登録に失敗",
		L"ポイントライトの消灯に失敗",
		L"ポイントライトの点灯に失敗",
		L"スポットライトのデバイス登録に失敗",
		L"スポットライトの消灯に失敗",
		L"スポットライトの点灯に失敗",
		L"カメラのデバイス登録に失敗",
		L"プロジェクションのデバイス登録に失敗",
		L"ビューポートのデバイス登録に失敗",
		L"プリミティブ用メッシュの作成に失敗",
		L"プリミティブ用頂点バッファのロックに失敗",
		L"プリミティブ用インデックスバッファのロックに失敗",
		L"球体の作成に失敗",
		L"球体の法線計算に失敗",
		L"球体のクローン作成に失敗",
		L"プリミティブ用テクスチャの読み込みに失敗",
		L"パーティクル用テクスチャの読み込みに失敗",
		L"Ｘファイルのロードに失敗",
		L"メッシュのボーンマトリックス設定に失敗",
		//L"メッシュのアニメーションセット設定に失敗",
		L"ルートフレームの行列取得に失敗",
		L"指定したフレームの行列取得に失敗",
		L"隣接データ構造体配列の領域確保に失敗",
		L"隣接データ配列の領域確保に失敗",
		L"隣接データの生成に失敗",
		L"メッシュの頂点バッファロックに失敗",
		L"メッシュのインデックスバッファロックに失敗",
		L"ビルボード用メッシュの作成に失敗",
		L"ビルボードの頂点バッファロックに失敗",
		L"ビルボードのインデックスバッファロックに失敗",
		L"ビルボード用テクスチャの読み込みに失敗",
		L"フレームへのボーンマトリックス設定に失敗",
		L"兄弟フレームへのボーンマトリックス設定に失敗",
		L"子フレームへのボーンマトリックス設定に失敗",
		L"変換行列の設定に失敗",
		L"頂点ブレンドの設定に失敗",
		L"マテリアルの設定に失敗",
		L"テクスチャの設定に失敗",
		L"サブセットの描画に失敗",
		L"ソフトウェア頂点処理の開始に失敗",
		L"ソフトウェア頂点処理の終了に失敗",
		L"頂点ブレンドの終了に失敗",
		L"拡張フレームの領域確保に失敗",
		L"フレーム名のコピーに失敗",
		L"ノーマルタイプのメッシュのみ利用できます",
		L"ＦＶＦが設定されていないメッシュは利用できません",
		L"拡張メッシュコンテナの領域確保に失敗",
		L"コンテナ名のコピーに失敗",
		L"メッシュのクローンに失敗",
		L"マテリアル、テクスチャ、隣接データの領域確保に失敗",
		L"ボーンオフセットマトリックスの領域確保に失敗",
		L"スキンメッシュの作成に失敗",
		L"モデル管理クラスの領域確保に失敗",
		L"モデルベースクラスの領域の確保に失敗",
		L"階層メッシュ読み込み用クラスの領域の確保に失敗",
		L"フレームクラスの領域の確保に失敗",
		L"アニメーションセットの領域の確保に失敗",
		L"入力管理クラスの領域確保に失敗",
		L"入力管理クラスの初期化に失敗",
		L"DirectInputの作成に失敗",
		L"キーボードデバイスの初期化に失敗",
		L"マウスデバイスの初期化に失敗",
		L"ゲームパッドデバイスの初期化に失敗",
		L"エンジンクラスの領域確保に失敗",
		L"エンジンクラスの初期化に失敗",
		L"newによる領域確保に失敗",
		L"線描画オブジェクトの作成に失敗",
		L"線描画オブジェクトの領域確保に失敗",
		L"ファイルのオープンに失敗",
		L"ビルボードシェーダーの読み込みに失敗",
		L"ビルボードの頂点宣言に失敗",
		L"プリミティブシェーダーの読み込みに失敗",
		L"プリミティブの頂点宣言に失敗",
		L"パーティクルシェーダーの読み込みに失敗",
		L"パーティクルの頂点宣言に失敗",
		L"Ｘファイルシェーダー（スキンなし）の読み込みに失敗",
		L"Ｘファイルシェーダーの読み込みに失敗",
		L"Ｘファイルの頂点宣言に失敗",
		L"バウンディングボックスシェーダーの読み込みに失敗",
		L"バウンディングボックスの頂点宣言に失敗",
		L"バウンディングスフィアシェーダーの読み込みに失敗",
		L"バウンディングスフィアの頂点宣言に失敗",
		L"ダイアログの作成に失敗",
		L"ソケットの初期化に失敗",
		L"ソケットの作成に失敗",
		L"サーバーへの接続に失敗",
		L"データ受信スレッドの開始に失敗",
		L"サーバーに接続拒否されました",
		L"ダイアログの作成に失敗",
		L"ソケットの初期化に失敗",
		L"ソケットの作成に失敗",
		L"クライアント受付スレッドの開始に失敗",
		L"データ受信スレッドの開始に失敗",
		L"サブシーンからは元のシーンか次のサブシーン以外への遷移は禁止されています",
		L"スクリーンの高さを超えるウインドウは作成できません",
		L"",
	};

	return OriginalErrorMessage[message];
}

