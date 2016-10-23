///////////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                                                                                           //
//  Copyright (c) 2012-2015, Jan de Graaf (jan@jlib.nl)                                                      //
//                                                                                                           //
//  Permission to use, copy, modify, and/or distribute this software for any purpose with or                 //
//  without fee is hereby granted, provided that the above copyright notice and this permission              //
//  notice appear in all copies.                                                                             //
//                                                                                                           //
//  THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS             //
//  SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL              //
//  THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES          //
//  WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE     //
//  OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.  //
//                                                                                                           //
///////////////////////////////////////////////////////////////////////////////////////////////////////////////

// Modified version of 
//	(http://jlib.nl/index.php?file=./code/include/jlib/matrix)

#pragma once
#include "Shared/Utility.hpp"
#include "Shared/Math.hpp"
#include "Shared/VectorMath.hpp"
#include <initializer_list>

template<size_t Dim, typename TFloatType> struct recursive_determinant;

/*
	Column-Major order templated matrix class
*/
template<size_t Width, size_t Height, typename TFloatType> class Matrix
{
public:
	static_assert(std::is_floating_point<TFloatType>::value, "Number type is not floating point");
	typedef TFloatType type;

	// Dont matrix with zero size
	static_assert(Width > 0 && Height > 0, "Matrix cannot have a width or height of zero");

	// Constructors
	Matrix()
	{
		this->SetIdentity();
	}
	explicit Matrix(bool create_identity)
	{
		if(create_identity)
			this->SetIdentity();
	}
	Matrix(const Matrix<Width, Height, TFloatType>& other)
	{
		memcpy(mat, other.mat, Size * sizeof(TFloatType));
	}
	Matrix(Matrix<Width, Height, TFloatType>&& other)
	{
		memcpy(mat, other.mat, Size * sizeof(TFloatType));
	}
	explicit Matrix(TFloatType value)
	{
		for(size_t i = 0; i < Size; i++)
			mat[i] = value;
	}
	explicit Matrix(const TFloatType* copy)
	{
		memcpy(mat, copy, Size * sizeof(TFloatType));
	}
	Matrix(std::initializer_list<TFloatType> list)
	{
		const size_t list_size = list.size() > Size ? Size : list.size();
		for(size_t i = 0; i < list_size; i++)
			mat[i] = *(list.begin() + i);
	}

	Matrix& operator=(const Matrix<Width, Height, TFloatType>& other)
	{
		memcpy(mat, other.mat, Size * sizeof(TFloatType));
		return *this;
	}
	Matrix& operator=(Matrix<Width, Height, TFloatType>&& other)
	{
		std::swap(mat, other.mat);
		return *this;
	}
	Matrix& operator=(std::initializer_list<TFloatType> list)
	{
		const size_t list_size = list.size() > Size ? Size : list.size();
		for(size_t i = 0; i < list_size; i++)
			mat[i] = *(list.begin() + i);
		return *this;
	}

	// Matrix operations
	void SetIdentity()
	{
		memset(mat, 0, Size * sizeof(TFloatType));
		for(size_t i = 0; i < Size; i += Height, i++)
			mat[i] = 1;
	}
	inline TFloatType Determinant() const
	{
		static_assert(Width == Height, "This operation requires a square matrix");

		return recursive_determinant<Width, TFloatType>::get(mat);
	}
	inline void Transpose()
	{
		static_assert(Width == Height, "This operation requires a square matrix");

		TFloatType new_matrix[Width * Height];

		for(size_t x = 0; x < Width; x++)
		{
			for(size_t y = 0; y < Height; y++)
				new_matrix[y * Width + x] = mat[x * Height + y];
		}

		memcpy(mat, new_matrix, Size * sizeof(TFloatType));
	}
	inline Matrix<Height, Width, TFloatType> Transposed() const
	{
		Matrix<Height, Width, TFloatType> result = *this;
		result.Transpose();
		return result;
	}
	inline void Invert()
	{
		// Source: https://chi3x10.wordpress.com/2008/05/28/calculate-matrix-inversion-in-c/

		static_assert(Width == Height, "This operation requires a square matrix");

		const TFloatType inv_det = TFloatType(1) / Determinant();

		TFloatType result[Size];
		TFloatType minor[(Width - 1) * (Width - 1)];
		for(size_t i = 0; i < Width; i++)
		{
			for(size_t j = 0; j < Width; j++)
			{
				size_t idx = 0;
				for(size_t x = 0; x < Width; x++)
				{
					if(x == i)
						continue;
					for(size_t y = 0; y < Width; y++)
					{
						if(y == j)
							continue;
						minor[idx++] = mat[x * Width + y];
					}
				}

				result[j * Width + i] = inv_det * ((Matrix<Width - 1, Width - 1, TFloatType>*)minor)->Determinant();
				if((j + i) % 2 == 1)
					result[j * Width + i] = -result[j * Width + i];
			}
		}

		memcpy(mat, result, Size * sizeof(TFloatType));
	}
	inline Matrix<Width, Height, TFloatType> Inverted() const
	{
		Matrix<Width, Height, TFloatType> result = *this;
		result.Invert();
		return result;
	}

	// Arithmetic operators
	inline Matrix<Width, Height, TFloatType>& operator*=(const Matrix<Width, Height, TFloatType>& other)
	{
		static_assert(Width == Height, "This operation requires a square matrix");

		TFloatType new_matrix[Width * Height];

		size_t index = 0;
		for(size_t x = 0; x < Width; x++)
		{
			for(size_t y = 0; y < Width; y++)
			{
				new_matrix[index] = 0;
				const size_t left = y;
				const size_t top = x * Width;
				for(size_t i = 0; i < Width; i++)
					new_matrix[index] += mat[left + Width * i] * other.mat[top + i];
				index++;
			}
		}

		memcpy(mat, new_matrix, Size * sizeof(TFloatType));
		return *this;
	}
	inline Matrix<Width, Height, TFloatType>& operator+=(const Matrix<Width, Height, TFloatType>& other)
	{
		for(size_t i = 0; i < Size; i++)
			mat[i] += other.mat[i];
		return *this;
	}
	inline Matrix<Width, Height, TFloatType>& operator-=(const Matrix<Width, Height, TFloatType>& other)
	{
		for(size_t i = 0; i < Size; i++)
			mat[i] -= other.mat[i];
		return *this;
	}
	inline Matrix<Width, Height, TFloatType>& operator*=(TFloatType value)
	{
		for(size_t i = 0; i < Size; i++)
			mat[i] *= value;
		return *this;
	}
	inline Matrix<Width, Height, TFloatType>& operator/=(TFloatType value)
	{
		for(size_t i = 0; i < Size; i++)
			mat[i] /= value;
		return *this;
	}

	template<size_t owidth, size_t oheight> inline Matrix<owidth, Height, TFloatType> operator*(const Matrix<owidth, oheight, TFloatType>& other) const
	{
		static_assert(Width == oheight, "The number of columns on the left matrix is not the same as the number of rows of the right matrix");

		Matrix<owidth, Height, TFloatType> result;

		size_t index = 0;
		for(size_t x = 0; x < owidth; x++)
		{
			for(size_t y = 0; y < Height; y++)
			{
				result[index] = 0;
				size_t left = y;
				size_t top = x * Width;
				for(size_t i = 0; i < Width; i++)
				{
					result[index] += mat[left] * other.mat[top];
					left += Height, top++;
				}
				index++;
			}
		}

		return result;
	}
	inline Matrix<Width, Height, TFloatType> operator+(const Matrix<Width, Height, TFloatType>& other) const
	{
		Matrix<Width, Height, TFloatType> result = *this;
		result += other;
		return result;
	}
	inline Matrix<Width, Height, TFloatType> operator-(const Matrix<Width, Height, TFloatType>& other) const
	{
		Matrix<Width, Height, TFloatType> result = *this;
		result -= other;
		return result;
	}
	inline Matrix<Width, Height, TFloatType> operator*(TFloatType value) const
	{
		Matrix<Width, Height, TFloatType> result = *this;
		result *= value;
		return result;
	}
	inline Matrix<Width, Height, TFloatType> operator/(TFloatType value) const
	{
		Matrix<Width, Height, TFloatType> result = *this;
		result /= value;
		return result;
	}

	// Index operators
	inline TFloatType& operator[](const size_t index)
	{
		return mat[index];
	}
	inline const TFloatType& operator[](const size_t index) const
	{
		return mat[index];
	}

	inline TFloatType& operator()(size_t column, size_t row)
	{
		return mat[column * Height + row];
	}
	inline const TFloatType& operator()(size_t column, size_t row) const
	{
		return mat[column * Height + row];
	}

	// Comparison operators
	inline bool operator==(const Matrix<Width, Height, TFloatType>& other) const
	{
		const size_t size = Width * Height;
		for(size_t i = 0; i < size; i++)
		{
			if(mat[i] != other.mat[i])
				return false;
		}
		return true;
	}
	inline bool operator!=(const Matrix<Width, Height, TFloatType>& other) const
	{
		const size_t size = Width * Height;
		for(size_t i = 0; i < size; i++)
		{
			if(mat[i] != other.mat[i])
				return true;
		}
		return false;
	}

	// Compile time constants
	static const size_t Columns = Width;
	static const size_t Rows = Height;
	static const size_t Size = Width * Height;

	inline TFloatType* GetData()
	{
		return mat;
	}
	inline const TFloatType* GetData() const
	{
		return mat;
	}

	// Matrix data
	TFloatType mat[Width * Height];
};

