#pragma once

#include "..\\..\\GameBase.h"

class TitleBack
{
public:
	TitleBack();
	~TitleBack();

	void Initialize();

	void Update();

	void SetMoveCamera(bool bMove);

private:
	static const std::string MODEL_CASTLE;
	static const std::string TEXTURE_CASTLE;
	static const std::string TEXTURE_SKYBOX;

	Model* m_pCastle;

	bool m_bCameraMove;
};
