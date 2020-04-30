//	"C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Tools\MSVC\14.24.28314\bin\Hostx64\x64\cl.exe" Depth16ToYuv.c /FeDepth16ToYuv.dll /O2 /link /LIBPATH:"C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Tools\MSVC\14.24.28314\lib\x64" /LIBPATH:"D:\Windows Kits\10\Lib\10.0.18362.0\um\x64" /DLL /LIBPATH:"D:\Windows Kits\10\Lib\10.0.18362.0\ucrt\x64"
#if defined(_MSC_VER)
#define EXPORT	__declspec( dllexport )
#else
#define EXPORT 
#endif

typedef unsigned short uint16_t;
typedef unsigned char uint8_t;
typedef unsigned int uint32_t;

#define ERROR_VALUE	255

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

EXPORT void SetYuvError(uint8_t* Yuv8_8_8Plane, uint32_t Width, uint32_t Height, uint8_t ErrorValue)
{
	int LumaSize = Width * Height;
	int LumaWidth = Width;
	int ChromaWidth = Width / 2;
	int ChromaHeight = Height / 2;
	int ChromaSize = ChromaWidth * ChromaHeight;
	int DepthSize = Width * Height;
	int YuvSize = LumaSize + ChromaSize + ChromaSize;
	for (int i = 0; i < YuvSize; i++)
		Yuv8_8_8Plane[i] = ErrorValue;
}

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


//	returns the max UV range count
EXPORT uint32_t GetUvRanges8(int32_t UvRangeCount,uint8_t* URanges,uint8_t* VRanges,uint32_t RangesSize)
{
	if ( UvRangeCount < 1 )
		UvRangeCount = 1;
	
	//	pick the next square number up so we can do a grid of uvs
	uint32_t UvMapCount = SoyMath_GetNextPower2(UvRangeCount);
	if (UvMapCount > RangesSize)
	{
		//	gr: do we need to make this next power down?
		//	gr: can I just do
		//	UvMapCount = UvMapCount/2
		//	?
		UvMapCount = RangesSize;
	}
	
	//	generate the uvs
	int w = sqrt(UvMapCount);
	for (int i = 0; i <UvMapCount; i++)
	{
		int x = i % w;
		int y = i / w;
		float u = x / static_cast<float>(w-1);
		float v = y / static_cast<float>(w-1);
		int u8 = u * 255.0f;
		int v8 = v * 255.0f;
		URanges[i] = u8;
		VRanges[i] = v8;
	}
	
	return UvMapCount;
}

typedef void WriteYuv_t(uint32_t x,uint32_t y,uint8_t Luma,uint8_t ChromaU,uint8_t ChromaV,void* This);


EXPORT void Depth16ToYuv(uint16_t* Depth16Plane,uint32_t Width, uint32_t Height, EncodeParams_t Params,WriteYuv_t* WriteYuv,void* This)
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
#define MAX_UVMAPCOUNT	(32)
	uint8_t URange8s[MAX_UVMAPCOUNT];
	uint8_t VRange8s[MAX_UVMAPCOUNT];
	uint32_t UvMapCount = GetUvRanges8(UvRangeCount, URange8s, VRange8s, MAX_UVMAPCOUNT);

	if (UvRangeCount > UvMapCount)
		UvRangeCount = UvMapCount;
	if (UvRangeCount < 1)
		UvRangeCount = 1;
	int RangeLengthMin1 = UvRangeCount - 1;

	for (int i = 0; i < DepthSize; i++)
	{
		int x = i % Width;
		int y = i / Width;
		int DepthIndex = i;
		int Depth16 = Depth16Plane[DepthIndex];
		
		float Depthf = RangeClamped(DepthMin, DepthMax, Depth16);
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
			//SetYuvError(Yuv8_8_8Plane, Width, Height, ERROR_VALUE);
			return;
		}

		WriteYuv(x, y, Luma, URange8s[RangeIndex], VRange8s[RangeIndex], This );
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