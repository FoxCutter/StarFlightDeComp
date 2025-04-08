#pragma once
#include <stdint.h>
#include <vector>
#include <filesystem>
#include "String.h"

enum class ScreenType
{
	Default,
	BW,
	CGA,
	Tandy,
	Composite,
	Hercules,
	EGA,
};

// Starflight color constants, they look up a mapped color via the current color map
enum ScreenColor
{
	Black,		// 0
	DkBlue,		// 1
	DkGreen,	// 2
	Green,		// 3
	Red,		// 4
	Violet,		// 5
	Brown,		// 6
	Grey1,		// 7
	Grey2,		// 8
	Blue,		// 9
	LtGreen,	// A
	LtBlue,		// B
	Pink,		// C
	Orange,		// D
	Yellow,		// E
	White,		// F
};

// Returns the number of pixels drawn on the image for each actual pixel
int ScreenPixelSize(ScreenType ScreenMode = ScreenType::Default);

class Image : public std::vector<uint32_t>
{
private:

public:
	int Width;
	int Height;
	int PixelSize;

	Image()
	{
		Height = 0;
		Width = 0;
		PixelSize = 0;
	}

	Image(int width, int height, uint32_t Fill = 0x000000, int pixelSize = ScreenPixelSize())
	{
		Width = width;
		Height = height;
		PixelSize = pixelSize;
		this->resize((Width * PixelSize) * Height, Fill);
	}

	void SetPoint(int PosX, int PosY, uint32_t Color);
	void SetPoint(int PosX, int PosY, int Offset, uint32_t Color);

	void DrawLine(int StartX, int StartY, int DestX, int DestY, uint32_t Color);
	void DrawCircle(int StartX, int StartY, int Radius, uint32_t Color, bool Filled);
};

// Converts a color index to a color
uint32_t GetColor(uint8_t ColorIndex, ScreenType ScreenMode = ScreenType::Default);

// Converts the screen color enum to a color
uint32_t GetScreenColor(ScreenColor ColorNumber, ScreenType ScreenMode = ScreenType::Default);

// COnverts a screen color enum to a color index
uint8_t ScreenColorToColorIndex(ScreenColor ColorNumber, ScreenType ScreenMode = ScreenType::Default);

// Coverts file colors to a color index
uint8_t FileColorToColorIndex(uint8_t ColorNumber, ScreenType ScreenMode = ScreenType::Default);

// Set a point, handling any pixel splitting if needed
void SetPoint(Image& ImageData, int xPos, int yPos, uint8_t ColorIndex);





// ==========================================







struct FrameInfo
{
	Image ImageData;
	int Delay = 0;
};


void DrawBitmapAsText(std::ofstream& Output, uint32_t Address, int Width, int Height, bool Direct = true, char Off = '_', char On = '#');

void Read16ColorImageFile(uint32_t Address, Image& ImageData, int ImageWidth = 0, int ImageHeight = 0, int xPos = 0, int yPos = 0, bool WriteBlack = true);
void ReadUpsideDown16ColorImageFile(uint32_t Address, Image& ImageData, int ImageWidth = 0, int ImageHeight = 0, int xPos = 0, int yPos = 0);
int ReadBitmapImage(uint32_t Address, Image& ImageData, int ImageWidth, int ImageHeight, uint8_t ColorIndex, int xPos = 0, int yPos = 0, bool Direct = true);
int ReadRLEBitmapImage(uint32_t Address, Image& ImageData, uint32_t Length, int ImageWidth, int ImageHeight, uint8_t ColorIndex, bool Bright = false, int xPos = 0, int yPos = 0);

void WriteImageFile(const std::filesystem::path& OutputPath, std::string FileName, Image& ImageData);
void WriteAnimatedFile(const std::filesystem::path& OutputPath, std::string FileName, std::vector<FrameInfo>& ImageData, bool Loop = true);