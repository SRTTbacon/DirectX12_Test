#pragma once
#include <wrl/client.h>
template<typename T> using ComPtr = Microsoft::WRL::ComPtr<T>;

#include "Engine\\Core\\Exception\\DxSystemException.h"