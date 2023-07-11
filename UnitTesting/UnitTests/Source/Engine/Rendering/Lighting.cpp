#include <gtest/gtest.h>

void swap(int& val1, int& val2) {
	int tVal = val1;
	val1 = val2;
	val2 = tVal;
}

TEST(Swaptest, CanSwap)
{
	int val1 = 5;
	int val2 = 10;

	EXPECT_LT(val1,val2);

	swap(val1,val2);

	EXPECT_GT(val1,val2);
}


//#include "Core/Platform/Rendering/DX12/DX12RenderContext.h"

//using ::testing::AtLeast;
//
//class DX12RenderContextMock	: public DX12RenderContext
//{
//public:
//	//DX12RenderContextMock();
//	//~DX12RenderContextMock();
//	
//	MOCK_METHOD0(ForceUpdate, void());
//	MOCK_METHOD1(Submit, void(const LightPointData& renderData));
//};
//
///*TEST(LightingTest, CanSubmit) {
//	
//	DX12RenderContext RCTest;
//
//
//}*/
//
//TEST(LightingTest, CanUpdate)
//{
//	DX12RenderContextMock DXRC;
//
//	EXPECT_CALL(DXRC, ForceUpdate())
//		.Times(AtLeast(1));
//	
//	LightPointData LPDTest;
//	LPDTest.pos = Vector3(0.f,0.f,0.f);
//	LPDTest.col = Vector3(1.f, 1.f, 1.f);
//	LPDTest.range = 100.f;
//	LPDTest.strength = 1.f;
//
//	DXRC.Submit(LPDTest);
//}