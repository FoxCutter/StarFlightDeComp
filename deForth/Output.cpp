#include "Output.h"

#include <iostream>
#include <fstream>

#include "StarflightData.h"
#include "Files.h"
#include "Instance.h"
#include "Images.h"
#include "Rehydrate.h"

extern uint16_t UserBase;


std::string GetRace(int Type);
ForthWord FindValidWord(uint16_t Value);



void DumpHex(std::ofstream& Output, uint8_t* Buffer, int BufferStart, int BufferLength, int DisplayOffset = 0, int LineOffset = 0, bool Padding = false)
{
	int Pos = 0;

	std::string Hex;
	std::string Text;

	Hex += std::format("@{:04X} ", DisplayOffset - Pos);

	if (Padding)
	{
		Pos = DisplayOffset % 16;
		for (int x = 0; x < Pos; x++)
		{
			Hex += "   ";
			Text += " ";
		}
	}

	for (int x = 0; x < BufferLength; x++, Pos++)
	{
		if (Pos != 0 && Pos % 16 == 0)
		{
			for (int x = 0; x < LineOffset; x++)
				Output << " ";

			Output << Hex << "  " << Text << "\n";

			Hex = std::format("@{:04X} ", DisplayOffset + x);
			Text = "";
		}

		uint8_t Val = Buffer[BufferStart + x];

		Hex += std::format("{:02X} ", Val);

		if (Val >= 0x20 && Val < 0x7F)
		{
			Text += Val;
		}
		else
		{
			Text += ".";
		}
	}

	if (Text.length() != 0)
	{
		while (Pos % 16 != 0)
		{
			Hex += "   ";
			Text += " ";
			Pos++;
		};
	}

	for (int x = 0; x < LineOffset; x++)
		Output << " ";
	Output << Hex << "  " << Text << "\n";

}

void DumpWideHex(std::ofstream& Output, uint8_t* Buffer, int BufferStart, int BufferLength, int DisplayOffset = 0)
{
	int Pos = DisplayOffset % 32;

	std::string Hex = std::format("@{:04X} ", DisplayOffset - Pos);
	std::string Text;


	for (int x = 0; x < Pos; x++)
	{
		Hex += "00 ";
		Text += ".";
	}

	for (int x = 0; x < BufferLength; x++, Pos++)
	{
		if (Pos != 0 && Pos % 32 == 0)
		{
			Output << Hex << "  " << Text << "\n";

			Hex = std::format("@{:04X} ", DisplayOffset + x);
			Text = "";
		}
		else if (Pos != 0 && Pos % 16 == 0)
		{
			Hex += "- ";
			Text += " - ";
		}

		uint8_t Val = Buffer[BufferStart + x];

		Hex += std::format("{:02X} ", Val);

		if (Val >= 0x20 && Val < 0x7F)
		{
			Text += Val;
		}
		else
		{
			Text += ".";
		}
	}

	if (Text.length() != 0)
	{
		while (Pos % 32 != 0)
		{
			if (Pos != 0 && Pos % 16 == 0)
			{
				Hex += "- ";
				Text += " - ";
			}

			Hex += "__ ";
			Text += " ";
			Pos++;
		};
	}

	Output << Hex << "  " << Text << "\n";

}

void DumpKnownFields(const std::filesystem::path& OutputPath, std::string FileName)
{
	std::filesystem::path OutputFile = OutputPath / (FileName + ".txt");

	std::ofstream Output;
	Output.open(OutputFile, std::ios::binary);

	if (!Output.is_open())
		abort();

	std::sort(KnowFields.begin(), KnowFields.end(), [](const FieldInfo& lhs, const FieldInfo& rhs) {
		if (lhs.Type != rhs.Type)
			return lhs.Type < rhs.Type;

		else if (lhs.File != rhs.File)
			return lhs.File < rhs.File;

		else
			return lhs.Offset < rhs.Offset;
		});


	for (auto Field : KnowFields)
	{
		auto File = FindFileID(Field.File);
		std::string OverlayName;
		if (Field.Overlay != 0)
			OverlayName = Files[Field.Overlay].Name;

		if (Field.Type == 'I' && Field.Offset >= 11)
			Output << std::format("IFIELD  {:16} {:25} File: {:16} Offset: {:3} ({:3}) Length: {:3}\n", OverlayName, Field.Name, File.Name, Field.Offset - 11, Field.Offset, Field.Length);

		else
			Output << std::format("{}FIELD  {:16} {:25} File: {:16} Offset: {:3}       Length: {:3}\n", Field.Type, OverlayName, Field.Name, File.Name, Field.Offset, Field.Length);
	}

	Output.close();
}


void DumpRenames(const std::filesystem::path& OutputPath, std::string FileName)
{
	std::filesystem::path OutputFile = OutputPath / (FileName + ".txt");

	std::ofstream Output;
	Output.open(OutputFile);

	if (!Output.is_open())
		abort();

	for (auto& File : Files)
	{
		if (File.second.Type == FileType::Main || File.second.Type == FileType::Overlay || File.second.Type == FileType::Video)
		{
			LoadOverlay(File.first);

			for (auto& Entry : Files[CurrentOverlay].CodeFields)
			{
				if (HasFlag(Entry.Flags, WordFlags::Nameless) || HasFlag(Entry.Flags, WordFlags::Truncated))
				{
					Output << "// Codefield\n";
					if (CurrentOverlay != 0)
						Output << GetOverlayName(Entry.BaseAddress);

					Output << ", " << Entry.Name << ", Rename, " << Entry.Name << ", , 0\n";
				}
			}

			for (auto& Entry : Files[CurrentOverlay].Words)
			{
				if (HasFlag(Entry.Flags, WordFlags::Nameless) || HasFlag(Entry.Flags, WordFlags::Truncated))
				{
					if (CurrentOverlay != 0)
						Output << GetOverlayName(Entry.BaseAddress);

					Output << ", " << Entry.Name << ", Rename, " << Entry.Name << ", , 0\n";
				}
			}

			UnloadOverlay();
		}
	}

	Output.close();
}

void DumpGeneric(const std::filesystem::path& OutputPath, std::string FileName, FileInformation& File)
{
	std::filesystem::path OutputFile = OutputPath / (FileName + ".txt");

	std::ofstream Output;
	Output.open(OutputFile, std::ios::binary);

	if (!Output.is_open())
		abort();

	uint32_t Address = 0;

	Output << std::format("{} Record Length: {} Record Count: {}\n\n", File.Name, File.RecordLength, File.RecordCount);

	for (int x = 0; x < File.RecordCount; x++)
	{
		Output << std::format("Record #{} @{:06X}\n", x, Address);
		DumpHex(Output, GetPtrDirect(0), File.Address + Address, File.RecordLength, File.Address + Address);
		Output << "\n";

		Address += File.RecordLength;

		// Records can't cross block boundries, so adjust its position. 
		if (0x400 - ((File.Address + Address) % 0x400) < File.RecordLength)
		{
			Address += 0x400 - ((File.Address + Address) % 0x400);
		}
	}

	if (Address < File.Length)
	{
		DumpHex(Output, GetPtrDirect(0), File.Address + Address, File.Length - Address, Address);
		Output << "\n";

	}


	Output.close();
}

void DumpStringData(const std::filesystem::path& OutputPath, std::string FileName, FileInformation& File, char Trim, bool Length)
{
	std::filesystem::path OutputFile = OutputPath / (FileName + ".txt");

	std::ofstream Output;
	Output.open(OutputFile, std::ios::binary);

	if (!Output.is_open())
		abort();

	//uint32_t Address = File.Address;

	Output << std::format("{} Record Length: {} Record Count: {}\n\n", File.Name, File.RecordLength, File.RecordCount);

	std::vector<std::string> Entries = ReadAllStringRecords(File, Trim, Length);

	for (int x = 0; x < Entries.size(); x++)
	{
		Output << std::format("Record #{:<3} : {}\n", x, Entries[x]);
	}

	Output.close();
}

void DumpMain(const std::filesystem::path& OutputPath, std::string FileName, FileInformation& File)
{
	std::filesystem::path OutputFile = OutputPath / (FileName + ".hex");

	std::ofstream Output;
	Output.open(OutputFile);

	if (!Output.is_open())
		abort();

	DumpWideHex(Output, GetPtr(0), File.Address, File.Length, 0);

	Output.close();
}

