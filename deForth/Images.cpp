#include "Images.h"
#include "files.h"

// Using the gif library from https://github.com/dloebl/cgif
#include "cgif/cgif.h"

#include <iostream>
#include <fstream>

#include <format>

// Screen Modes 
// 1: B&W		1E27 (CGA     620x200 B&W (Mode 6) , Palet 1 Dark, Forground color 7 )
// 2: RGB		2A20 (CGA     320x200 4 Color (Mode 4), Pallet 1 Dark, Background Color 0)
//    Tandy     0A27 (Tandy   160x200 16 Color, background color 7, Custom Pallet)
// 3: Compsite	1A27 (CGA     620x200 'Color', Palet 1 Dark, Forground color 7 )
// 4: Hercules  1E27 (Herules 620x200 B&W (Mode 6) , Palet 1 Dark, Forground color 7 )
// 5: EGA            (EGA     160x200 16 colors, (Mode Dh) )

// Future
//    VGA		     (VGA     320x200 256 Color (Mode 13h?) ) 

// CGA Color:    00 22 11 99 44 66 88 AA 55 77 BB 33 EE CC DD FF
// EGA Color:    00 01 02 0A 04 05 06 07 08 0B 03 09 0C 06 0E 0F 
// Tandy Pallet: 00 02 01 0B 04 08 05 09 06 0A 07 03 0C 0E 0D 0F	// [GAME-OV:E000]

// These maps the screen color enum to the pallet entry
uint8_t CGAScreenColorMap[16] = { 0x00, 0x02, 0x01, 0x09, 0x04, 0x06, 0x08, 0x0A, 0x05, 0x07, 0x0B, 0x03, 0x0E, 0x0C, 0x0D, 0x0F, };
uint8_t EGAScreenColorMap[16] = { 0x00, 0x01, 0x02, 0x0A, 0x04, 0x05, 0x06, 0x07, 0x08, 0x0B, 0x03, 0x09, 0x0C, 0x06, 0x0E, 0x0F, };

// For Tandy, the game uses the CGA color map, and changes the Tandy pallet, not quite mapping everything to EGA colors in the process
uint8_t TandyPallet[16] = { 0x00, 0x02, 0x01, 0x0B, 0x04, 0x08, 0x05, 0x09, 0x06, 0x0A, 0x07, 0x03, 0x0C, 0x0E, 0x0D, 0x0F, };


// Colors in image files are stored with CGA color values.
// To convert to EGA, we seach he CGA color map to find
// The color, then use the EGA color at the same index

//               0  1  2  3  4  5  6  7  8  9  A  B  C  D  E  F
// CGA Color:    00 22 11 99 44 66 88 AA 55 77 BB 33 EE CC DD FF
// EGA Color:    00 01 02 0A 04 05 06 07 08 0B 03 09 0C 06 0E 0F 
uint8_t CGAtoEGAColorMap[16] = { 0x00, 0x02, 0x01, 0x09, 0x04, 0x08, 0x05, 0x0B, 0x06, 0x0A, 0x07, 0x03, 0x06, 0x0E, 0x0C, 0x0F, };



#define RGB(r, g, b) ((((b) & 0xFF) << 16) | (((g) & 0xFF) << 8) | ((r) & 0xFF))

uint32_t CGAPallet[16] =
{
	// Pallet 0 - Dark
	RGB(0x00, 0x00, 0x00),	 // 00 Black
	RGB(0x00, 0xAA, 0x00),	 // 02 Green
	RGB(0xAA, 0x00, 0x00),	 // 04 Red
	RGB(0xAA, 0x55, 0x00),	 // 06 Brown

	// Pallet 0 - Bright	
	RGB(0x00, 0x00, 0x00),	 // 00 Black
	RGB(0x55, 0xFF, 0x55),	 // 0A Light Green
	RGB(0xFF, 0x55, 0x55),	 // 0C Light Red/Pink 
	RGB(0xFF, 0xFF, 0x55),	 // 0E Yellow

	// Pallet 1 - Dark
	RGB(0x00, 0x00, 0x00),	 // 00 Black
	RGB(0x00, 0xAA, 0xAA),	 // 03 Cyan
	RGB(0xAA, 0x00, 0xAA),	 // 05 Magenta
	RGB(0xAA, 0xAA, 0xAA),	 // 07 Light Grey

	// Pallet 1 - Bright
	RGB(0x00, 0x00, 0x00),	 // 00 Black
	RGB(0x55, 0xFF, 0xFF),	 // 0B Light Cyan
	RGB(0xFF, 0x55, 0xFF),	 // 0D Light Magenta
	RGB(0xFF, 0xFF, 0xFF),	 // 0F White
};

