#ifndef GD_CORE_MATH_MATRIX3
#define GD_CORE_MATH_MATRIX3

#include <GoddamnEngine/Include.hh>
#include <GoddamnEngine/Core/Math/Vector3.hh>
#include <GoddamnEngine/Core/Math/Matrix4x4.hh>

GD_NAMESPACE_BEGIN

	namespace Math
	{
		template<class T>
		class Matrix3t
		{
		private:
			Vector3t<T> data[3];

		public:
			Matrix3t<T>(const T init = 1.0f)
			{
				self->data[0] = Vector3t<T>(init, 0.0f, 0.0f);
				self->data[0] = Vector3t<T>(0.0f, init, 0.0f);
				self->data[0] = Vector3t<T>(0.0f, 0.0f, init);
			}

			/*Matrix3t<T>(const Matrix4x4t<T>& m)
			{
				self->data[0] = Vector3t<T>(m[0].x, m[0].y, m[0].z);
				self->data[1] = Vector3t<T>(m[1].x, m[1].y, m[1].z);
				self->data[2] = Vector3t<T>(m[2].x, m[2].y, m[2].z);
			}*/

			Matrix3t<T> Transpose() const
			{
				Matrix3t<T> result;
				result[0][0] = self->data[0][0];
				result[0][1] = self->data[1][0];
				result[0][2] = self->data[2][0];

				result[1][0] = self->data[0][1];
				result[1][1] = self->data[1][1];
				result[1][2] = self->data[2][1];

				result[2][0] = self->data[0][2];
				result[2][1] = self->data[1][2];
				result[2][2] = self->data[2][2];
				
				return result;
			}

			const	Vector3t<T>& operator[] (const size_t index) const { return self->data[index]; }
					Vector3t<T>& operator[] (const size_t index)		{ return self->data[index]; }
		};
	}

	typedef Math::Matrix3t<float> Matrix3;

GD_NAMESPACE_END

#endif