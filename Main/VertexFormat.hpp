#pragma once
#include <type_traits>

struct VertexFormatDesc
{
	uint32 components;
	uint32 componentSize;
	bool isFloat;
	bool isSigned;
};

typedef Vector<VertexFormatDesc> VertexFormatList;

template<typename... T>
struct VertexFormat
{
	static Vector<VertexFormatDesc> GetDescriptors()
	{
		Vector<VertexFormatDesc> res;
		AddFormats<sizeof...(T), T..., void>(res);
		return res;
	}
private:
	template<typename VT, size_t N>
	static void AddFormat(VertexFormatList& dsc, VectorMath::VectorBase<VT, N> dummy)
	{
		VertexFormatDesc d;
		d.components = N;
		d.componentSize = sizeof(VT);
		d.isFloat = std::is_floating_point<VT>::value;
		d.isSigned = std::is_signed<VT>::value;
		dsc.push_back(d);
	}

	template<size_t left, typename TFirst, typename... TRest>
	static void AddFormats(VertexFormatList& dsc)
	{
		AddFormat(dsc, TFirst());
		AddFormats<left - 1, TRest...>(dsc);
	}
	template<>
	static void AddFormats<0, void>(VertexFormatList& dsc)
	{
	}
};