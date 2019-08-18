#ifndef _Camara_
#define _Camara_

class Camara
{
private:
	float aspect = 0;
public:
	D3DXVECTOR3 tpsPosition;
	D3DXVECTOR3 Position;
	D3DXVECTOR3 Target;
	D3DXVECTOR3 Up;

	D3DXMATRIX View;
	D3DXMATRIX Projection;

	D3DXVECTOR3 DefaultForward;
	D3DXVECTOR3 DefaultRight;
	D3DXVECTOR3 Forward;
	D3DXVECTOR3 Right;

	D3DXMATRIX RotationMatrix;
	D3DXMATRIX groundWorld;

	float Yaw;
	float Pitch;
	float Roll;

	D3DXVECTOR3 OrbitAngle = { 0, 0, 0 };
	D3DXVECTOR3 OrbitView = { 0, 0, 0 };
	D3DXVECTOR3 OrbitTarget = { 0, 0, 0 };

	float distance;

	bool isTPS;

	Camara(D3DXVECTOR3 Position, D3DXVECTOR3 Target, D3DXVECTOR3 Up, int Width, int Height)
	{
		this->Position = Position;
		this->Target = Target;
		this->Up = Up;

		//Set the View matrix 
		D3DXMatrixLookAtLH(&View, &Position, &Target, &Up);

		aspect = (float)Width / Height;

		//Set the Projection matrix
		D3DXMatrixPerspectiveFovLH(&Projection, 
			D3DXToRadian(90), aspect, 
			0.1f, 5050.0f);

		DefaultForward = D3DXVECTOR3(0.0f,0.0f,1.0f);
		DefaultRight = D3DXVECTOR3(1.0f,0.0f,0.0f);
		Forward = D3DXVECTOR3(0.0f,0.0f,1.0f);
		Right = D3DXVECTOR3(1.0f,0.0f,0.0f);

		Yaw = 0.0f;
		Pitch = 0.0f;
		Roll = 0.0f;
		distance = 15.0f;
	}
	void setFov(float FOV) {
		D3DXMatrixPerspectiveFovLH(&Projection,
			D3DXToRadian(FOV), aspect,
			0.1f, 5050.0f);
	}

	void setDistance(float distance)
	{
		this->distance = distance;
	}

	void UpdateCamera(float moveBackForward, float moveLeftRight, float yaw, float pitch)
	{
		Yaw += yaw;
		Pitch += pitch;


		D3DXMatrixRotationYawPitchRoll(&RotationMatrix, Yaw, Pitch, 0);
		D3DXVec3TransformCoord(&Target, &DefaultForward, &RotationMatrix);
		D3DXVec3Normalize(&Target, &Target);

		D3DXMATRIX RotateYTempMatrix;
		D3DXMatrixRotationY(&RotateYTempMatrix, Yaw);

		D3DXVec3TransformCoord(&Right, &DefaultRight, &RotateYTempMatrix);
		D3DXVec3TransformCoord(&Up, &Up, &RotateYTempMatrix);
		D3DXVec3TransformCoord(&Forward, &DefaultForward, &RotateYTempMatrix);

		Position += moveLeftRight*Right;
		Position += moveBackForward*Forward;

		Target = Position + Target;

		D3DXMatrixLookAtLH(&View, &Position, &Target, &Up);
	}
	D3DXVECTOR3 updateFoward(){
		D3DXMatrixRotationYawPitchRoll(&RotationMatrix, Yaw, Pitch, Roll);
		D3DXVec3TransformCoord(&Forward, &DefaultForward, &RotationMatrix);
		return Forward;
	}
	void UpdateCameraTPS(float moveBackForward, float moveLeftRight, float yaw, float pitch)
	{
		//...<----7##		(OrbitView)
		//....#../....#
		//....#./.....#		(OrbitAngle)
		//..<[SHIP].#		 (Position)

		Yaw += yaw;
		Pitch += pitch;

		D3DXMatrixRotationYawPitchRoll(&RotationMatrix, Yaw, Pitch, Roll);

		D3DXVec3TransformCoord(&Right, &DefaultRight, &RotationMatrix);
		D3DXVec3TransformCoord(&Forward, &DefaultForward, &RotationMatrix);
		D3DXVec3Cross(&Up, &Forward, &Right);

		Position += moveLeftRight*Right;
		Position += moveBackForward*Forward;

		D3DXMATRIX angle;
		D3DXMatrixRotationYawPitchRoll(&angle, D3DXToRadian(OrbitAngle.x), D3DXToRadian(OrbitAngle.y), D3DXToRadian(OrbitAngle.z));
		angle *= RotationMatrix;

		tpsPosition *= 0;
		D3DXVec3TransformCoord(&tpsPosition, &DefaultForward, &angle);
		D3DXVec3Normalize(&tpsPosition, &tpsPosition);

		tpsPosition = Position + tpsPosition*-distance;

		D3DXMATRIX TargetAngle;
		D3DXMatrixRotationYawPitchRoll(&TargetAngle, D3DXToRadian(OrbitView.x), D3DXToRadian(OrbitView.y), D3DXToRadian(OrbitView.z));
		TargetAngle *= angle;
		D3DXVec3TransformCoord(&OrbitTarget, &DefaultForward, &TargetAngle);
		D3DXVec3Normalize(&OrbitTarget, &OrbitTarget);

		Target = tpsPosition + OrbitTarget;

		D3DXMatrixLookAtLH(&View, &tpsPosition, &Target, &Up);
	}
};

#endif