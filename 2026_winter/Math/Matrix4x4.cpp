#include "Matrix4x4.h"





//Vector3Ç∆çsóÒÇÃåvéZ
VECTOR operator*(VECTOR& vec, Matrix4x4 it)
{
	vec.x = vec.x * it.m00 + vec.y * it.m10 + vec.z * it.m20 + it.m30;
	vec.y = vec.x * it.m01 + vec.y * it.m11 + vec.z * it.m21 + it.m31;
	vec.z = vec.x * it.m02 + vec.y * it.m12 + vec.z * it.m22 + it.m32;
	return vec;

}
//



