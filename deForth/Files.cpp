#include "Files.h"
#include <cstdio>
#include "StarflightData.h"

// Turn off the useless buffer overrun warnings so it dosn't claime that reading 22 bytes from the start of 300k buffer is an overrun
#pragma warning(disable:6386)
#pragma warning(disable:6385)

std::map<uint16_t, FileInformation> Files;
int CurrentOverlay = 0;
int StartOfOverlay = 0;
int EndOfMemory = 0;

int InsertCount = 0;

uint32_t StarBAddress = 0;

uint8_t* DataBuffer = nullptr;
uint8_t* OverlayData = nullptr;

bool LoadFiles(const std::filesystem::path& StarFlt, const std::filesystem::path& StarA, const std::filesystem::path& StarB)
{
	FILE* starflt = nullptr;
	FILE* stara = nullptr;
	FILE* starb = nullptr;

	fopen_s(&starflt, StarFlt.string().c_str(), "rb");
	fopen_s(&stara, StarA.string().c_str(), "rb");
	fopen_s(&starb, StarB.string().c_str(), "rb");

	if (starflt == nullptr || stara == nullptr || starb == nullptr)
		return false;

	std::fseek(starflt, 0, SEEK_END);
	EndOfMemory = std::ftell(starflt) + 0x100;
	std::fseek(starflt, 0, SEEK_SET);

	std::fseek(stara, 0, SEEK_END);
	int StarASize = std::ftell(stara);
	std::fseek(stara, 0, SEEK_SET);

	std::fseek(starb, 0, SEEK_END);
	int StarBSize = std::ftell(starb);
	std::fseek(starb, 0, SEEK_SET);

	DataBuffer = new uint8_t[0x10000];
	OverlayData = new uint8_t[StarASize + StarBSize];

	memset(DataBuffer, 0, 0x10000);
	memset(OverlayData, 0, size_t(StarASize + StarBSize));

	std::fread(&DataBuffer[0x100], 1, size_t(0x10000 - 0x100), starflt);
	std::fread(OverlayData, 1, size_t(StarASize), stara);
	std::fread(OverlayData + StarASize, 1, StarBSize, starb);
	StarBAddress = StarASize;

	std::fclose(starflt);
	std::fclose(stara);
	std::fclose(starb);

	// Add the main file 
	FileInformation MainFile;
	MainFile.Name = "STARFLT";
	MainFile.Base = 0x100;
	MainFile.End = EndOfMemory;
	MainFile.Length = EndOfMemory - 0x100;
	MainFile.Type = FileType::Main;
	MainFile.OverlayId = 0x0000;
	MainFile.InstanceLen = 0;

	Files[0x0000] = MainFile;

	// Load the directory
	std::vector<DirectoryInfoEntry> DirectoryInfo;
	DirectoryInfoEntry* DirectoryRoot = reinterpret_cast<DirectoryInfoEntry*>(OverlayData);

	uint32_t Address = 0;

	for (int x = 0; x < DirectoryRoot->RecodCount; x++)
	{
		DirectoryInfoEntry NewEntry = *reinterpret_cast<DirectoryInfoEntry*>(GetPtrDirect(Address));
		DirectoryInfo.push_back(NewEntry);

		Address += DirectoryRoot->RecordLen;
		Address = BlockCorrection(Address, DirectoryRoot->RecordLen);
	}

	// Build the file informatino from the directory
	for (int x = 0; x < DirectoryInfo.size(); x++)
	{
		FileInformation File;
		File.Name = std::string_view(DirectoryInfo[x].Name, 12);
		File.Disk = DirectoryInfo[x].Disk;
		File.Type = FileType::Other;
		File.OverlayId = (uint16_t)x + 1;
		File.Address = DirectoryInfo[x].Start * 0x10;
		File.Base = 0;
		File.End = 0;

		if (DirectoryInfo[x].End != 0)
			File.Length = ((DirectoryInfo[x].End + 1) * 0x10) - File.Address;
		else if (x == 0)
			File.Length = 0x1000;
		else
			File.Length = 0;

		File.DirectoryId = x;
		File.InstanceLen = DirectoryInfo[x].InstanceLen;
		File.RecordCount = DirectoryInfo[x].RecodCount;
		File.RecordLength = DirectoryInfo[x].RecordLen;

		File.Name = TrimString(File.Name);

		if (File.Name.empty() || File.Name == "<unused>")
			continue;

		//if (Verbose >= VerboseLevel::All)
		//{
		//	std::cout << std::format("{:3} @{:05X} {:12} Disk: {:02X} Start: {:04X} End: {:4X} Count: {:4} Len: {:3} Instance Len: {:2} File Length: {:06X}\n", File.DirectoryId, File.Address, File.Name.c_str(),
		//		DirectoryInfo[x].Disk, DirectoryInfo[x].Start, DirectoryInfo[x].End, DirectoryInfo[x].RecodCount,
		//		DirectoryInfo[x].RecordLen, File.InstanceLen, File.Length
		//	);
		//}

		int PageCount = (DirectoryInfo[x].End - DirectoryInfo[x].Start) + 1;

		OverlayHeader* Header = reinterpret_cast<OverlayHeader*>(GetPtrDirect(File.Address));

		if (Header->Start == DirectoryInfo[x].Start)
		{
			File.Type = FileType::Overlay;
			File.OverlayId = Header->Start;
			File.Base = Header->LoadAddress;
			File.End = Header->LoadAddress + (Header->Size << 4);
			File.Length = Header->Size << 4;

			//if (Verbose >= VerboseLevel::Debug)
			//	std::cout << std::format("  OVERLAY:  {: <12} ID: {:04X} LoadAddress: {:04X}-{:04X} Length {:04X}\n", File.Name.c_str(), File.OverlayId, File.Base, File.End, File.Length);
		}
		else if (File.Name == "EGA" || File.Name == "CGA")
		{
			File.Type = FileType::Video;
			File.OverlayId = DirectoryInfo[x].Start;
			File.Base = 0xE000;
			File.End = File.Base + File.Length;

			//if (Verbose >= VerboseLevel::Debug)
			//	std::cout << std::format("    VIDEO:  {: <12} ID: {:04X} LoadAddress: {:04X}-{:04X} Length {:04X}\n", File.Name.c_str(), File.OverlayId, File.Base, File.End, File.Length);
		}

		Files[File.OverlayId] = File;
	}


	return false;
}


