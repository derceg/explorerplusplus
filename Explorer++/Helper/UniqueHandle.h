#pragma once

#include <Windows.h>
#include "Macros.h"

/* This class is similar to std::unique_ptr,
and is based on the unique_handle class
outlined in http://msdn.microsoft.com/en-us/magazine/hh288076.aspx
and expanded on in http://visualstudiomagazine.com/articles/2013/09/01/get-a-handle-on-the-windows-api.aspx.

Using a unique_ptr with a HANDLE
is possible, but requires the
default invalid pointer value
(nullptr) to be changed (e.g.
to INVALID_HANDLE_VALUE).

This is possible, by designing
a deleter pointer type that satisfies
the NullablePointer requirements,
but results in code that is
rather hacky.

Having a dedicated class allows
the invalid value to be set
very easily, and removes the
ability to automatically
dereference the pointer,
which is never a valid operation
for a HANDLE. */
template <typename Traits>
class unique_handle
{
private:

	typedef typename Traits::pointer pointer;

	typedef void (unique_handle::*bool_type) () const;
	void no_bool_comparison() const {}

	DISALLOW_COPY_AND_ASSIGN(unique_handle);

	bool operator==(const unique_handle &);
	bool operator!=(const unique_handle &);

	void close()
	{
		if(*this)
		{
			Traits::close(m_value);
		}
	}

	pointer m_value;

public:

	explicit unique_handle(pointer value = Traits::invalid()) :
		m_value(value)
	{

	}

	unique_handle(unique_handle &&other) :
		m_value(other.release())
	{

	}

	~unique_handle()
	{
		close();
	}

	unique_handle & operator=(unique_handle &&other)
	{
		reset(other.release());
		return *this;
	}

	/* Either returns zero, or non-zero,
	which implicitly converts to bool, but no
	other type (since this is a void *). */
	operator bool_type() const
	{
		return m_value != Traits::invalid() ?
			&unique_handle::no_bool_comparison : nullptr;
	}

	pointer get() const
	{
		return m_value;
	}

	void reset(pointer value = Traits::invalid())
	{
		if(m_value != value)
		{
			close();
			m_value = value;
		}
	}

	pointer release()
	{
		pointer value = m_value;
		m_value = Traits::invalid();
		return value;
	}

	void swap(unique_handle &other)
	{
		std::swap(m_value, other.m_value);
	}
};