void DumpOverlay(const std::filesystem::path& OutputPath, std::string FileName, FileInformation& File)
{
	std::filesystem::path OutputFile = OutputPath / (FileName + ".hex");

	std::ofstream Output;
	Output.open(OutputFile);

	if (!Output.is_open())
		abort();

	DumpWideHex(Output, GetPtrDirect(0), File.Address, File.Length, File.Base);

	Output.close();
}



void DumpDirectory(const std::filesystem::path& OutputPath, std::string FileName, FileInformation& File)
{
	std::filesystem::path OutputFile = OutputPath / (FileName + ".txt");

	std::ofstream Output;
	Output.open(OutputFile);

	if (!Output.is_open())
		abort();

	std::vector<DirectoryInfoEntry> DirectoryInfo = ReadAllRecords<DirectoryInfoEntry>(File);

	Output << " ID  NAME         Disk     Recs      Len  InstLen   Start   End\n";

	for (int x = 0; x <= DirectoryInfo.size(); x++)
	{
		std::string DirName;
		DirName = std::string_view(DirectoryInfo[x].Name, 12);

		if (DirName == "            ")
			break;

		Output << std::format("{:3}  {} {:4} {:8} {:8} {:8}    {:04X}   {:04X}\n", x, DirName, DirectoryInfo[x].Disk, DirectoryInfo[x].RecodCount, DirectoryInfo[x].RecordLen, DirectoryInfo[x].InstanceLen, DirectoryInfo[x].Start, DirectoryInfo[x].End);
	}


	{
		uint32_t Signature = ReadDoubleWordDirect(0x0FF0);
		uint16_t TimeStamp = ReadWordDirect(0x0FF4);
		uint16_t CheckSum = ReadWordDirect(0x0FF6);
		uint16_t SaveFlag = ReadWordDirect(0x0FF8);
		uint16_t Version = ReadWordDirect(0x0FFA);

		Output << std::format("\n\nSTARA Signature: {:08X} Timestamp: {:04X} Checksum: {:04X} SaveFlag: {} Version: {:04X}\n", Signature, TimeStamp, CheckSum, SaveFlag, Version);
	}
	{
		uint32_t Signature = ReadDoubleWordDirect(StarBAddress + 0x0FF0);
		uint16_t TimeStamp = ReadWordDirect(StarBAddress + 0x0FF4);
		uint16_t CheckSum = ReadWordDirect(StarBAddress + 0x0FF6);
		uint16_t SaveFlag = ReadWordDirect(StarBAddress + 0x0FF8);
		uint16_t Version = ReadWordDirect(StarBAddress + 0x0FFA);

		Output << std::format("STARB Signature: {:08X} Timestamp: {:04X} Checksum: {:04X} SaveFlag: {} Version: {:04X}\n", Signature, TimeStamp, CheckSum, SaveFlag, Version);
	}

	Output.close();
}

//bool isInside(int circle_x, int circle_y, int rad, int x, int y)
//{
//	// Compare radius of circle with distance 
//	// of its center from given point
//	if ((x - circle_x) * (x - circle_x) +
//		(y - circle_y) * (y - circle_y) <= rad * rad)
//		return true;
//	else
//		return false;
//}

void DumpVessel(const std::filesystem::path& OutputPath, std::string FileName, FileInformation& File)
{
	std::filesystem::path OutputFile = OutputPath / (FileName + ".txt");

	std::vector<VesselData> Vessles = ReadAllRecords<VesselData>(File);

	std::ofstream Output;
	Output.open(OutputFile, std::ios::binary);

	if (!Output.is_open())
		abort();

	auto Vesblt = FindFile("VES-BLT");
	std::vector<ElementData> Elements = ReadAllRecords<ElementData>(FindFile("ELEMENT"));

	uint8_t ColorIdx[] = { 0x44, 0xCC, 0xDD, 0x33, 0x77, 0x99, 0x88, 0xAA, 0x55 };

	for (int x = 1; x < Vessles.size(); x++)
	{
		//Image ImageData(41, 55, ScreenPixelSize(), LookupColor(ScreenColor::Violet));

		uint32_t ImageAddress = Vessles[x].Image.Full();
		ImageAddress -= (Vesblt.Address - StarBAddress);

		uint16_t Length = ReadWordDirect(Vesblt.Address + ImageAddress);
		//ReadRLEBitmapImage(Vesblt.Address + ImageAddress + 2, ImageData, Length - 2, 41, 55, 0x0);

		Output << std::format("Ship: #{}\n", x);
		Output << std::format("  Species: {}\n", GetRace(Vessles[x].Species));
		Output << std::format("  Class: {}   Speed: {}   Mass: {}   Max Temp: {}\n", Vessles[x].Class, Vessles[x].Speed, Vessles[x].Mass, Vessles[x].TempMax);
		Output << std::format("  Shields: {}   Armor: {}\n", Vessles[x].Shield, Vessles[x].Armor);
		Output << std::format("  Missile: {}   Laser: {}   Plasma: {}   Fire Rate: {}   Power: {}\n", Vessles[x].MissileClass, Vessles[x].LaserClass, Vessles[x].PlasmaClass, Vessles[x].RateOfFire, Vessles[x].Power);
		Output << std::format("  Endurium Debris: {}\n", Vessles[x].EnduriumDebris);
		Output << std::format("  Element 1: {:16}   Debris: {}\n", TrimString(std::string(Elements[Vessles[x].Element[0]].Name, 16), '.'), Vessles[x].Debris[0]);
		Output << std::format("  Element 2: {:16}   Debris: {}\n", TrimString(std::string(Elements[Vessles[x].Element[1]].Name, 16), '.'), Vessles[x].Debris[1]);
		Output << std::format("  Element 3: {:16}   Debris: {}\n", TrimString(std::string(Elements[Vessles[x].Element[2]].Name, 16), '.'), Vessles[x].Debris[2]);
		Output << std::format("  Directional: {}   Laser Hit: {}   Missile Hit: {}\n", Vessles[x].Directional, Vessles[x].LaserHitFlag, Vessles[x].MissleHitFlag);
		Output << std::format("  Ship Base: {:06X}\n", ImageAddress);
		for (int y = 0; y < 7; y++)
		{
			Output << std::format("   Sensors {}:  ", y);
			if (Vessles[x].SensorData[y][0] == 0xFF)
			{
				Output << "None";
			}
			else
			{
				Output << std::format("Type: {}  ", Vessles[x].SensorData[y][0] == 0 ? "Power" : "Life ");
				if (Vessles[x].SensorData[y][1] == 0)
				{
					Output << std::format(" Shape: Rectangle  Top: {:<3}  Left: {:<3}  Bottom: {:<3}  Right: {:<3}", Vessles[x].SensorData[y][3], Vessles[x].SensorData[y][2], Vessles[x].SensorData[y][5], Vessles[x].SensorData[y][4]);
				}
				else
				{
					Output << std::format(" Shape: Circle  Y: {:<3}  X: {:<3}  Radius: {:<3}", Vessles[x].SensorData[y][3], Vessles[x].SensorData[y][2], Vessles[x].SensorData[y][4]);
				}
			}

			Output << "\n";

			//if (x != 0 && Vessles[x].SensorData[y][0] != 0xFF)
			//{
			//	if (Vessles[x].SensorData[y][1] == 1)
			//	{
			//		int Color = 0x0;

			//		int Radius = Vessles[x].SensorData[y][4];
			//		int DoubleRadius = Radius * 2;

			//		int CenterX = Vessles[x].SensorData[y][2];
			//		int CenterY = 54 - Vessles[x].SensorData[y][3];

			//		int Top = CenterY - Radius;
			//		int Left = CenterX - Radius;

			//		for (int z = 0; z < (DoubleRadius * 36); z++)
			//		{
			//			if (Vessles[x].SensorData[y][0] == 1)
			//				Color = ColorIdx[3 + std::rand() % 3];
			//			else
			//				Color = ColorIdx[(std::rand() % 3)];

			//			int XPos = -1;
			//			int YPos = -1;
			//			while (!isInside(CenterX, CenterY, Radius, XPos, YPos))
			//			{
			//				XPos = Left + (std::rand() % DoubleRadius);
			//				YPos = Top + (std::rand() % DoubleRadius);
			//			}

			//			SetPoint(ImageData, XPos, YPos, Color);
			//		}
			//	}
			//	else
			//	{
			//		int Color = 0x0;

			//		int Top = 54 - Vessles[x].SensorData[y][3];
			//		int Left = Vessles[x].SensorData[y][2];
			//		int Bottom = 54 - Vessles[x].SensorData[y][5];
			//		int Right = Vessles[x].SensorData[y][4];
			//		int Height = Bottom - Top + 1;
			//		int Width = Right - Left + 1;


			//		for (int z = 0; z < (Width * Height); z++)
			//		{
			//			if (Vessles[x].SensorData[y][0] == 1)
			//				Color = ColorIdx[3 + std::rand() % 3];
			//			else
			//				Color = ColorIdx[(std::rand() % 3)];


			//			int XPos = Left + (std::rand() % Width);
			//			int YPos = Top + (std::rand() % Height);

			//			SetPoint(ImageData, XPos, YPos, Color);
			//		}
			//	}
			//}
		}

		Output << "\n";
		//WriteImageFile(OutputPath / "img", std::format("{}_SCAN_{:02}", FileName, x), ImageData);
	}

	Output.close();

}