template<size_t Dim, typename TFloatType> struct recursive_determinant
{
	static float get(const TFloatType* mat_ptr)
	{
		TFloatType tmp_mat[(Dim - 1) * (Dim - 1)];
		TFloatType det = 0;
		TFloatType mod = 1;
		for(size_t i = 0; i < Dim; i++)
		{
			if(mat_ptr[i * Dim] != 0)
			{
				size_t idx = 0;
				for(size_t x = 0; x < Dim; x++)
				{
					if(x == i)
						continue;
					for(size_t y = 1; y < Dim; y++)
						tmp_mat[idx++] = mat_ptr[x * Dim + y];
				}
				det += mat_ptr[i * Dim] * recursive_determinant<Dim - 1, TFloatType>::get(tmp_mat) * mod;
			}
			mod *= TFloatType(-1.0);
		}
		return det;
	}
};
template<typename TFloatType> struct recursive_determinant<2, TFloatType>
{
	static float get(const TFloatType* mat_ptr)
	{
		return ((mat_ptr[0] * mat_ptr[3]) - (mat_ptr[1] * mat_ptr[2]));
	}
};
template<typename TFloatType> struct recursive_determinant<1, TFloatType>
{
	static float get(const TFloatType* mat_ptr)
	{
		return mat_ptr[0];
	}
};
template<typename TFloatType> struct recursive_determinant<0, TFloatType>
{
	static float get(const TFloatType* mat_ptr)
	{
		return 0;
	}
};

typedef Matrix<3, 3, float> Matrix3x3;
typedef Matrix<4, 4, float> Matrix4x4;