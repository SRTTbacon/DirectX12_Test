#include <DirectXMath.h>

class LightTransform
{
public:
	LightTransform();
	~LightTransform() = default;

	void SetPosition(float x, float y, float z);
	void SetPosition(DirectX::XMFLOAT3 position);
	const DirectX::XMFLOAT3& GetPosition() const;

	void SetRotation(float x, float y, float z);
	const DirectX::XMFLOAT3 GetRotation() const;
	const DirectX::XMFLOAT3 GetRotationRadians() const;
	const DirectX::XMFLOAT4 GetQuaternion() const;

	void SetScale(float x, float y, float z);
	const DirectX::XMFLOAT3& GetScale() const;

	void AddTranslation(float x, float y, float z);
	void AddRotationX(float x);
	void AddRotationY(float y);
	void AddRotationZ(float z);

	DirectX::XMFLOAT3 GetForward() const;
	DirectX::XMFLOAT3 GetRight() const;
	DirectX::XMFLOAT3 GetUp() const;

	DirectX::XMMATRIX GetTransformMatrix() const;
	DirectX::XMMATRIX GetViewMatrix() const;

private:
	DirectX::XMFLOAT3 mPosition;
	DirectX::XMFLOAT3 mRotation;
	DirectX::XMFLOAT3 mScale;
};