void DumpArtifact(const std::filesystem::path& OutputPath, std::string FileName, FileInformation& File)
{
	std::filesystem::path OutputFile = OutputPath / (FileName + ".txt");
	std::ofstream Output;
	Output.open(OutputFile);

	if (!Output.is_open())
		abort();

	auto AnalyzeTextFile = FindFile("ANALYZE-TEXT");
	std::vector<AnalyzeTextData> AnalyzeText = ReadAllRecords<AnalyzeTextData>(AnalyzeTextFile);

	uint32_t Address = File.Address;

	for (int x = 0; x < File.RecordCount; x++)
	{
		Output << std::format("Artifact #{}\n", x);
		ArtifactData Value = *reinterpret_cast<ArtifactData*>(GetPtrDirect(Address));
		Output << "  Name : " << TrimString(std::string(Value.Name, 24), '.') << "\n";
		Output << std::format("  Volume: {}\n", Value.Volume);
		Output << std::format("  Value: {}\n", Value.Value);
		Output << std::format("  Analyzed: {}\n", Value.Analyzed);
		Output << std::format("  Analyze Text ID: {}\n", Value.AnalyzeTextID);

		if (x != 0)
		{
			auto Text = AnalyzeText[Value.AnalyzeTextID];
			for (int y = 0; y < 5; y++)
			{
				Output << std::format("    Line #{}: {}\n", y + 1, TrimString(std::string(Text.Text[y], 38)));
			}
		}

		Output << "\n";

		Address += File.RecordLength;
		Address = BlockCorrection(Address, File.RecordLength);

	}
}

void DumpAnalyzeText(const std::filesystem::path& OutputPath, std::string FileName, FileInformation& File)
{
	std::filesystem::path OutputFile = OutputPath / (FileName + ".txt");
	std::ofstream Output;
	Output.open(OutputFile);

	if (!Output.is_open())
		abort();

	std::vector<AnalyzeTextData> AnalyzeText = ReadAllRecords<AnalyzeTextData>(File);

	for (int x = 0; x < AnalyzeText.size(); x++)
	{
		Output << std::format("Anayze Text #{}\n", x);
		for (int y = 0; y < 5; y++)
		{
			Output << std::format("    Line #{}: {}\n", y + 1, TrimString(std::string(AnalyzeText[x].Text[y], 38)));
		}

		Output << "\n";
	}
}

void DumpButtons(const std::filesystem::path& OutputPath, std::string FileName, FileInformation& File)
{
	std::filesystem::path OutputFile = OutputPath / (FileName + ".txt");

	std::ofstream Output;
	Output.open(OutputFile, std::ios::binary);

	if (!Output.is_open())
		abort();

	std::vector<ButtonData> Buttons = ReadAllRecords<ButtonData>(File);

	for (int x = 0; x < Buttons.size(); x++)
	{
		Output << std::format("Button Text #{}\n", x);
		Output << std::format("  Button Count: {}\n", Buttons[x].Count);
		for (int y = 0; y < 6; y++)
		{
			Output << std::format("  Button #{}: {}\n", y + 1, TrimString(std::string(Buttons[x].Text[y], 12)));
		}

		Output << "\n";
	}
}

void DumpRegions(const std::filesystem::path& OutputPath, std::string FileName, FileInformation& File)
{
	std::filesystem::path OutputFile = OutputPath / (FileName + ".txt");

	std::ofstream Output;
	Output.open(OutputFile, std::ios::binary);

	if (!Output.is_open())
		abort();

	uint32_t Address = File.Address;

	std::vector<RegionData> Regions = ReadAllRecords<RegionData>(File);

	for (int x = 0; x < Regions.size(); x++)
	{
		Output << std::format("Region #{}\n", x);
		Output << std::format("  Name: {}\n", GetInstanceRecordValue(Regions[x].Name.Full()));
		Output << std::format("  LSeed Adjustment: {}\n", Regions[x].LSeedAdjustment);

		Output << "\n";
	}

	Output.close();
}

std::string GetPlanetType(uint8_t Type)
{
	switch (Type)
	{
	default:
	case 0:
		return "Crystal";

	case 1:
		return "Gas";

	case 2:
		return "Liquid";

	case 3:
		return "Frozen";

	case 4:
		return "Molton";

	case 5:
		return "Rock";
	}
}

void DumpPlanetData(const std::filesystem::path& OutputPath, std::string FileName, FileInformation& File)
{
	std::filesystem::path OutputFile = OutputPath / (FileName + ".txt");

	std::ofstream Output;
	Output.open(OutputFile, std::ios::binary);

	if (!Output.is_open())
		abort();

	std::vector<ElementData> Elements = ReadAllRecords<ElementData>(FindFile("ELEMENT"));
	uint32_t Address = File.Address;

	for (int x = 0; x < File.RecordCount; x++)
	{
		PlanetData Value = *reinterpret_cast<PlanetData*>(GetPtrDirect(Address));
		if (x != 0)
		{
			Output << std::format("PLANET {:2} @{:04X}:\n", x, Address);
			Output << std::format("  Surface Type: {}\n", GetPlanetType(Value.SurfaceType));
			Output << std::format("  Mass Type: {}\n", Value.MassType);
			Output << std::format("  Planet Mass: {}\n", Value.Mass);
			Output << std::format("  Mineral Density: {}\n", Value.MineralDensity);
			Output << std::format("  LSeed (Planet): ${:02X}  Region Seed: ${:04X}  Golbal Seed: ${:04X}\n", Value.LSeed, Value.RegionSeed, Value.GlobalSeed);
			Output << std::format("  Seed4: ${:02X}  Seed5 : ${:02X}\n", Value.Seed4, Value.Seed5);

			Output << "\n";
		}

		Address += File.RecordLength;
		Address = BlockCorrection(Address, File.RecordLength);

	}



	Output.close();
}

void DumpColorMap(const std::filesystem::path& OutputPath, std::string FileName, FileInformation& File)
{
	std::filesystem::path OutputFile = OutputPath / (FileName + ".txt");

	std::ofstream Output;
	Output.open(OutputFile);

	if (!Output.is_open())
		abort();

	std::vector<CMAPData> CMAP = ReadAllRecords<CMAPData>(File);

	std::string Type = "RGB";
	for (int x = 0; x < CMAP.size(); x++)
	{
		if (x == 5)
			Type = "Composite";

		if (x == 10)
			Type = "EGA";

		Output << std::format("Color Map #{}:  Type: {}  Planet: {}\n", x, Type, GetPlanetType((x % 5) + 1));

		for (int y = 0; y < 8; y++)
		{
			if (y != 0)
				Output << ", ";
			else
				Output << "  ";

			Output << std::format("{:02X}", CMAP[x].Color[y][0]);

		}

		Output << "\n\n";
	}

	Output.close();
}

