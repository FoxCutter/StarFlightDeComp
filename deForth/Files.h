#pragma once
#include "String.h"
#include <map>
#include <filesystem>

#include "Dictionary.h"

enum class FileType
{
	Main,		// The main StarFlt file
	Overlay,	// An Overlay
	Video,		// Video Driver
	Other,		// Other kind of file
	External,	// External files (SF2)
};


struct FileInformation
{
	std::string Name;
	FileType Type = FileType::Other;
	uint16_t DirectoryId = 0;
	uint16_t OverlayId = 0;
	uint32_t Address = 0;
	uint16_t Base = 0;
	uint16_t End = 0;
	uint32_t Length = 0;
	uint16_t RecordCount = 0;
	uint16_t RecordLength = 0;
	uint16_t InstanceLen = 0;
	uint8_t  Disk = 0;

	Dictionary Words;
	Dictionary CodeFields;
};

extern std::map<uint16_t, FileInformation> Files;


extern int CurrentOverlay;
extern int InsertCount;
extern uint32_t StarBAddress;

bool LoadFiles(const std::filesystem::path &StarFlt, const std::filesystem::path &StarA, const std::filesystem::path &StarB);


FileInformation FindFileID(int ID);
FileInformation FindFile(std::string Name);

int LookupOverlay(std::string OverlayName);

void LoadOverlay(uint16_t OverlayId);
void UnloadOverlay();

bool OverlayAddress(uint16_t Address);

void Insert(ForthWord& Entry);
void Update(ForthWord& Entry);

ForthWord Find(uint16_t CodeFieldPtr);
ForthWord FindCodePointer(uint16_t CodeFieldPtr);

std::string ReadString(uint16_t Address, int Len);
uint8_t ReadByte(uint16_t Address);
uint16_t ReadWord(uint16_t Address);
uint32_t ReadDoubleWord(uint32_t Address);	
uint32_t Read3Byte(uint32_t Address);

uint8_t* GetPtr(uint32_t Address);

std::string ReadStringDirect(uint32_t Address, int Len);
uint8_t ReadByteDirect(uint32_t Address);
uint16_t ReadWordDirect(uint32_t Address);
uint32_t ReadDoubleWordDirect(uint32_t Address);
uint32_t Read3ByteDirect(uint32_t Address);

uint8_t *GetPtrDirect(uint32_t Address);

uint32_t BlockCorrection(uint32_t Address, int RecordLength);

std::string GetOverlayName(uint16_t Address);



template <typename T>
std::vector<T> ReadAllRecords(FileInformation File)
{
	std::vector<T> Ret;

	uint32_t Address = File.Address;

	for (int x = 0; x < File.RecordCount; x++)
	{
		T Entry = *reinterpret_cast<T*>(GetPtrDirect(Address));
		Ret.push_back(Entry);

		Address += File.RecordLength;
		Address = BlockCorrection(Address, File.RecordLength);
	}

	return Ret;
}

std::vector<std::string> ReadAllStringRecords(FileInformation File, char Trim = ' ', bool HasLength = false);
