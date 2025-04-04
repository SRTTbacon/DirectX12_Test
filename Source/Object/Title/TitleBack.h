#pragma once

#include "..\\..\\GameBase.h"

class TitleBack
{
public:
	TitleBack();

	void Initialize();

	void Update();

	void SetMoveCamera(bool bMove);

private:
	Model* m_pCastle;

	bool m_bCameraMove;
};