void DumpEarth(const std::filesystem::path& OutputPath, std::string FileName, FileInformation& File)
{
	std::filesystem::path OutputFile = OutputPath / (FileName + ".txt");

	std::ofstream Output;
	Output.open(OutputFile);

	if (!Output.is_open())
		abort();

	int Address = File.Address;
	int Length = File.Length;

	// The earth data is upside down in the file. 
	for (int x = 23; x >= 0; x--)
	{
		for (int pos = 0; pos < 48; pos++)
		{
			int8_t Val = ReadByteDirect(Address + (x * 48) + pos);
			// The values seemed to be heights
			switch (Val)
			{
			case -110:	// water
				Output << ".";
				break;

			case 16:	// low
				Output << "1";
				break;

			case 32:	// med
				Output << "2";
				break;

			case 48:	// High
				Output << "3";
				break;

			case 96:	// Mountians
				Output << "4";
				break;

			case 120:	// Ice
				Output << "5";
				break;

			default:
				Output << std::format("! ", (uint8_t)Val);
				break;
			}
		}

		Output << "\n";
	}

	Output.close();

}

void DumpElements(const std::filesystem::path& OutputPath, std::string FileName, FileInformation& File)
{
	std::filesystem::path OutputFile = OutputPath / (FileName + ".txt");

	std::ofstream Output;
	Output.open(OutputFile);

	if (!Output.is_open())
		abort();

	std::vector<ElementData> Elements = ReadAllRecords<ElementData>(File);

	for (int x = 1; x < Elements.size(); x++)
	{
		Output << std::format("Element #{}\n", x);
		Output << "  Name: " << TrimString(std::string(Elements[x].Name, 16), '.') << "\n";
		Output << "  Value: " << Elements[x].Value << "\n";
		Output << "  MW: " << (int)Elements[x].Weight << "\n";
		Output << "  Volume: " << (int)Elements[x].Volume << "\n";

		Output << "\n";
	}

	Output.close();

}

void DumpCompounds(const std::filesystem::path& OutputPath, std::string FileName, FileInformation& File)
{
	std::filesystem::path OutputFile = OutputPath / (FileName + ".txt");

	std::ofstream Output;
	Output.open(OutputFile);

	if (!Output.is_open())
		abort();

	std::vector<CompoundData> Compounds = ReadAllRecords<CompoundData>(File);

	for (int x = 0; x < Compounds.size(); x++)
	{
		Output << std::format("Compound #{}: {}\n", x, GetInstanceRecordValue(Compounds[x].Name.Full()));
		Output << "\n";
	}

	Output.close();

}

void DumpCrewmember(const std::filesystem::path& OutputPath, std::string FileName, FileInformation& File)
{
	std::filesystem::path OutputFile = OutputPath / (FileName + ".txt");

	std::ofstream Output;
	Output.open(OutputFile);

	if (!Output.is_open())
		abort();

	std::vector<CrewMemberData> CrewMembers = ReadAllRecords<CrewMemberData>(File);

	for (int x = 0; x < CrewMembers.size() - 1; x++)
	{
		Output << std::format("Crewmember #{}\n", x);
		Output << std::format("  Species: {}\n", std::string(CrewMembers[x].Species, CrewMembers[x].SpeciesLen));
		Output << std::format("  Durability: {}\n", CrewMembers[x].Durability);
		Output << std::format("  Learning Rate: {}\n", CrewMembers[x].LearningRate);
		Output << std::format("  Inital Stats: {:3} {:3} {:3} {:3} {:3}\n", CrewMembers[x].Stats[0], CrewMembers[x].Stats[1], CrewMembers[x].Stats[2], CrewMembers[x].Stats[3], CrewMembers[x].Stats[4]);
		Output << std::format("  Max Stats:    {:3} {:3} {:3} {:3} {:3}\n", CrewMembers[x].MaxStats[0], CrewMembers[x].MaxStats[1], CrewMembers[x].MaxStats[2], CrewMembers[x].MaxStats[3], CrewMembers[x].MaxStats[4]);

		Output << "\n";
	}

	Output.close();

}

void DumpPStats(const std::filesystem::path& OutputPath, std::string FileName, FileInformation& File)
{
	std::filesystem::path OutputFile = OutputPath / (FileName + ".txt");

	std::ofstream Output;
	Output.open(OutputFile);

	if (!Output.is_open())
		abort();

	std::vector<PStatsData> PStats = ReadAllRecords<PStatsData>(File);

	for (int x = 0; x < PStats.size(); x++)
	{
		Output << std::format("PSAT #{}\n", x);
		Output << std::format("  Type: {}\n", TrimString(std::string(PStats[x].Type, 9)));
		Output << std::format("  Avg Height (dm): {}\n", PStats[x].AvgHeight);
		Output << std::format("  Avg Weight (kg): {}\n", PStats[x].AvgWeight);
		Output << "\n";
	}

	Output.close();

}


void DumpFullScreenImage(const std::filesystem::path& OutputPath, std::string FileName, FileInformation& File)
{
	int Width = 160;
	int Height = 200;

	Image ImageData(Width, Height);

	// Full screen images are upside down.
	ReadUpsideDown16ColorImageFile(File.Address, ImageData);

	WriteImageFile(OutputPath, FileName, ImageData);
}

void DumpCrewPic(const std::filesystem::path& OutputPath, std::string FileName, FileInformation& File)
{
	int Width = 54;
	int Height = 100;

	Image ImageData(Width, Height);

	int Length = ReadWordDirect(File.Address);

	ReadRLEBitmapImage(File.Address + 2, ImageData, Length - 2, Width, Height, 0xF);

	WriteImageFile(OutputPath, FileName, ImageData);
}

void DumpLayerPic(const std::filesystem::path& OutputPath, std::string FileName, FileInformation& File)
{
	int Address = File.Address;

	uint16_t DataLength = ReadWordDirect(Address);
	Address += 2;

	int Height = ReadByteDirect(Address);
	Address++;

	int Width = ReadByteDirect(Address);
	Address++;

	uint8_t LayerCount = ReadByteDirect(Address);
	Address++;

	Image ImageData(Width, Height);

	for (int x = 0; x < LayerCount; x++)
	{
		// Image ImageData2(Width, Height);

		uint16_t Length = ReadWordDirect(Address);
		Address += 2;

		uint8_t ColorIdx = FileColorToColorIndex(ReadByteDirect(Address));
		Address++;

		uint8_t EncodingType = ReadByteDirect(Address);
		Address++;

		if (EncodingType == 0x01)
		{
			uint16_t RLELength = ReadWordDirect(Address);
			ReadRLEBitmapImage(Address + 2, ImageData, RLELength, Width, Height, ColorIdx, true);
			//ReadRLEBitmapImage(Address + 2, ImageData2, RLELength, Width, Height, ColorIdx, true);
		}
		else
		{
			ReadBitmapImage(Address, ImageData, Width, Height, ColorIdx);
			//ReadBitmapImage(Address, ImageData2, Width, Height, ColorIdx);
		}

		//WriteImageFile(OutputPath, std::format("{}_{:02}", FileName, x) , ImageData2);

		Address += Length - 4;
	}


	WriteImageFile(OutputPath, FileName, ImageData);
}

void DumpIcons(const std::filesystem::path& OutputPath, std::string FileName, FileInformation& File)
{
	// Each icon record has two entries, 8x8 with a color, that overlay each other

	// Using a custom background color as black and white are both common colors in icons
	Image ImageData(10, File.RecordCount * 9, 0x454545);

	int Address = File.Address;

	for (int x = 0; x < File.RecordCount; x++)
	{
		uint8_t ColorIndex = FileColorToColorIndex(ReadByteDirect(Address));
		Address++;
		Address += ReadBitmapImage(Address, ImageData, 8, 8, ColorIndex, 1, x * 9);

		ColorIndex = FileColorToColorIndex(ReadByteDirect(Address));
		Address++;
		Address += ReadBitmapImage(Address, ImageData, 8, 8, ColorIndex, 1, x * 9);


		Address = BlockCorrection(Address, File.RecordLength);
	}

	WriteImageFile(OutputPath, FileName, ImageData);
}

