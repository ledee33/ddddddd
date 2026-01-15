#pragma once

namespace Util {
	class Vector2 {
	public:
		float x;
		float y;
	};

	class Vector3 {
	public:
		float x;
		float y;
		float z;
	};

	class WtsInfo {
		Vector2 screenpos;
		bool onscreen;
	};

	Vector2 newvector2(float x, float y);
	Vector3 newvector3(float x, float y, float z);

	WtsInfo world2screen(Vector3 objectpos);
}