uint32_t CGACompositePallet[16] =
{
	RGB(0x00, 0x00, 0x00),	// 0 Black
	RGB(0x00, 0x91, 0x00),	// 1 Dark Green
	RGB(0x00, 0x53, 0xFF),	// 2 Dark Blue
	RGB(0xFF, 0xC3, 0xFF),	// 3 Light Blue
	RGB(0xF4, 0x00, 0x83),	// 4 Red
	RGB(0x7F, 0x7F, 0x7F),	// 5 Grey2
	RGB(0xF7, 0x69, 0xFF),	// 6 Violet
	RGB(0x5B, 0xC0, 0xFF),	// 7 Blue
	RGB(0x6F, 0x54, 0x00),	// 8 Brown
	RGB(0x00, 0xEB, 0x00),	// 9 Green
	RGB(0x7F, 0x7F, 0x7F),	// A Grey1
	RGB(0x00, 0xff, 0x72),	// B Light Green
	RGB(0xFF, 0x2F, 0x00),	// C Orange
	RGB(0xF0, 0xD4, 0x00),	// D Yellow
	RGB(0xFD, 0x6D, 0xFF),	// E Pink
	RGB(0xFF, 0xFF, 0xFF),	// F White
};

constexpr const uint32_t EGAPallet[16] =
{
	// Stock CGA/EGA colors
	RGB(0x00, 0x00, 0x00),	 // 0 Black
	RGB(0x00, 0x00, 0xAA),	 // 1 Blue
	RGB(0x00, 0xAA, 0x00),	 // 2 Green
	RGB(0x00, 0xAA, 0xAA),	 // 3 Cyan
	RGB(0xAA, 0x00, 0x00),	 // 4 Red
	RGB(0xAA, 0x00, 0xAA),	 // 5 Magenta
	RGB(0xAA, 0x55, 0x00),	 // 6 Brown
	RGB(0xAA, 0xAA, 0xAA),	 // 7 Light Grey
	RGB(0x55, 0x55, 0x55),	 // 8 Dark Grey
	RGB(0x55, 0x55, 0xFF),	 // 9 Light Blue
	RGB(0x55, 0xFF, 0x55),	 // A Light Green
	RGB(0x55, 0xFF, 0xFF),	 // B Light Cyan
	RGB(0xFF, 0x55, 0x55),	 // C Light Red/Pink 
	RGB(0xFF, 0x55, 0xFF),	 // D Light Magenta
	RGB(0xFF, 0xFF, 0x55),	 // E Yellow
	RGB(0xFF, 0xFF, 0xFF),	 // F White
};

ScreenType Screen = ScreenType::EGA;

uint32_t GetColor(uint8_t ColorIndex, ScreenType ScreenMode)
{
	if (ScreenMode == ScreenType::Default)
		ScreenMode = Screen;

	switch (ScreenMode)
	{
		case ScreenType::BW:
		case ScreenType::Hercules:
		{
			if ((ColorIndex & 0x1) == 0)
				return 0x00;
			else
				return RGB(0xAA, 0xAA, 0xAA);
		}

		case ScreenType::CGA:
		{
			// Use Pallet 1 Dark
			return CGAPallet[(ColorIndex & 0x03) + 0x08];
		}

		case ScreenType::Composite:
			return CGACompositePallet[(ColorIndex & 0x0F)];

		case ScreenType::Tandy:
			return EGAPallet[TandyPallet[(ColorIndex & 0x0F)]];

		//case ScreenType::VGA:
		//	return VGAPallet[ColorIndex];

		case ScreenType::Default:
		case ScreenType::EGA:
		default:
			return EGAPallet[ColorIndex & 0x0F];
	}
}