FileInformation FindFileID(int ID)
{
	for (auto &File : Files)
	{
		if (File.second.DirectoryId == ID)
			return File.second;
	}

	return Files[0];
}

FileInformation FindFile(std::string Name)
{
	for (auto &File : Files)
	{
		if (File.second.Name == Name)
			return File.second;
	}

	return Files[0];
}

int LookupOverlay(std::string OverlayName)
{
	for (auto &Entry : Files)
	{
		if (Entry.second.Name == OverlayName)
		{
			return Entry.first;
		}
	}

	return -1;
}


void UnloadOverlay()
{
	if (CurrentOverlay != 0x0000)
	{
		memset(&DataBuffer[StartOfOverlay], 0x20, 0xFFFF - StartOfOverlay);
		StartOfOverlay = 0xFFFF;
		CurrentOverlay = 0x0000;
	}
}

void LoadOverlay(uint16_t OverlayId)
{
	if (OverlayId != CurrentOverlay)
	{
		UnloadOverlay();

		if (OverlayId != 0x0000)
		{
			FileInformation Overlay = Files[OverlayId];

			StartOfOverlay = Overlay.Base;

			memcpy(&DataBuffer[Overlay.Base], &OverlayData[Overlay.Address], Overlay.End - Overlay.Base);
		}

		CurrentOverlay = OverlayId;
	}
}


bool OverlayAddress(uint16_t Address)
{
	return Address >= EndOfMemory;
}

void Insert(ForthWord& Entry)
{	
	if (!OverlayAddress(Entry.ExecutionToken))
		Files[0x0000].Words.Insert(Entry);
	else
		Files[CurrentOverlay].Words.Insert(Entry);

	InsertCount++;
}

