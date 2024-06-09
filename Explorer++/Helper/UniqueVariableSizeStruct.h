// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include <memory>
#include <type_traits>

template <typename T>
concept TrivialStruct = std::is_class_v<T> && std::is_trivial_v<T> && std::is_standard_layout_v<T>;

struct VariableSizeStructDeleter
{
	template <TrivialStruct T>
	void operator()(T *p) const
	{
		::operator delete(p);
	}
};

// Some Windows API structs have a variable size. For example, DLGTEMPLATEEX and FILEGROUPDESCRIPTOR
// are both variable size. This type and the function below exist to make it easier to work with
// such structs. Since the size of a struct of this type will vary, the struct has to be dynamically
// allocated and using a unique_ptr is beneficial for that purpose.
template <TrivialStruct T>
using UniqueVariableSizeStruct = std::unique_ptr<T, VariableSizeStructDeleter>;

template <TrivialStruct T>
UniqueVariableSizeStruct<T> MakeUniqueVariableSizeStruct(size_t size)
{
	// A variable size struct contains optional additional data past the end of the base struct. So,
	// the size being allocated here should be at least the size of the base struct.
	CHECK_GE(size, sizeof(T));

	return UniqueVariableSizeStruct<T>(static_cast<T *>(::operator new(size)));
}