uint8_t ScreenColorToColorIndex(ScreenColor ColorNumber, ScreenType ScreenMode)
{
	if (ScreenMode == ScreenType::Default)
		ScreenMode = Screen;

	uint32_t ColorIndex = 0;
	if (ScreenMode == ScreenType::EGA || ScreenMode == ScreenType::Default)
		ColorIndex = EGAScreenColorMap[(int)ColorNumber];
	else
		ColorIndex = CGAScreenColorMap[(int)ColorNumber];

	return ColorIndex;
}


// Looks up the mapped color based on the Enum
uint32_t GetScreenColor(ScreenColor ColorNumber, ScreenType ScreenMode)
{
	return GetColor(ScreenColorToColorIndex(ColorNumber, ScreenMode), ScreenMode);
}

uint8_t FileColorToColorIndex(uint8_t ColorNumber, ScreenType ScreenMode)
{
	if (ScreenMode == ScreenType::Default)
		ScreenMode = Screen;

	if (ScreenMode == ScreenType::EGA || ScreenMode == ScreenType::Default)
	{
		ColorNumber = CGAtoEGAColorMap[ColorNumber & 0x0F];
	}

	return ColorNumber;
}

void Image::SetPoint(int PosX, int PosY, uint32_t Color)
{
	for (int x = 0; x < PixelSize; x++)
	{
		SetPoint(PosX, PosY, x, Color);
	}
}

void Image::SetPoint(int PosX, int PosY, int Offset, uint32_t Color)
{
	if (PosX < 0 || PosY < 0)
		return;

	if (PosX >= Width || PosY >= Height)
		return;

	int Pos = (PosY * (Width * PixelSize)) + (PosX * PixelSize);

	(*this)[Pos + Offset] = Color;
}

void Image::DrawLine(int StartX, int StartY, int DestX, int DestY, uint32_t Color)
{
	int DeltaX = abs(DestX - StartX);
	int sx = StartX < DestX ? 1 : -1;

	int DeltaY = -abs(DestY - StartY);
	int sy = StartY < DestY ? 1 : -1;

	int Error = DeltaX + DeltaY;

	int xPos = StartX;
	int yPos = StartY;

	while (true)
	{
		SetPoint(xPos, yPos, Color);

		int E2 = 2 * Error;
		if (E2 >= DeltaY)
		{
			if (xPos == DestX)
				break;

			Error += DeltaY;
			xPos += sx;
		}
		if (E2 <= DeltaX)
		{
			if (yPos == DestY)
				break;

			Error += DeltaX;
			yPos += sy;
		}
	}
}

void Image::DrawCircle(int StartX, int StartY, int Radius, uint32_t Color, bool Filled)
{
	int T1 = Radius / 16;
	int x = Radius;
	int y = 0;

	while (x >= y)
	{
		if (Filled)
		{
			DrawLine((StartX + x), (StartY + y), (StartX + x), (StartY - y), Color);
			DrawLine((StartX - x), (StartY + y), (StartX - x), (StartY - y), Color);
			DrawLine((StartX + y), (StartY + x), (StartX + y), (StartY - x), Color);
			DrawLine((StartX - y), (StartY + x), (StartX - y), (StartY - x), Color);
		}
		else
		{
			SetPoint((StartX + x), (StartY + y), Color);
			SetPoint((StartX + x), (StartY - y), Color);
			SetPoint((StartX - x), (StartY + y), Color);
			SetPoint((StartX - x), (StartY - y), Color);
			SetPoint((StartX + y), (StartY + x), Color);
			SetPoint((StartX + y), (StartY - x), Color);
			SetPoint((StartX - y), (StartY + x), Color);
			SetPoint((StartX - y), (StartY - x), Color);
		}
		y++;
		T1 = T1 + y;
		int T2 = T1 - x;
		if (T2 >= 0)
		{
			T1 = T2;
			x--;
		}
	}
}

