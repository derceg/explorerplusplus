// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "pch.h"
#include "../Helper/WeakPtrFactory.h"
#include "GTestHelper.h"
#include "../Helper/WeakPtr.h"
#include <gtest/gtest.h>
#include <memory>
#include <thread>

namespace
{

class WeakFactoryOwner
{
public:
	WeakFactoryOwner() : m_weakPtrFactory(this)
	{
	}

	WeakPtr<WeakFactoryOwner> GetWeakPtr()
	{
		return m_weakPtrFactory.GetWeakPtr();
	}

	void InvalidateWeakPtrs()
	{
		m_weakPtrFactory.InvalidateWeakPtrs();
	}

	void IncrementCounter()
	{
		m_counter++;
	}

private:
	int m_counter = 0;
	WeakPtrFactory<WeakFactoryOwner> m_weakPtrFactory;
};

}

TEST(WeakPtrTest, DefaultConstruction)
{
	WeakPtr<WeakFactoryOwner> weakPtr;
	EXPECT_FALSE(weakPtr);
	EXPECT_EQ(weakPtr.Get(), nullptr);
}

TEST(WeakPtrFactoryTest, WeakPtrGet)
{
	auto weakFactoryOwner = std::make_unique<WeakFactoryOwner>();
	auto weakPtr = weakFactoryOwner->GetWeakPtr();
	EXPECT_EQ(weakPtr.Get(), weakFactoryOwner.get());

	weakFactoryOwner.reset();
	EXPECT_EQ(weakPtr.Get(), nullptr);
}

TEST(WeakPtrFactoryTest, WeakPtrBoolConversion)
{
	auto weakFactoryOwner = std::make_unique<WeakFactoryOwner>();
	auto weakPtr = weakFactoryOwner->GetWeakPtr();
	EXPECT_TRUE(weakPtr);

	weakFactoryOwner.reset();
	EXPECT_FALSE(weakPtr);
}

TEST(WeakPtrFactoryTest, WeakPtrDereference)
{
	auto weakFactoryOwner = std::make_unique<WeakFactoryOwner>();
	auto weakPtr = weakFactoryOwner->GetWeakPtr();

	// These calls should succeed, since the object is still live.
	weakPtr->IncrementCounter();
	(*weakPtr).IncrementCounter();
}

TEST(WeakPtrFactoryTest, WeakPtrReset)
{
	auto weakFactoryOwner = std::make_unique<WeakFactoryOwner>();
	auto weakPtr = weakFactoryOwner->GetWeakPtr();
	EXPECT_EQ(weakPtr.Get(), weakFactoryOwner.get());

	weakPtr.Reset();
	EXPECT_EQ(weakPtr.Get(), nullptr);
}

TEST(WeakPtrFactoryTest, WeakPtrCopy)
{
	auto weakFactoryOwner = std::make_unique<WeakFactoryOwner>();
	auto weakPtr = weakFactoryOwner->GetWeakPtr();
	auto weakPtrCopy = weakPtr;
	EXPECT_EQ(weakPtrCopy.Get(), weakPtr.Get());

	weakFactoryOwner.reset();
	EXPECT_EQ(weakPtr.Get(), nullptr);
	EXPECT_EQ(weakPtrCopy.Get(), nullptr);
}

TEST(WeakPtrFactoryTest, InvalidateWeakPtrs)
{
	auto weakFactoryOwner = std::make_unique<WeakFactoryOwner>();
	auto weakPtr1 = weakFactoryOwner->GetWeakPtr();

	weakFactoryOwner->InvalidateWeakPtrs();

	EXPECT_EQ(weakPtr1.Get(), nullptr);

	// Should be able to create new WeakPtrs after invalidating existing pointers.
	auto weakPtr2 = weakFactoryOwner->GetWeakPtr();
	EXPECT_EQ(weakPtr2.Get(), weakFactoryOwner.get());
}

TEST(WeakPtrFactoryDeathTest, UseAfterInvalidation)
{
	auto weakFactoryOwner = std::make_unique<WeakFactoryOwner>();
	auto weakPtr = weakFactoryOwner->GetWeakPtr();

	weakFactoryOwner.reset();

	// The object has been destroyed, so this call should fail.
	EXPECT_CHECK_DEATH(weakPtr->IncrementCounter());
}

TEST(WeakPtrFactoryDeathTest, DereferenceAfterInvalidation)
{
	auto weakFactoryOwner = std::make_unique<WeakFactoryOwner>();
	auto weakPtr = weakFactoryOwner->GetWeakPtr();

	weakFactoryOwner.reset();

	EXPECT_CHECK_DEATH(*weakPtr);
}

TEST(WeakPtrFactoryDeathTest, InvalidationOnDifferentThread)
{
	auto weakFactoryOwner = std::make_unique<WeakFactoryOwner>();

	EXPECT_CHECK_DEATH({
		// WeakPtrFactory requires that the object be created and destroyed on a single thread,
		// so destroying the object on a different thread isn't a supported scenario.
		std::jthread thread([&weakFactoryOwner]() { weakFactoryOwner.reset(); });
	});
}

TEST(WeakPtrFactoryDeathTest, UseOnDifferentThread)
{
	auto weakFactoryOwner = std::make_unique<WeakFactoryOwner>();
	auto weakPtr = weakFactoryOwner->GetWeakPtr();

	EXPECT_CHECK_DEATH({
		// Dereferencing the WeakPtr on a different thread isn't supported, since the WeakPtr
		// has no way of keeping the object alive.
		std::jthread thread([weakPtr]() { weakPtr->IncrementCounter(); });
	});
}

TEST(WeakPtrFactoryDeathTest, DereferenceOnDifferentThread)
{
	auto weakFactoryOwner = std::make_unique<WeakFactoryOwner>();
	auto weakPtr = weakFactoryOwner->GetWeakPtr();

	EXPECT_CHECK_DEATH({ std::jthread thread([weakPtr]() { *weakPtr; }); });
}

TEST(WeakPtrFactoryDeathTest, GetOnDifferentThread)
{
	auto weakFactoryOwner = std::make_unique<WeakFactoryOwner>();
	auto weakPtr = weakFactoryOwner->GetWeakPtr();

	EXPECT_CHECK_DEATH({ std::jthread thread([weakPtr]() { weakPtr.Get(); }); });
}

TEST(WeakPtrFactoryDeathTest, BoolConversionOnDifferentThread)
{
	auto weakFactoryOwner = std::make_unique<WeakFactoryOwner>();
	auto weakPtr = weakFactoryOwner->GetWeakPtr();

	EXPECT_CHECK_DEATH({ std::jthread thread([weakPtr]() { static_cast<bool>(weakPtr); }); });
}

TEST(WeakPtrFactoryDeathTest, InvalidateWeakPtrsOnDifferentThread)
{
	auto weakFactoryOwner = std::make_unique<WeakFactoryOwner>();

	EXPECT_CHECK_DEATH({
		std::jthread thread([&weakFactoryOwner]() { weakFactoryOwner->InvalidateWeakPtrs(); });
	});
}
