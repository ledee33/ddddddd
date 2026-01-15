#include "pch.h"
#include "Util.h"

namespace Util {
	Vector2 newvector2(float x, float y)
	{
		Vector2 newvec;
		newvec.x = x, newvec.y = y;
		return newvec;
	}
	Vector3 newvector3(float x, float y, float z)
	{
		Vector3 newvec;
		newvec.x = x, newvec.y = y, newvec.z = z;
		return newvec;
	}
}