void SetPoint(Image& ImageData, int xPos, int yPos, uint8_t ColorIndex)
{
	if (ImageData.PixelSize == ScreenPixelSize() && (Screen == ScreenType::BW || Screen == ScreenType::Hercules))
	{
		// For B&W Each pixel in the color map is just On or Off
		ImageData.SetPoint(xPos, yPos, 0, GetColor((ColorIndex & 0x08) >> 3));
		ImageData.SetPoint(xPos, yPos, 1, GetColor((ColorIndex & 0x04) >> 2));
		ImageData.SetPoint(xPos, yPos, 2, GetColor((ColorIndex & 0x02) >> 1));
		ImageData.SetPoint(xPos, yPos, 3, GetColor((ColorIndex & 0x01)));
	}
	else if (ImageData.PixelSize == ScreenPixelSize() && Screen == ScreenType::CGA)
	{
		// For CGA the high two bits are one pixel, and the low two bits are the second
		ImageData.SetPoint(xPos, yPos, 0, GetColor((ColorIndex & 0x0C) >> 2));
		ImageData.SetPoint(xPos, yPos, 1, GetColor((ColorIndex & 0x03)));
	}
	else if (ImageData.PixelSize == ScreenPixelSize() && Screen == ScreenType::Composite)
	{
		ImageData.SetPoint(xPos, yPos, GetColor((ColorIndex)));
	}
	else
	{
		// Everything else just repeats 
		ImageData.SetPoint(xPos, yPos, GetColor(ColorIndex));
	}
}


void Read16ColorImageFile(uint32_t Address, Image& ImageData, int ImageWidth, int ImageHeight, int xPos, int yPos, bool WriteBlack)
{
	if (ImageWidth == 0)
		ImageWidth = ImageData.Width;

	if (ImageHeight == 0)
		ImageHeight = ImageData.Height;

	int Pos = 0;
	for (int PosY = 0; PosY < ImageHeight; PosY++)
	{
		for (int PosX = 0; PosX < ImageWidth; PosX += 2)
		{
			uint8_t Val = ReadByteDirect(Address);
			Address++;

			uint8_t C1 = (Val & 0xF0) >> 4;
			uint8_t C2 = (Val & 0x0F);

			if(C1 != 0 || WriteBlack)
				SetPoint(ImageData, xPos + PosX,     yPos + PosY, FileColorToColorIndex(C1));
			
			if (C2 != 0 || WriteBlack)
				SetPoint(ImageData, xPos + PosX + 1, yPos + PosY, FileColorToColorIndex(C2));
		}
	}
}

void ReadUpsideDown16ColorImageFile(uint32_t Address, Image& ImageData, int ImageWidth, int ImageHeight, int xPos, int yPos)
{
	if (ImageWidth == 0)
		ImageWidth = ImageData.Width;

	if (ImageHeight == 0)
		ImageHeight = ImageData.Height;

	// Color images are stored two pixels in a byte, 4-bpp, providing 16 colors 
	int Pos = 0;
	for (int PosY = ImageHeight - 1; PosY >= 0; PosY--)
	{
		for (int PosX = 0; PosX < ImageWidth; PosX += 2)
		{
			uint8_t Val = ReadByteDirect(Address);
			Address++;

			SetPoint(ImageData, xPos + PosX,     yPos + PosY, FileColorToColorIndex((Val & 0xF0) >> 4));
			SetPoint(ImageData, xPos + PosX + 1, yPos + PosY, FileColorToColorIndex(Val & 0x0F));
		}
	}
}

void DrawBitmapAsText(std::ofstream& Output, uint32_t Address, int Width, int Height, bool Direct, char Off, char On)
{
	uint16_t Mask = 0;
	uint16_t Val = 0;

	Output << "   ";

	for (int x = 0; x < Width * Height; x++)
	{
		if (x != 0 && x % Width == 0)
			Output << "\n   ";

		if (Mask == 0)
		{
			if(Direct)
				Val = ReadWordDirect(Address);
			else
				Val = ReadWord(Address);

			Address += 2;
			Mask = 0x8000;
		}

		if ((Val & Mask) == 0)
		{
			Output << Off;
		}
		else
		{
			Output << On;
		}
		Mask >>= 1;
	}

	Output << "\n";
}