void DumpSystemIcon(const std::filesystem::path& OutputPath, std::string FileName, FileInformation& File)
{
	int Address = File.Address;

	uint8_t Height = ReadByteDirect(Address);
	Address++;

	uint8_t Width = ReadByteDirect(Address);
	Address++;

	uint8_t LayerCount = ReadByteDirect(Address);
	Address++;

	Image ImageData(Width, Height);

	for (int x = 0; x < LayerCount; x++)
	{
		uint8_t ColorIndex = FileColorToColorIndex(ReadByteDirect(Address));
		Address++;

		Address += ReadBitmapImage(Address, ImageData, Width, Height, ColorIndex);
	}

	WriteImageFile(OutputPath, FileName, ImageData);
}

void DumpVesBlt(const std::filesystem::path& OutputPath, std::string FileName, FileInformation& File)
{
	std::filesystem::path OutputFile = OutputPath / (FileName + ".txt");

	std::vector<VesselData> Vessles = ReadAllRecords<VesselData>(File);

	std::ofstream Output;
	Output.open(OutputFile, std::ios::binary);

	if (!Output.is_open())
		abort();

	uint32_t Address = File.Address;
	int ImageNum = 1;

	while (Address < File.Address + File.Length)
	{
		uint16_t Len = ReadWordDirect(Address);
		Address += 2;
		if (Len != 0x2020)
		{
			Output << std::format("Vessel Image: {:<2} @{:06X}\n", ImageNum, Address - 2 - File.Address);

			Image ImageData(41, 55);
			ReadRLEBitmapImage(Address, ImageData, Len, 41, 55, 0xF);

			WriteImageFile(OutputPath / "img", std::format("{}_{:06X}", FileName, Address - 2 - File.Address), ImageData);
			ImageNum++;

			Address += Len;

			Address = BlockCorrection(Address, 100);
		}
		else
		{
			Address = BlockCorrection(Address, 0x400);
		}
	}

	Output.close();
}

//void DumpFont(std::ofstream& Output, uint32_t Address, int Width, int Height)
//{
//	int WordCount = (Width * Height) / 16;
//	if ((Width * Height * 5) % 16 != 0)
//		WordCount++;
//
//	for (int x = 0; x < WordCount; x++)
//	{
//		uint16_t val = ReadWordDirect(Address + (2 * x));
//		Output << std::format("{:04X} ", val);
//	}
//
//	Output << "\n";
//	DrawBitmapAsText(Output, Address, Width, Height);
//	Output << "\n";
//}

void DumpFonts(const std::filesystem::path& OutputPath, std::string FileName, FileInformation& File)
{
	//std::filesystem::path OutputFile = OutputPath / (FileName + ".txt");

	//std::ofstream Output;
	//Output.open(OutputFile, std::ios::binary);

	int ImgHeight = 10 * 3;
	int ImgWidth = (63 * 7);

	Image ImageData(ImgWidth, ImgHeight);

	//if (!Output.is_open())
	//	abort();

	//Output << "Font 1:\n";

	int PosX = 1;
	for (int x = 0; x < 59; x++)
	{
		//Output << std::format(" '{:c}' : 3x5 ", 32 + x);

		uint32_t Address = File.Address + (x * 2);

		//DumpFont(Output, Address, 3, 5);
		ReadBitmapImage(Address, ImageData, 3, 5, 0xF, PosX, 1);

		PosX += 7;
	}

	for (int x = 0; x < 4; x++)
	{
		//Output << std::format(" '{:c}' : 5x5: ", 91 + x);

		uint32_t Address = File.Address + 118 + (x * 4);

		//DumpFont(Output, Address, 5, 5);

		ReadBitmapImage(Address, ImageData, 5, 5, 0xF, PosX, 1);

		PosX += 7;
	}

	//Output << "\n\nFont 2:\n";

	PosX = 1;

	for (int x = 0; x < 59; x++)
	{
		uint8_t Width = ReadByteDirect(File.Address + 0x01E8 + x);

		//Output << std::format(" '{:c}' : {}x7 ", 32 + x, Width);

		uint32_t Address = File.Address + 134 + (x * 6);

		//DumpFont(Output, Address, Width, 7);

		ReadBitmapImage(Address, ImageData, Width, 7, 0xF, PosX, 7);
		PosX += 7;
	}

	//Output << "\n";
	//Output << "Font 3:\n";

	PosX = 1 + (7 * 33);

	for (int x = 0; x < 26; x++)
	{
		uint8_t Width = ReadByteDirect(File.Address + 0x01E8 + x + 33);

		//Output << std::format(" '{:c}' : {}x9 ", 65 + x, Width);

		uint32_t Address = File.Address + 0x223 + (x * 8);

		//DumpFont(Output, Address, Width, 9);

		ReadBitmapImage(Address, ImageData, Width, 9, 0xF, PosX, 15);
		PosX += 7;

	}

	//Output.close();

	WriteImageFile(OutputPath / "img", FileName, ImageData);
}

void DumpPhazes(const std::filesystem::path& OutputPath, std::string FileName, FileInformation& File)
{
	std::filesystem::path OutputFile = OutputPath / (FileName + ".txt");

	std::ofstream Output;
	Output.open(OutputFile, std::ios::binary);

	if (!Output.is_open())
		abort();

	// From [MUSIC:E829] / _phazes_anim in MUSIC
	int Count = 11;
	int xoff = 11;
	int yoff = 199 - 103;
	int pause = 0x01F4;

	Output << std::format("Frame Count: {}\n", Count);
	Output << "Background image: SPLASH\n\n";

	int BaseAddress = File.Address;
	int Offset = 0;

	auto SplashFile = FindFile("SPLASH");

	FrameInfo Frame;
	Frame.ImageData = Image(160, 200);
	ReadUpsideDown16ColorImageFile(SplashFile.Address, Frame.ImageData);
	Frame.Delay = 0;

	std::vector<FrameInfo> Frames;

	for (int x = 0; x < Count; x++)
	{
		int Width = ReadByteDirect(BaseAddress + Offset + 6) * 2; // Width in bytes
		int Height = ReadByteDirect(BaseAddress + Offset + 10);

		// yoff is from the bottom of the image, and xOff is in bytes, so adjust them 
		Output << std::format("Frame {} @{:04X}\n", x, BaseAddress + Offset);
		Output << std::format("  Width {} Height {} \n", Width, Height);
		Output << std::format("  YPos {} XPos {}\n", yoff - Height, xoff * 2);
		Output << std::format("  Pause {} ms\n\n", pause);

		Image ImageData2(Width, Height, 0xFF);
		Read16ColorImageFile(BaseAddress + Offset + 16, ImageData2, Width);
		WriteImageFile(OutputPath / "img", std::format("{}_{:02}", FileName, x), ImageData2);

		FrameInfo CurrentFrame = Frame;
		CurrentFrame.Delay = pause / 10;

		Read16ColorImageFile(BaseAddress + Offset + 16, CurrentFrame.ImageData, Width, Height, xoff * 2, yoff - Height);
		Frames.emplace_back(std::move(CurrentFrame));

		//WriteImageFile(OutputPath, std::format("{}_{:02}", FileName, x), CurrentFrame.ImageData);

		int Size = ((Width / 2) * Height + 16);
		Offset += Size + 512 - (Size % 512);
	}

	Output.close();

	WriteAnimatedFile(OutputPath / "img", FileName, Frames);
}

