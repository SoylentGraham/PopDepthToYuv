//	"C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Tools\MSVC\14.24.28314\bin\Hostx64\x64\cl.exe" Depth16ToYuv.c /FeDepth16ToYuv.dll /O2 /link /LIBPATH:"C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Tools\MSVC\14.24.28314\lib\x64" /LIBPATH:"D:\Windows Kits\10\Lib\10.0.18362.0\um\x64" /DLL /LIBPATH:"D:\Windows Kits\10\Lib\10.0.18362.0\ucrt\x64"
#if defined(_MSC_VER)
#define EXPORT	__declspec( dllexport )
#else
#define EXPORT 
#endif

typedef unsigned short uint16_t;
typedef unsigned char uint8_t;
typedef unsigned int uint32_t;

//#define TEST_OUTPUT

int Floor(float f)
{
	return (int)f;
}

int Min(int a, int b)
{
	return (a < b) ? a : b;
}

int Max(int a, int b)
{
	return (a > b) ? a : b;
}

float Lerp(float Start,float End,float Time)
{
	return Start + ((End-Start)*Time);
}

float Range(float Min, float Max, float Value)
{
	return (Value - Min) / (Max - Min);
}

float RangeClamped(float Min, float Max, float Value)
{
	float f = Range(Min, Max, Value);
	if (f < 0)
		return 0;
	if (f > 1)
		return 1;
	return f;
}


struct float2
{
	float x;
	float y;
};

struct uint8_2
{
	uint8_t x;
	uint8_t y;
};

struct EncodeParams_t
{
	uint16_t DepthMin = 0;
	uint16_t DepthMax = 0xffff;
	uint16_t ChromaRangeCount = 1;
};


uint32_t SoyMath_GetNextPower2(uint32_t x)
{
	//	from hackers delight.
	if ( x == 0 )
		return 0;
	
	x--;
	x |= x >> 1;
	x |= x >> 2;
	x |= x >> 4;
	x |= x >> 8;
	x |= x >> 16;
	return x+1;
}

const uint32_t MaxUvRangeCount = 10 * 10;

EXPORT uint32_t GetUvRangeWidthHeight(int32_t UvRangeCount)
{
	//	get WxH for this size
	if (UvRangeCount < 2 * 2)	
		return 1;
	if (UvRangeCount <= 2 * 2)	return 2;
	if (UvRangeCount <= 3 * 3)	return 3;
	if (UvRangeCount <= 4 * 4)	return 4;
	if (UvRangeCount <= 5 * 5)	return 5;
	if (UvRangeCount <= 6 * 6)	return 6;
	if (UvRangeCount <= 7 * 7)	return 7;
	if (UvRangeCount <= 8 * 8)	return 8;
	if (UvRangeCount <= 9 * 9)	return 9;
	//if (UvRangeCount <= 10 * 10)
		return 10;
}

//	returns the max UV range count
EXPORT uint32_t GetUvRanges8(int32_t UvRangeCount,uint8_t* URanges,uint8_t* VRanges,uint32_t RangesSize)
{
	auto Width = GetUvRangeWidthHeight(UvRangeCount);
	auto Height = Width;
	UvRangeCount = Width * Height;

	//	generate the uvs
	for (int i = 0; i < UvRangeCount; i++)
	{
		int x = i % Width;
		int y = i / Width;
		float u = x / static_cast<float>(Width -1);
		float v = y / static_cast<float>(Width -1);
		int u8 = u * 255.0f;
		int v8 = v * 255.0f;
		URanges[i] = u8;
		VRanges[i] = v8;
	}
	
	return UvRangeCount;
}

//	callbacks
typedef void OnWriteYuv_t(uint32_t x,uint32_t y,uint8_t Luma,uint8_t ChromaU,uint8_t ChromaV,void* This);
typedef void OnError_t(const char* Error,void* This);

