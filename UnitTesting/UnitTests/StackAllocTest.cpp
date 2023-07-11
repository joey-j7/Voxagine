
#include "gtest/gtest.h"

#include "Core/Memory/Allocators/StackAlloc.h"

struct Foo
{
	Foo() = default;

	int i = 5;
	float j = 6.f;
	double k = 7.0;
};

static StackAlloc s_alloc;
static Foo* s_pFoo;

class StackAllocTest : public ::testing::Test
{
protected:

	StackAllocTest() {}

	~StackAllocTest() override {}

	static void SetUpTestSuite() {
	}

	static void TearDownTestSuite() {
	}

	void SetUp() override {
	}

	void TearDown() override {
	}

};



TEST_F(StackAllocTest, Initialize)
{
	s_alloc.Initialize(16 * 1024);

	EXPECT_EQ(s_alloc.HasBaseAddr(), true);
}

TEST_F(StackAllocTest, Allocate)
{

	s_pFoo = static_cast<Foo*>(s_alloc.Allocate(sizeof(Foo), alignof(Foo)));
	*s_pFoo = Foo();

	StackAlloc::StackHeader* headerPtr = reinterpret_cast<StackAlloc::StackHeader*>(
		reinterpret_cast<uint8_t*>(s_pFoo) - sizeof(StackAlloc::StackHeader));
	BaseAlloc::BlockFooter* footerPtr = reinterpret_cast<BaseAlloc::BlockFooter*>(
		reinterpret_cast<uint8_t*>(s_pFoo) + headerPtr->m_size);

	EXPECT_EQ(footerPtr->m_guard, 42);

	EXPECT_EQ(s_pFoo->i, 5);
	EXPECT_NEAR(s_pFoo->j, 6.f, 0.0000005);
	EXPECT_NEAR(s_pFoo->k, 7.0, 0.0000005);
}


TEST_F(StackAllocTest, Free)
{
	StackAlloc::StackHeader* headerPtr = reinterpret_cast<StackAlloc::StackHeader*>(
		reinterpret_cast<uint8_t*>(s_pFoo) - sizeof(StackAlloc::StackHeader));
	BaseAlloc::BlockFooter* footerPtr = reinterpret_cast<BaseAlloc::BlockFooter*>(
		reinterpret_cast<uint8_t*>(s_pFoo) + headerPtr->m_size);

	EXPECT_EQ(footerPtr->m_guard, 42);

	s_alloc.Free(s_pFoo);

	EXPECT_EQ(footerPtr->m_guard, -1);

}

TEST_F(StackAllocTest, Destroy)
{
	s_alloc.Destroy();

	EXPECT_EQ(s_alloc.HasBaseAddr(), false);
}