void Update(ForthWord& Entry)
{
	if (!OverlayAddress(Entry.ExecutionToken))
		Files[0x0000].Words.Update(Entry);
	else
		Files[CurrentOverlay].Words.Update(Entry);
}

ForthWord Find(uint16_t CodeFieldPtr)
{
	if (!OverlayAddress(CodeFieldPtr))
		return Files[0x0000].Words.FindWord(CodeFieldPtr);
	else
		return Files[CurrentOverlay].Words.FindWord(CodeFieldPtr);
}

ForthWord FindCodePointer(uint16_t CodeFieldPtr)
{
	if (!OverlayAddress(CodeFieldPtr))
		return Files[0x0000].CodeFields.FindWord(CodeFieldPtr);
	else
		return Files[CurrentOverlay].CodeFields.FindWord(CodeFieldPtr);
}

std::string GetOverlayName(uint16_t Address)
{
	if (!OverlayAddress(Address))
		return Files[0x0000].Name;
	else
		return Files[CurrentOverlay].Name;
}


std::string ReadStringDirect(uint32_t Address, int Len)
{
	return std::string((char*)& OverlayData[Address], Len);
}

uint32_t Read3ByteDirect(uint32_t Address)
{
	return
		(uint32_t)OverlayData[Address + 2] << 16 |
		(uint32_t)OverlayData[Address + 1] << 8 |
		(uint32_t)OverlayData[Address];
}

uint32_t ReadDoubleWordDirect(uint32_t Address)
{
	// Double Words are values stored as two Words, with the high value first and the low value following
	// Both stored in little endian order.	
	return (uint32_t)ReadWordDirect(Address) << 16 | (uint32_t)ReadWordDirect(Address + 2);
}

uint16_t ReadWordDirect(uint32_t Address)
{
	return OverlayData[Address] | OverlayData[Address + 1] << 8;
}

uint8_t ReadByteDirect(uint32_t Address)
{
	return OverlayData[Address];
}

uint8_t* GetPtrDirect(uint32_t Address)
{
	return &OverlayData[Address];
}


std::string ReadString(uint16_t Address, int Len)
{
	return std::string((char*)&DataBuffer[Address], Len);
}

uint32_t Read3Byte(uint32_t Address)
{
	return 
		(uint32_t)DataBuffer[Address + 2] << 16 |
		(uint32_t)DataBuffer[Address + 1] << 8 | 
		(uint32_t)DataBuffer[Address];
}

uint32_t ReadDoubleWord(uint32_t Address)
{
	// Double Words are values stored as two Words, with the high value first and the low value following
	// Both stored in little endian order.	
	return (uint32_t)ReadWord(Address) << 16 | (uint32_t)ReadWord(Address + 2);
}

uint16_t ReadWord(uint16_t Address)
{
	return DataBuffer[Address] | DataBuffer[Address + 1] << 8;
}

uint8_t ReadByte(uint16_t Address)
{
	return DataBuffer[Address];
}

uint8_t* GetPtr(uint32_t Address)
{
	return &DataBuffer[Address];
}

uint32_t BlockCorrection(uint32_t Address, int RecordLength)
{
	// Records can't cross block boundries, so adjust its position. 
	int Remainder = 0x400 - (Address % 0x400);

	if (Remainder < RecordLength)
		return Address + Remainder;

	return Address;
}


std::vector<std::string> ReadAllStringRecords(FileInformation File, char Trim, bool HasLength)
{
	std::vector<std::string> Ret;

	uint32_t Address = File.Address;

	for (int x = 0; x < File.RecordCount; x++)
	{
		std::string Entry;
		if (HasLength)
		{
			uint8_t len = ReadByteDirect(Address);
			Entry = ReadStringDirect(Address + 1, len);
		}
		else
		{
			Entry = ReadStringDirect(Address, File.RecordLength);
		}

		Entry = TrimString(Entry, Trim);

		Ret.push_back(Entry);

		Address += File.RecordLength;
		Address = BlockCorrection(Address, File.RecordLength);
	}

	return Ret;
}