//	unit-test function
EXPORT uint16_t YuvToDepth(uint8_t Luma,uint8_t ChromaU,uint8_t ChromaV,EncodeParams_t& Params)
{
	//	work out from 0 to max this uv points at
	int Width = GetUvRangeWidthHeight(Params.ChromaRangeCount);
	auto Height = Width;
	int RangeMax = (Width*Height) - 1;

	//	gr: emulate shader
	float ChromaUv_x = ChromaU / 255.f;
	float ChromaUv_y = ChromaV / 255.f;

	//	in the encoder, u=x%width and /width, so scales back up to -1
	float xf = ChromaUv_x * float(Width - 1);
	float yf = ChromaUv_y * float(Height - 1);
	//	we need the nearest, so we floor but go up a texel
	int x = floor(xf + 0.5);
	int y = floor(yf + 0.5);
	
	//	gr: this should be nearest, not floor so add half
	//ChromaUv = floor(ChromaUv + float2(0.5, 0.5) );
	int Index = x + (y*Width);
		
	float Indexf = Index / float(RangeMax);
	float Nextf = (Index + 1) / float(RangeMax);
	//return float2(Indexf, Nextf);

	//	put into depth space
	Indexf = Lerp( Params.DepthMin, Params.DepthMax, Indexf );
	Nextf = Lerp( Params.DepthMin, Params.DepthMax, Nextf );
	float Lumaf = Luma / 255.0f;
	float Depth = Lerp( Indexf, Nextf, Lumaf );
	uint16_t Depth16 = Depth;
	
	return Depth16;
}

EXPORT void Depth16ToYuv(uint16_t* Depth16Plane,uint32_t Width, uint32_t Height, EncodeParams_t Params,OnWriteYuv_t* WriteYuv,OnError_t* OnError,void* This)
{
	auto DepthMin = Params.DepthMin;
	auto DepthMax = Params.DepthMax;
	auto UvRangeCount = Params.ChromaRangeCount;

	int LumaSize = Width * Height;
	int LumaWidth = Width;
	int ChromaWidth = Width / 2;
	int ChromaHeight = Height / 2;
	int ChromaSize = ChromaWidth * ChromaHeight;
	int DepthSize = Width * Height;
	int YuvSize = LumaSize + ChromaSize + ChromaSize;

	//	make our own YUV range count
	uint8_t URange8s[MaxUvRangeCount];
	uint8_t VRange8s[MaxUvRangeCount];
	UvRangeCount = GetUvRanges8(UvRangeCount, URange8s, VRange8s, MaxUvRangeCount);

	int RangeLengthMin1 = UvRangeCount - 1;

	for (int i = 0; i < DepthSize; i++)
	{
		int x = i % Width;
		int y = i / Width;
		int DepthIndex = i;
		int Depth16 = Depth16Plane[DepthIndex];
		
		float Depthf = RangeClamped(DepthMin, DepthMax, Depth16);
		//	gr: for testing, Depth16 might fall out of range
		int Depth16Clamped = Lerp( DepthMin, DepthMax, Depthf );
		//float Depthf = Range( DepthMin, DepthMax, Depth16 );

		//	split value into Distance Chunk(uv range) and 
		//	it's high-resolution normalised/quantisied value inside (remain)
		float DepthScaled = Depthf * (float)Max(RangeLengthMin1, 1);
		int RangeIndex = Floor(DepthScaled);
		//	float Remain = DepthScaled - RangeIndex;
		float Remain = DepthScaled - Floor(DepthScaled);
		RangeIndex = Min(RangeIndex, RangeLengthMin1);

		/*
				//	make luma go 0-1 1-0 0-1 so luma image wont have edges for compression
				if (Params.PingPongLuma)
					if (RangeIndex & 1)
						Remain = 1 - Remain;
		*/

		//	something wrong with RangeIndex, manually setting an index works
		//	but ANYTHING calculated seems to result in 0
		uint8_t Luma = Remain * 255.f;
		//uint8_t Luma = min(Depthf * 255.f,255);

		if (RangeIndex < 0 || RangeIndex >= UvRangeCount)
		{
			if ( OnError )
				OnError("Calculated range out of bounds",This);
			return;
		}

		auto u = URange8s[RangeIndex];
		auto v = VRange8s[RangeIndex];
		WriteYuv(x, y, Luma, u, v, This );
		
#if defined(TEST_OUTPUT)
		auto TestDepth = YuvToDepth( Luma, u, v, Params );
		if ( TestDepth != Depth16Clamped )
		{
			//	we should be able to figure out expected data loss (ie, tolerance)
			//	if this is > 1 then we're expecting data loss
			auto Tolerancef = (DepthMax-DepthMin) / (UvRangeCount * 255.0f);
			auto Tolerance = static_cast<int>(Tolerancef)+1;
			if ( abs(TestDepth-Depth16Clamped) > Tolerance )
			{
				TestDepth = YuvToDepth( Luma, u, v, Params );
				if ( OnError )
				{
					//	customise error
					auto Diff = TestDepth-Depth16Clamped;
					char ErrorStr[] = "Depth->Yuv->Depth failed [+XXXXX]";
					ErrorStr[26] = (Diff<0) ? '-' : '+';
					Diff = abs(Diff);
					ErrorStr[27] = '0' + ((Diff / 10000)%10);
					ErrorStr[28] = '0' + ((Diff / 1000)%10);
					ErrorStr[29] = '0' + ((Diff / 100)%10);
					ErrorStr[30] = '0' + ((Diff / 10)%10);
					ErrorStr[31] = '0' + ((Diff / 1)%10);
					//	swap leading zeros for spaces
					for ( auto i=27;	i<31;	i++ )
					{
						if ( ErrorStr[i] != '0' )
							break;
						ErrorStr[i] = ' ';
					}
					OnError(ErrorStr,This);
				}
			}
		}
#endif
	}
}


