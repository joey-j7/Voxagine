#include "gtest/gtest.h"

#include "Core/Memory/Allocators/PoolAlloc.h"

struct Foo
{
	Foo() = default;

	int i = 5;
	float j = 6.f;
	double k = 7.0;
};

static PoolAlloc s_alloc(sizeof(Foo),alignof(Foo));
static Foo* s_pFoo;

class PoolAllocTest : public ::testing::Test
{
protected:

	PoolAllocTest() {}

	~PoolAllocTest() override {}

	static void SetUpTestSuite() {
	}

	static void TearDownTestSuite() {
	}

	void SetUp() override {
	}

	void TearDown() override {
	}

};



TEST_F(PoolAllocTest, Initialize)
{
	s_alloc.Initialize(16 * 1024);

	EXPECT_EQ(s_alloc.HasBaseAddr(), true);
}

TEST_F(PoolAllocTest, Allocate)
{

	s_pFoo = static_cast<Foo*>(s_alloc.Allocate(sizeof(Foo), alignof(Foo)));
	*s_pFoo = Foo();

	BaseAlloc::BlockHeader* headerPtr = reinterpret_cast<BaseAlloc::BlockHeader*>(
		reinterpret_cast<uint8_t*>(s_pFoo) - sizeof(BaseAlloc::BlockHeader));
	BaseAlloc::BlockFooter* footerPtr = reinterpret_cast<BaseAlloc::BlockFooter*>(
		reinterpret_cast<uint8_t*>(s_pFoo) + headerPtr->m_size);

	EXPECT_EQ(footerPtr->m_guard, 42);

	EXPECT_EQ(s_pFoo->i, 5);
	EXPECT_NEAR(s_pFoo->j, 6.f, 0.0000005);
	EXPECT_NEAR(s_pFoo->k, 7.0, 0.0000005);
}


TEST_F(PoolAllocTest, Free)
{
	BaseAlloc::BlockHeader* headerPtr = reinterpret_cast<BaseAlloc::BlockHeader*>(
		reinterpret_cast<uint8_t*>(s_pFoo) - sizeof(BaseAlloc::BlockHeader));
	BaseAlloc::BlockFooter* footerPtr = reinterpret_cast<BaseAlloc::BlockFooter*>(
		reinterpret_cast<uint8_t*>(s_pFoo) + headerPtr->m_size);

	EXPECT_EQ(footerPtr->m_guard, 42);

	s_alloc.Free(s_pFoo);

	EXPECT_EQ(footerPtr->m_guard, -1);

}

TEST_F(PoolAllocTest, Destroy)
{
	s_alloc.Destroy();

	EXPECT_EQ(s_alloc.HasBaseAddr(), false);
}