void DumpGalaxy(const std::filesystem::path& OutputPath, std::string FileName, FileInformation& File)
{
	std::filesystem::path OutputFile = OutputPath / (FileName + ".txt");

	std::ofstream Output;
	Output.open(OutputFile, std::ios::binary);

	if (!Output.is_open())
		abort();

	// From [MUSIC:E926] / _galaxy_anim in MUSIC
	int Count = 13;
	int yoff = 199 - 70;

	// XOFFS Table in MUSIC
	uint16_t xoff[] = { 0x27, 0x27, 0x27, 0x26, 0x26, 0x24, 0x23, 0x22, 0x20, 0x1E, 0x1A, 0x12, 0x0C };
	// PAUSEOFF Table in Music
	uint16_t pause[] = { 0x258, 0x1F4, 0x190, 0x15E, 0x102, 0xFA, 0xC8, 0x96, 0x64, 0x4B, 0x32, 0x23, 0x19 };


	Output << std::format("Frame Count: {}\n", Count);
	Output << "Background image: CREDITS\n\n";

	auto SplashFile = FindFile("CREDITS");

	FrameInfo Frame;
	Frame.ImageData = Image(160, 200);
	ReadUpsideDown16ColorImageFile(SplashFile.Address, Frame.ImageData);
	Frame.Delay = 0;

	std::vector<FrameInfo> Frames;
	Frames.emplace_back(Frame);

	int BaseAddress = File.Address;
	int Offset = 0;

	for (int x = 0; x < Count; x++)
	{
		int Width = ReadByteDirect(BaseAddress + Offset + 6) * 2;
		int Height = ReadByteDirect(BaseAddress + Offset + 10);

		// yoff is from the bottom of the image, and xOff is in bytes, so adjust them 
		Output << std::format("Frame {} @{:04X}\n", x, BaseAddress + Offset);
		Output << std::format("  Width {} Height {}\n", Width, Height);
		Output << std::format("  YPos {} XPos {}\n", yoff - Height, xoff[x] * 2);
		Output << std::format("  Pause {} ms\n\n", pause[x]);

		Image ImageData2(Width, Height, 0xFF);
		Read16ColorImageFile(BaseAddress + Offset + 16, ImageData2, Width);
		WriteImageFile(OutputPath / "img", std::format("{}_{:02}", FileName, x), ImageData2);

		FrameInfo CurrentFrame = Frame;
		CurrentFrame.Delay = pause[x] / 10;

		Read16ColorImageFile(BaseAddress + Offset + 16, CurrentFrame.ImageData, Width, Height, xoff[x] * 2, yoff - Height, false);
		Frames.emplace_back(std::move(CurrentFrame));

		int Size = ((Width / 2) * Height + 16);
		Offset += Size;
	}

	WriteAnimatedFile(OutputPath / "img", FileName, Frames, false);
}


std::string GetStageString(DiscoveryStage Stage)
{
	switch (Stage)
	{
	default:
		return "unknown";

	case DiscoveryStage::Internal:
		return "Internal";

	case DiscoveryStage::External:
		return "External";

	case DiscoveryStage::Dictionaries:
		return "Dictionaries";

	case DiscoveryStage::CodePointers:
		return "CodePointers";

	case DiscoveryStage::Words:
		return "Words";

	case DiscoveryStage::Data:
		return "Data";

	case DiscoveryStage::Memory:
		return "Memory";

	case DiscoveryStage::MemoryGap:
		return "Memory - Gap";

	}
}

std::string GetSubtypeString(WordSubType SubType)
{
	switch (SubType)
	{
	case WordSubType::None:
		return "Data";

	case WordSubType::ByteValue:
		return "Data:Byte";

	case WordSubType::WordValue:
		return "Data:Word";

	case WordSubType::DoubleWordValue:
		return "Data:DoubleWord";

	case WordSubType::Variable:
		return "Data:Variable";

	case WordSubType::TwoWordValue:
		return "Data:2Word";

	case WordSubType::TwoDoubleWordValue:
		return "Data:2DoubleWord";

	case WordSubType::ByteArray:
		return "Data:ByteArray";

	case WordSubType::WordArray:
		return "Data:WordArray";

	case WordSubType::DoubleWordArray:
		return "Data:DoubleWordArray";

	case WordSubType::IAddrArray:
		return "Data:IAaddrArray";

	case WordSubType::OverlayHeader:
		return "Data:OverlayHeader";

	case WordSubType::OverlayJunk:
		return "Data:OverlayJunk";

	case WordSubType::PointerTable:
		return "Data:PointerTable";

	case WordSubType::Gap:
		return "Data:Gap";

	case WordSubType::AField:
		return "Data:AField";

	case WordSubType::IField:
		return "Data:IField";

	case WordSubType::Vocabulary:
		return "Data:Vocabulary";

	case WordSubType::Syn:
		return "Data:Syn";

	case WordSubType::Case:
		return "Data:Case";

	case WordSubType::CaseEx:
		return "Data:CaseEx";

	case WordSubType::Expert:
		return "Data:Expert";

	case WordSubType::ProbabilityArray:
		return "Data:ProbabilityArray";

	case WordSubType::BLT:
		return "Data:BLT";

	default:
		return "Data:Huh?";
	}
}


std::string GetValueString(uint8_t Value, bool Hex = false)
{
	if (Hex)
		return std::format("${:02X}", Value);
	else
		return std::format("{}", Value);
}

std::string GetValueString(uint16_t Value, bool Lookup = true, bool Hex = false)
{
	if (Lookup)
	{
		auto Match = FindValidWord(Value);

		if (Match.IsValid())
		{
			return std::format("${:04X} ( ' {} )", Value, Match.Name);
		}
	}

	if (!Hex && ((int16_t)Value >= -128 && (int16_t)Value < 128))
		return std::format("{}", (int16_t)Value);
	else
		return std::format("${:04X}", Value);
}

std::string GetValueString(uint32_t Value, bool Lookup = true, bool Hex = false)
{
	if (Lookup && ValidInstanceRecord(Value))
	{
		return std::format("${:06X}. ( {} )", Value, GetInstanceRecordValue(Value));
	}

	if (Hex)
		return std::format("${:06X}.", Value);
	else
		return std::format("{}.", Value);
}

void DumpBLT(const std::filesystem::path& OutputPath, std::string FileName, ForthWord& Word, int Width, int Height)
{
	int Count = (Width * Height) / 8;
	if ((Width * Height) % 8 != 0)
		Count++;

	Count = Word.DataLength / Count;
	if (Count == 0)
	{
		std::cout << std::format("WARN: Invalid Heigh/Widith on {}: {} {} Size:{} \n", Word.Name, Height, Width, Word.DataLength);
		Count++;
	}

	Image ImageData(Width, (Height + 1) * Count);

	int Address = Word.DataAddress;
	for (int x = 0; x < Count; x++)
	{
		Address += ReadBitmapImage(Address, ImageData, Width, Height, 0x0F, 0, x * (Height + 1), false);
	}

	WriteImageFile(OutputPath, FileName, ImageData);

}


