#include <DirectXMath.h>

class LightTransform
{
public:
	LightTransform();
	~LightTransform() = default;

	//�f�O���[�p�Ŋp�x���w��
	void SetRotation(float x, float y, float z);

	const DirectX::XMFLOAT3 GetRotation() const;
	const DirectX::XMFLOAT4 GetQuaternion() const;

	void AddTranslation(float x, float y, float z);
	void AddRotationX(float x);
	void AddRotationY(float y);
	void AddRotationZ(float z);

	DirectX::XMFLOAT3 GetForward() const;
	DirectX::XMFLOAT3 GetRight() const;
	DirectX::XMFLOAT3 GetUp() const;

	DirectX::XMMATRIX GetTransformMatrix() const;
	DirectX::XMMATRIX GetViewMatrix() const;

public:
	DirectX::XMFLOAT3 m_position;	//�ʒu
	DirectX::XMFLOAT3 m_rotation;	//���W�A���p
	DirectX::XMFLOAT3 m_scale;		//�X�P�[��

private:
};