int ReadBitmapImage(uint32_t Address, Image& ImageData, int ImageWidth, int ImageHeight, uint8_t ColorIndex, int xPos, int yPos, bool Direct)
{
	if (ImageWidth == 0)
		ImageWidth = ImageData.Width;

	if (ImageHeight == 0)
		ImageHeight = ImageData.Height;

	// Bitmap images are stored as a series of Words, with each bit representing a pixel.

	uint16_t Mask = 0;
	uint16_t Val = 0;
	int Pos = 0;
	int Offset = 0;
	for (int PosY = 0; PosY < ImageHeight; PosY++)
	{
		for (int PosX = 0; PosX < ImageWidth; PosX++)
		{
			if (Mask == 0)
			{
				Mask = 0x8000;
				if(Direct)
					Val = ReadWordDirect(Address + Offset);
				else
					Val = ReadWord(Address + Offset);

				Offset += 2;
			}

			if ((Val & Mask) != 0)
			{
				SetPoint(ImageData, xPos + PosX, yPos + PosY, ColorIndex);
			}

			Mask >>= 1;
		}
	}

	return Offset;
}

int ReadRLEBitmapImage(uint32_t Address, Image& ImageData, uint32_t Length, int ImageWidth, int ImageHeight, uint8_t ColorIndex, bool Bright, int xPos, int yPos)
{
	if (ImageWidth == 0)
		ImageWidth = ImageData.Width;

	if (ImageHeight == 0)
		ImageHeight = ImageData.Height;

	// The RLE for 1-bit image data is easy. Each byte is the run length, and the value switches each byte
	// We only write out the 'bright' colors, dark dosn't change anything.

	int Pos = 0;
	int Offset = 0;

	uint8_t Count = 0;
	// Invert it, as it will be inverted again when we read the first count
	Bright = !Bright;

	for (int PosY = 0; PosY < ImageHeight; PosY++)
	{
		if (Count == 0 && Offset == Length)
			break;

		for (int PosX = 0; PosX < ImageWidth; PosX++)
		{
			if (Count == 0 && Offset == Length)
				break;

			while (Count == 0)
			{
				Count = ReadByteDirect(Address + Offset);
				Offset++;
				Bright = !Bright;
			};

			if (Count != 0 && Bright)
			{
				SetPoint(ImageData, xPos + PosX, yPos + PosY, ColorIndex);
			}

			Count--;
		}
	}

	return Offset;
}

int ScreenPixelSize(ScreenType ScreenMode)
{
	if (ScreenMode == ScreenType::Default)
		ScreenMode = Screen;

	switch (ScreenMode)
	{
		// Render 4 B&W pixels for each Pixel (640 x 200)
		case ScreenType::BW:
		case ScreenType::Hercules:
			return 4;

		// Two Pixeles per write, split into CGA colors (CGA 320 x 200)
		case ScreenType::CGA:
			return 2;

		// Two pixel per a write (Tandy 320 x 200, CGA Composite 320), repeating the value
		case ScreenType::Composite:
			return 2;

		case ScreenType::Tandy:
			return 2;

		// Two pixels per a write (EGA 320 x 200)
		case ScreenType::Default:
		case ScreenType::EGA:
			return 2;

	}

	return 1;
}

void WritePPMFile(std::string& OutputPath, std::string FileName, std::vector<uint8_t> &ImageData, int Width, int Height)
{
	std::string OutputFile = OutputPath + FileName + ".ppm";

	std::ofstream Output;
	Output.open(OutputFile, std::ios::binary);

	if (!Output.is_open())
		abort();

	Output << "P6\n";
	Output << std::format("{} {} \n", Width, Height);
	Output << "255\n";

	//if (UpsideDown)
	//{
	//	for (int x = Height - 1; x >= 0; x--)
	//	{
	//		uint32_t Pos = (Width) * x;
	//		for (int y = 0; y < Width; y++)
	//		{
	//			Output.write((char*)&ImageData[Pos + y], 3);
	//		}
	//	}
	//}
	//else
	//{
	for (auto Val : ImageData)
	{
		uint32_t color = Val;
		if (Screen == ScreenType::Composite && color != 0x000000)
			color |= 0x333333;

		Output.write((char*)&color, 3);
	}
	//}

	Output.close();
}

struct GIFData
{
	std::map<uint32_t, uint8_t> ColorMap;
	std::vector<uint8_t> ColorTable;
	std::vector<uint8_t> Data;
};