EXPORT void DepthfToYuv(float* DepthfPlane,uint32_t Width, uint32_t Height, EncodeParams_t Params,OnWriteYuv_t* WriteYuv,OnError_t* OnError,void* This)
{
	auto DepthMin = Params.DepthMin;
	auto DepthMax = Params.DepthMax;
	auto UvRangeCount = Params.ChromaRangeCount;
	
	int LumaSize = Width * Height;
	int LumaWidth = Width;
	int ChromaWidth = Width / 2;
	int ChromaHeight = Height / 2;
	int ChromaSize = ChromaWidth * ChromaHeight;
	int DepthSize = Width * Height;
	int YuvSize = LumaSize + ChromaSize + ChromaSize;
	
	//	make our own YUV range count
	uint8_t URange8s[MaxUvRangeCount];
	uint8_t VRange8s[MaxUvRangeCount];
	UvRangeCount = GetUvRanges8(UvRangeCount, URange8s, VRange8s, MaxUvRangeCount);
	
	int RangeLengthMin1 = UvRangeCount - 1;
	
	for (int i = 0; i < DepthSize; i++)
	{
		int x = i % Width;
		int y = i / Width;
		int DepthIndex = i;
		float Depthf = DepthfPlane[DepthIndex];
		
		Depthf = RangeClamped(DepthMin, DepthMax, Depthf);

		//	split value into Distance Chunk(uv range) and
		//	it's high-resolution normalised/quantisied value inside (remain)
		float DepthScaled = Depthf * (float)Max(RangeLengthMin1, 1);
		int RangeIndex = Floor(DepthScaled);
		//	float Remain = DepthScaled - RangeIndex;
		float Remain = DepthScaled - Floor(DepthScaled);
		RangeIndex = Min(RangeIndex, RangeLengthMin1);
		
		/*
		 //	make luma go 0-1 1-0 0-1 so luma image wont have edges for compression
		 if (Params.PingPongLuma)
		 if (RangeIndex & 1)
		 Remain = 1 - Remain;
		 */
		
		//	something wrong with RangeIndex, manually setting an index works
		//	but ANYTHING calculated seems to result in 0
		uint8_t Luma = Remain * 255.f;
		//uint8_t Luma = min(Depthf * 255.f,255);
		
		if (RangeIndex < 0 || RangeIndex >= UvRangeCount)
		{
			if ( OnError )
			OnError("Calculated range out of bounds",This);
			return;
		}
		
		auto u = URange8s[RangeIndex];
		auto v = VRange8s[RangeIndex];
		WriteYuv(x, y, Luma, u, v, This );
		
#if defined(TEST_OUTPUT)
		auto TestDepth = YuvToDepth( Luma, u, v, Params );
		if ( TestDepth != Depth16Clamped )
		{
			//	we should be able to figure out expected data loss (ie, tolerance)
			//	if this is > 1 then we're expecting data loss
			auto Tolerancef = (DepthMax-DepthMin) / (UvRangeCount * 255.0f);
			auto Tolerance = static_cast<int>(Tolerancef)+1;
			if ( abs(TestDepth-Depth16Clamped) > Tolerance )
			{
				TestDepth = YuvToDepth( Luma, u, v, Params );
				if ( OnError )
				{
					//	customise error
					auto Diff = TestDepth-Depth16Clamped;
					char ErrorStr[] = "Depth->Yuv->Depth failed [+XXXXX]";
					ErrorStr[26] = (Diff<0) ? '-' : '+';
					Diff = abs(Diff);
					ErrorStr[27] = '0' + ((Diff / 10000)%10);
					ErrorStr[28] = '0' + ((Diff / 1000)%10);
					ErrorStr[29] = '0' + ((Diff / 100)%10);
					ErrorStr[30] = '0' + ((Diff / 10)%10);
					ErrorStr[31] = '0' + ((Diff / 1)%10);
					//	swap leading zeros for spaces
					for ( auto i=27;	i<31;	i++ )
					{
						if ( ErrorStr[i] != '0' )
						break;
						ErrorStr[i] = ' ';
					}
					OnError(ErrorStr,This);
				}
			}
		}
#endif
	}
}