void DumpData(const std::filesystem::path& OutputPath, std::ofstream& Output, ForthWord& Entry, bool Video)
{
	auto CodePointer = FindCodePointer(Entry.CodePointer);
	if (HasFlag(Entry.Flags, WordFlags::User))
	{
		uint16_t Index = ReadWord(Entry.DataAddress);
		auto XTWord = Find((UserBase + Index));

		Output << std::format("{} {} {} ( @{:04X}={}={}", XTWord.DataLength, CodePointer.Name, Entry.Name, Entry.DataAddress, Index, XTWord.Name);

		if (XTWord.DataLength == 1)
			Output << std::format("={}", ReadByte(XTWord.DataAddress));
		else if (XTWord.DataLength == 2)
			Output << std::format("={}", ReadWord(XTWord.DataAddress));

		Output << " )\n";

	}
	else switch (Entry.SubType)
	{
		case WordSubType::ByteValue:
		{
			uint8_t Value = ReadByte(Entry.DataAddress);
			Output << std::format("{} {} {}\n", GetValueString(Value), CodePointer.Name, Entry.Name);
		}
		break;

		case WordSubType::WordValue:
		{
			uint16_t Value = ReadWord(Entry.DataAddress);
			Output << std::format("{} {} {}\n", GetValueString(Value, Entry.Param[0] != 0 ? false : true), CodePointer.Name, Entry.Name);
		}
		break;

		case WordSubType::DoubleWordValue:	
		{
			uint32_t Value = ReadDoubleWord(Entry.DataAddress);
			Output << std::format("{} {} {}\n", GetValueString(Value, Entry.Param[0] != 0 ? false: true), CodePointer.Name, Entry.Name);
		}
		break;

		case WordSubType::Variable:
		{
			int Address = Entry.DataAddress;
			if (Video)
				Address -= 0xE000;

			Output << std::format("VARIABLE {} ( @{:04X}={})\n", Entry.Name, Entry.DataAddress, GetValueString(ReadWord(Address), Entry.Param[0] != 0 ? false : true));
		}
		break;

		case WordSubType::TwoWordValue:
		{
			uint16_t Value1 = (ReadWord(Entry.DataAddress));
			uint16_t Value2 = (ReadWord(Entry.DataAddress + 2));

			Output << std::format("{} {} {} {}\n", GetValueString(Value2, Entry.Param[0] != 0 ? false : true), GetValueString(Value1, Entry.Param[0] != 0 ? false : true), CodePointer.Name, Entry.Name);
		}		
		break;

		case WordSubType::TwoDoubleWordValue:
		{
			uint32_t IaddreA = ReadDoubleWord(Entry.DataAddress);
			uint32_t IaddreB = ReadDoubleWord(Entry.DataAddress + 4);

			Output << std::format("{} {} {} {}\n", GetValueString(IaddreA, Entry.Param[0] != 0 ? false : true), GetValueString(IaddreB, Entry.Param[0] != 0 ? false : true), CodePointer.Name, Entry.Name);
		}
		break;

		case WordSubType::IField:
		case WordSubType::AField:
		{
			uint8_t FileID = ReadByte(Entry.DataAddress);
			uint8_t Offset = ReadByte(Entry.DataAddress + 1);
			uint8_t Len = ReadByte(Entry.DataAddress + 2);

			auto File = FindFileID(FileID);

			Output << std::format("{} {} {} {} {} ( {} )\n", FileID, Offset, Len, CodePointer.Name, Entry.Name, File.Name);
		}
		break;

		case WordSubType::Syn:
		{
			Output << std::format("{} {} {}\n", CodePointer.Name, Entry.Name, Find(ReadWord(Entry.DataAddress)).Name);
		}			
		break;

		case WordSubType::Vocabulary:
		{
			int Count = Entry.DataLength / 2;
			int Address = Entry.DataAddress;

			if (Entry.Param[1] != 0)
			{
				Count = ReadWord(Address);
				Address += 2;
				Output << std::format("{} ", Count);
			}

			Output << std::format("{} {}\n", CodePointer.Name, Entry.Name);
			Output << "  ";
			for (int x = 0; x < Count; x++)
			{
				if (x != 0)
					Output << ", ";
				Output <<  GetValueString(ReadWord(Address), false, true);
				Address += 2;
			}

			Output << "\n";
		}
		break;

		case WordSubType::CaseEx:
		case WordSubType::WordArray:
		{
			int Count = Entry.DataLength / 2;
			int Address = Entry.DataAddress;

			if (Entry.Param[1] != 0)
			{
				Count = ReadWord(Address);
				Address += 2;
				Output << std::format("{} ", Count);
			}

			Output << std::format("{} {}\n", CodePointer.Name, Entry.Name);

			for (int x = 0; x < Count; x++)
			{
				Output << "  " << GetValueString(ReadWord(Address), Entry.Param[0] != 0 ? false : true) << "\n";
				Address += 2;
			}
		}
		break;

		case WordSubType::DoubleWordArray:
		{
			int Count = Entry.DataLength / 4;
			int Address = Entry.DataAddress;

			if (Entry.Param[1] != 0)
			{
				Count = ReadDoubleWord(Entry.DataAddress);
				Address += 4;
				Output << std::format("{} ", Count);
			}

			Output << std::format("{} {}\n", CodePointer.Name, Entry.Name);

			for (int x = 0; x < Count; x++)
			{
				uint32_t Value = ReadDoubleWord(Entry.DataAddress);
				Output << "  " << GetValueString(Value, Entry.Param[0] != 0 ? false : true) << " , \n";
				Address += 4;
			}
		}
		break;

		case WordSubType::IAddrArray:
		{
			int Count = Entry.DataLength / 3;
			int Address = Entry.DataAddress;

			if (Entry.Param[1] != 0)
			{
				Count = Read3Byte(Entry.DataAddress);
				Address += 3;
				Output << std::format("{} ", Count);
			}

			Output << std::format("{} {}\n", CodePointer.Name, Entry.Name);

			for (int x = 0; x < Count; x++)
			{
				Output << "  " << GetValueString(Read3Byte(Address)) << " , \n";
				Address += 3;
			}
		}
		break;

		case WordSubType::Case:
		{
			Output << std::format("{} {}\n", CodePointer.Name, Entry.Name);

			uint16_t Address = Entry.DataAddress;

			uint16_t Count = ReadWord(Address);
			Address += 2;

			uint16_t Value = ReadWord(Address) - 2;
			Address += 2;

			auto OtherMatch = Find(Value);

			for (int x = 0; x < Count; x++)
			{
				uint16_t CaseValue = ReadWord(Address);
				Address += 2;

				Value = ReadWord(Address) - 2;
				Address += 2;
				auto Match = Find(Value);

				Output << std::format(" {} IS {}\n", CaseValue, Match.Name);
			}

			Output << std::format(" OTHERS {}\n", OtherMatch.Name);
		}
		break;

		case WordSubType::ProbabilityArray:
		{
			int Count = ReadWord(Entry.DataAddress) / 6;

			Output << std::format("{} {} {}\n", Count, CodePointer.Name, Entry.Name);

			uint16_t Address = Entry.DataAddress + 2;
			uint16_t SubAddress = 0;

			for (int x = 0; x < Count; x++)
			{
				uint32_t Type = ReadDoubleWord(Address);
				SubAddress = ReadWord(Address + 4);
				Output << std::format("   ${:06X}", Type);

				if (Type > 0x100)
				{
					if (ValidInstanceRecord(Type))
					{
						Output << std::format(" ( {} )", GetInstanceRecordValue(Type));
					}
					else
					{
						auto Match = Find((uint16_t)Type - 2);
						if (Match.IsValid())
							Output << std::format(" ( ' {})", Match.Name);
					}
				}

				Output << "\n";

				uint8_t LastPercent = 0;
				while (SubAddress != 0)
				{
					uint8_t Percent = ReadByte(SubAddress);
					uint32_t IAddr = Read3Byte(SubAddress + 1);

					Output << std::format("     {:>3} % {}\n", Percent - LastPercent, GetValueString(IAddr));

					LastPercent = Percent;
					SubAddress += 4;

					if (Percent >= 100)
						break;
				}

				Address += 6;
			}

		}
		break;


		case WordSubType::Expert:
		{
			uint8_t RuleLimit = ReadByte(Entry.DataAddress);
			uint8_t ConditionLimit = ReadByte(Entry.DataAddress + 1);
			uint8_t RuleCount = ReadByte(Entry.DataAddress + 2);

			Output << std::format("{} {} {} {}\n", RuleLimit, ConditionLimit, CodePointer.Name, Entry.Name);

			uint16_t RuleArray = Entry.DataAddress + 3;
			uint16_t ConditionArray = RuleArray + (RuleLimit * 2);

			for (int x = 0; x < RuleCount; x++)
			{
				Output << "   RULE: ";
				uint16_t Address = ReadWord(RuleArray + (x * 2));
				uint8_t Count = ReadByte(Address);
				Address++;

				auto Execute = Find(ReadWord(Address) - 2);
				Address += 2;

				for (int pos = 0; pos < Count; pos++)
				{
					uint8_t Condition = ReadByte(Address) & 0x7F;
					uint8_t TorF = ReadByte(Address) & 0x80;
					Address++;

					auto Match = Find(ReadWord(ConditionArray + (Condition * 2)) - 2);

					Output << std::format("{} {} ", Match.Name, TorF ? "TRUE" : "FALSE");
				}


				Output << std::format(" --> {}\n", Execute.Name);

			}
		}
		break;
		
		case WordSubType::BLT:
		{
			int Address = Entry.DataAddress;
			if (Video)
				Address -= 0xE000;

			Output << std::format("{} {} {} ALLOT ( BLT: {}x{}, @{:04X} =\n", CodePointer.Name, Entry.Name, Entry.DataLength, Entry.Param[0], Entry.Param[1],Address);

			//DrawBitmapAsText(Output, Entry.DataAddress, Entry.Param[0], Entry.Param[1], false);
			DumpBLT(OutputPath / "img", Entry.Name, Entry, Entry.Param[0], Entry.Param[1]);

			DumpHex(Output, GetPtr(0), Entry.DataAddress, Entry.DataLength, Address);
			Output << " )\n";
			

		}		
		break;

		case WordSubType::OverlayHeader:
		{
			OverlayHeader* Header = reinterpret_cast<OverlayHeader*>(GetPtr(Entry.DataAddress));
			uint16_t OverlayEnd = Header->LoadAddress + (Header->Size << 4);
			uint16_t DataEnd = Header->DataPtr;
			Output << std::format("( Overlay Header:\n");
			Output << std::format("  Disk Start: {:04X} \tCount: {:04X} \tData Size: {:04X} (Paragraphs)\n", Header->Start, Header->Size, Header->DataEnd);
			Output << std::format("    On Disk Location {:04X}-{:04X}\n", Header->Start << 4, (Header->Start + Header->Size) << 4);
			Output << std::format("  Overlay Address: @{:04X} \tOverlay DP: @{:04X}\n", Header->LoadAddress, Header->DataPtr);
			Output << std::format("    Load Location: @{:04X}-@{:04X} \tLoad Length: {}\n", Header->LoadAddress, OverlayEnd, OverlayEnd - Header->LoadAddress);
			Output << std::format("    Data: @{:04X}-@{:04X} \tData Length: {}\n    Junk: @{:04X}-@{:04X} \tJunk Length: {}\n", Header->LoadAddress, DataEnd, DataEnd - Header->LoadAddress, DataEnd, OverlayEnd, OverlayEnd - DataEnd);
			Output << std::format("  VOCABULARY Word: {:04X} {}\n", Header->Vocabulary, Find(Header->Vocabulary - 4).Name);
			for (int x = 0; x < 4; x++)
				Output << std::format("   Dictionary Link {}: {:04X}\n", x + 1, Header->Link[x]);

			Output << "\n";
			DumpHex(Output, GetPtr(0), Entry.DataAddress, Entry.DataLength, Entry.DataAddress);
			Output << ")\n";
		}
		break;

		case WordSubType::PointerTable:
		{
			int Address = Entry.DataAddress;
			Output << "( Video Overlay Offset Table:\n";

			for (int x = 0; x < 30; x += 2)
			{
				uint16_t Pointer = ReadWord(Address + x);
				auto Match = Find(Address + Pointer - 2);
				Output << std::format("    @{:04X}", Pointer);
				if (Pointer != 0 && Pointer != 0x2020)
					Output << std::format(" = {} ", Match.Name);
				Output << "\n";
			}

			if (Video)
				Address -= 0xE000;

			Output << "\n";
			DumpHex(Output, GetPtr(0), Entry.DataAddress, Entry.DataLength, Address);
			Output << ")\n";
		}
		break;

		case WordSubType::OverlayJunk:
		{
			int Address = Entry.DataAddress;
			if (Video)
				Address -= 0xE000;

			Output << std::format("{} {} {} ALLOT ( @{:04X} =\n", CodePointer.Name, Entry.Name, Entry.DataLength, Address);
			DumpHex(Output, GetPtr(0), Entry.DataAddress, Entry.DataLength, Address, 0, true);
			Output << " )\n";
		}
		break;

		default:
		{
			int Address = Entry.DataAddress;
			if (Video)
				Address -= 0xE000;

			Output << std::format("{} {} {} ALLOT ( @{:04X} =\n", CodePointer.Name, Entry.Name, Entry.DataLength, Address);
			DumpHex(Output, GetPtr(0), Entry.DataAddress, Entry.DataLength, Address);
			Output << " )\n";
		}
		break;
	}
}