void BuildGIFData(GIFData& GIFData, std::vector<uint32_t>& ImageData, int Width, int Height)
{
	GIFData.Data.resize(0);

	for (int y = 0; y < Height; y++)
	{
		int Base = y * Width;
		for (int x = 0; x < Width; x++)
		{
			uint32_t Val = ImageData[Base + x];
			if (GIFData.ColorMap.find(Val) == GIFData.ColorMap.end())
			{
				GIFData.ColorMap[Val] = (uint8_t)(GIFData.ColorTable.size() / 3);
				GIFData.ColorTable.emplace_back(Val & 0x0000FF);
				GIFData.ColorTable.emplace_back((Val & 0x00FF00) >> 8);
				GIFData.ColorTable.emplace_back((Val & 0xFF0000) >> 16);
			}

			GIFData.Data.emplace_back(GIFData.ColorMap[Val]);
		}
	}
}

void WriteGIFFile(const std::filesystem::path& OutputPath, std::string FileName, std::vector<uint32_t>& ImageData, int Width, int Height)
{
	GIFData Data;
	BuildGIFData(Data, ImageData, Width, Height);
	
	std::filesystem::path OutputFile = OutputPath / (FileName + ".gif");
	auto OutputFileStr = OutputFile.string();

	CGIF_Config	gConfig;
	memset(&gConfig, 0, sizeof(CGIF_Config));
	gConfig.width = Width;
	gConfig.height = Height;
	gConfig.pGlobalPalette = &Data.ColorTable[0];
	gConfig.numGlobalPaletteEntries = (uint16_t)(Data.ColorTable.size() / 3);
	gConfig.path = OutputFileStr.c_str();

	auto pGIF = cgif_newgif(&gConfig);

	CGIF_FrameConfig   fConfig;
	memset(&fConfig, 0, sizeof(CGIF_FrameConfig));
	fConfig.pImageData = &Data.Data[0];

	cgif_addframe(pGIF, &fConfig);

	cgif_close(pGIF);
}

void WriteAnimatedGIFFile(const std::filesystem::path& OutputPath, std::string FileName, std::vector<FrameInfo>& ImageData, int Width, int Height, bool Loop = true)
{
	std::filesystem::path OutputFile = OutputPath / (FileName + ".gif");
	auto OutputFileStr = OutputFile.string();

	CGIF_Config	gConfig;
	memset(&gConfig, 0, sizeof(CGIF_Config));
	gConfig.width = Width;
	gConfig.height = Height;
	gConfig.path = OutputFileStr.c_str();
	gConfig.attrFlags = CGIF_ATTR_IS_ANIMATED | CGIF_ATTR_NO_GLOBAL_TABLE;
	if (!Loop)
		gConfig.attrFlags |= CGIF_ATTR_NO_LOOP;

	auto pGIF = cgif_newgif(&gConfig);
	
	for (auto &Entry : ImageData)
	{
		GIFData Data;
		BuildGIFData(Data, Entry.ImageData, Width, Height);
		
		CGIF_FrameConfig   fConfig;
		memset(&fConfig, 0, sizeof(CGIF_FrameConfig));
		fConfig.pImageData = &Data.Data[0];
		fConfig.delay = Entry.Delay;
		fConfig.pLocalPalette = &Data.ColorTable[0];
		fConfig.numLocalPaletteEntries = (uint16_t)(Data.ColorTable.size() / 3);
		fConfig.attrFlags |= CGIF_FRAME_ATTR_USE_LOCAL_TABLE;

		cgif_addframe(pGIF, &fConfig);
	}

	cgif_close(pGIF);
}

void WriteImageFile(const std::filesystem::path& OutputPath, std::string FileName, Image &ImageData)
{
	WriteGIFFile(OutputPath, FileName, ImageData, ImageData.Width * ImageData.PixelSize, ImageData.Height);
	//WritePPMFile(OutputPath, FileName, ImageData, ImageData.Width * ImageData.PixelSize, ImageData.Height);
}

void WriteAnimatedFile(const std::filesystem::path& OutputPath, std::string FileName, std::vector<FrameInfo> &ImageData, bool Loop)
{
	WriteAnimatedGIFFile(OutputPath, FileName, ImageData, ImageData[0].ImageData.Width * ImageData[0].ImageData.PixelSize, ImageData[0].ImageData.Height, Loop);
}