/*
EXPORT void Depth16ToYuv_8_8_8(uint16_t* Depth16Plane, uint8_t* Yuv8_8_8Plane, uint32_t Width, uint32_t Height, uint32_t DepthMin, uint32_t DepthMax,uint16_t ChromaRangeCount)
{
	int LumaSize = Width * Height;
	int LumaWidth = Width;
	int ChromaWidth = Width / 2;
	int ChromaHeight = Height / 2;
	int ChromaSize = ChromaWidth * ChromaHeight;
	int DepthSize = Width * Height;
	int YuvSize = LumaSize + ChromaSize + ChromaSize;


	int GetChromaIndex_8_8_(int Depthx, int Depthy, int LumaWidth)
	{
		int ChromaWidth = LumaWidth / 2;
		//	this is more complicated than one would assume
		//	we need the TEXTURE BUFFER index we're writing to;
		//	each TEXTURE row is 2x pixel rows as the chroma width is half, but
		//	the luma width is the same, so chroma buffer looks like this
		//	ROW1ROW2
		//	ROW3ROW4
		int ChromaX = (Depthx / 2);
		int ChromaY = (Depthy / 2);
		int Left = (ChromaY % 2) == 0;

		int WriteX = Left ? ChromaX : ChromaX + ChromaWidth;
		int WriteY = (ChromaY / 2);

		int WriteIndex = WriteX + (WriteY * LumaWidth);
		return WriteIndex;
	}

	auto WriteYuv = [&](uint32_t x, uint32_t y, uint8_t Luma, uint8_t ChromaU, uint8_t ChromaV)
	{
		int LumaIndex = (y* LumaWidth) + x;
		int ChromaIndex = GetChromaIndex(x, y, LumaWidth);
		int ChromaUIndex = LumaSize + ChromaIndex;
		int ChromaVIndex = LumaSize + ChromaSize + ChromaIndex;


	};

	EncodeParams_t EncodeParams;
	EncodeParams.DepthMin = DepthMin;
	EncodeParams.DepthMax = DepthMax;

	Depth16ToYuv(Depth16Plane, Width, Height, EncodeParams, WriteYuv);


	//	make our own YUV range count
#define MAX_UVMAPCOUNT	(32)
	uint8_t URange8s[MAX_UVMAPCOUNT];
	uint8_t VRange8s[MAX_UVMAPCOUNT];
	uint32_t UvMapCount = GetUvRanges8( UvRangeCount, URange8s, VRange8s, MAX_UVMAPCOUNT );

	if ( UvRangeCount > UvMapCount )
		UvRangeCount = UvMapCount;
	if ( UvRangeCount < 1 )
		UvRangeCount = 1;
	int RangeLengthMin1 = UvRangeCount - 1;

	for (int i = 0; i < DepthSize; i++)
	{
		int x = i % Width;
		int y = i / Width;
		int DepthIndex = i;
		int Depth16 = Depth16Plane[DepthIndex];
		
		if (ChromaUIndex >= YuvSize || ChromaVIndex >= YuvSize)
		{
			SetYuvError(Yuv8_8_8Plane, Width, Height,ERROR_VALUE);
			return;
		}

		float Depthf = RangeClamped(DepthMin, DepthMax, Depth16);
		//float Depthf = Range( DepthMin, DepthMax, Depth16 );

		float DepthScaled = Depthf * (float)Max(RangeLengthMin1,1);
		int RangeIndex = Floor(DepthScaled);
		//	float Remain = DepthScaled - RangeIndex;
		float Remain = DepthScaled - Floor(DepthScaled);
		RangeIndex = Min(RangeIndex, RangeLengthMin1);

		//	something wrong with RangeIndex, manually setting an index works
		//	but ANYTHING calculated seems to result in 0
		uint8_t Luma = Remain * 255.f;
		//uint8_t Luma = min(Depthf * 255.f,255);

		if (RangeIndex < 0 || RangeIndex >= UvRangeCount)
		{
			SetYuvError(Yuv8_8_8Plane, Width, Height, ERROR_VALUE);
			return;
		}
		Yuv8_8_8Plane[LumaIndex] = Luma;
		Yuv8_8_8Plane[ChromaUIndex] = URange8s[RangeIndex];
		Yuv8_8_8Plane[ChromaVIndex] = VRange8s[RangeIndex];
	}
}
*/