void DumpWordlist(const std::filesystem::path& OutputPath, std::string FileName, FileInformation& File)
{
	std::filesystem::path OutputFile = OutputPath / (FileName + ".txt");

	LoadOverlay(File.OverlayId);

	std::ofstream Output;
	Output.open(OutputFile);

	if (!Output.is_open())
		abort();

	for (auto& Entry : Files[CurrentOverlay].Words)
	{
		int DispBaseAddress = Entry.BaseAddress;
		int DispEndAddress = Entry.DataAddress + Entry.DataLength;
		int DispXt = Entry.ExecutionToken + 2;

		std::string Stage = GetStageString(Entry.Stage);
		std::string Subtype = GetSubtypeString(Entry.SubType);
		bool Video = false;

		if (File.Type == FileType::Video)
		{
			Video = true;

			DispBaseAddress -= 0xE000;
			DispEndAddress -= 0xE000;
			DispXt -= 0xE000;
		}

		if (Entry.Type == WordType::Word)
		{
			Output << std::format("( @{:04X}-{:04X} XT:{:04X} CFA:{:04X} DLen: {} STAGE: {})\n", DispBaseAddress, DispEndAddress, DispXt, Entry.CodePointer, Entry.DataLength, Stage);
			if (HasFlag(Entry.Flags, WordFlags::User))
			{
				auto CodeField = FindCodePointer(Entry.CodePointer);
				uint16_t Index = ReadWord(Entry.DataAddress);
				auto XTWord = Find(ReadWord(UserBase + Index) - 2);

				Output << std::format("' {} {} {} ( @{:04X} )\n", XTWord.Name, CodeField.Name, Entry.Name, Index);
			}
			else if (Entry.SubType == WordSubType::Alias)
			{
				auto CodeField = Find(Entry.CodePointer - 2);
				Output << std::format("ALIAS {} {}\n", Entry.Name, CodeField.Name);
			}
			else
			{
				RehydrateWord(Output, Entry);
			}
			Output << "\n";

		}
		else if (Entry.Type == WordType::Code)
		{
			Output << std::format("( @{:04X}-{:04X} XT:{:04X} CFA:{:04X} DLen: {} STAGE: {})\n", DispBaseAddress, DispEndAddress, DispXt, Entry.CodePointer, Entry.DataLength, Stage);
			RehydrateCode(Output, Entry, File.Type == FileType::Video);
			Output << "\n";
		}
		else if (Entry.Type == WordType::RawCode)
		{
			Output << std::format("( RAW @{:04X}-{:04X} DLen: {} STAGE: {})\n", DispBaseAddress, DispEndAddress, Entry.DataLength, Stage);
			RehydrateCode(Output, Entry, File.Type == FileType::Video);
			Output << "\n";

		}
		else if (Entry.Type == WordType::Data)
		{
			if (Entry.DataLength > 0xF000)
				std::cout << std::format(" INFO: To long Data {}\n", Entry.Name);

			Output << std::format("( {} @{:04X}-{:04X} XT:{:04X} CFA:{:04X} DLen: {} STAGE: {})\n", Subtype, DispBaseAddress, DispEndAddress, DispXt, Entry.CodePointer, Entry.DataLength, Stage);
			DumpData(OutputPath, Output, Entry, File.Type == FileType::Video);

			Output << "\n";
		}
		else if (Entry.Type == WordType::Raw)
		{
			if (Entry.DataLength > 0xF000)
				std::cout << std::format(" INFO: To long Data {}\n", Entry.Name);

			Output << std::format("( RAW {} @{:04X}-{:04X} DLen: {} STAGE: {})\n", Subtype, DispBaseAddress, DispEndAddress, Entry.DataLength, Stage);
			DumpData(OutputPath, Output, Entry, File.Type == FileType::Video);

			Output << "\n";
		}
		else if (Entry.Type == WordType::Overlay)
		{
			Output << std::format("( @{:04X}-{:04X} XT:{:04X} CFA:{:04X} DLen: {} STAGE: {})\n", DispBaseAddress, DispEndAddress, DispXt, Entry.CodePointer, Entry.DataLength, Stage);
			auto File = Files[ReadWord(Entry.DataAddress)];
			Output << std::format("{} OVERLAY {}\n", File.DirectoryId, Entry.Name);

			Output << "\n";
		}
		else if (Entry.Type != WordType::Bad)
		{
			Output << std::format("( UNKNOWN {} {} @{:04X}-{:04X} XT:{:04X} CFA:{:04X} DLen: {} STAGE: {})\n", (int)Entry.Type, Entry.Name, DispBaseAddress, DispEndAddress, DispXt, Entry.CodePointer, Entry.DataLength, Stage);

		}
	}

	Output.flush();
	Output.close();

	UnloadOverlay();
}
