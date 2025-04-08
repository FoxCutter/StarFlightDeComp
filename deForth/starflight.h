#pragma once
#include <stdint.h>
#include "String.h"
#include <vector>
#include <filesystem>

#include "Dictionary.h"
#include "Enum.h"



enum class NamingAction
{
	// Does Nothing 
	None = 0x00000000,

	// Define a name entry that can not otherwise be defined
	//	Using Name and Type
	Define = 0x00000001,

	// Rename an existing word 
	Rename = 0x00000010,

	// Resize an existing word
	Resize = 0x00000100,

	// Change the type of an exsting word
	Retype = 0x00001000,

};

enum class VerboseLevel
{
	None,
	Basic,
	All,
	Debug,
};


struct NamingInformation
{
	NamingAction Action = NamingAction::None;
	std::string OverlayName;

	uint16_t CodeFieldPtr = 0;
	std::string MatchName;

	std::string Name;
	WordType Type = WordType::Unknown;
	WordSubType SubType = WordSubType::None;
	uint16_t TypeParam[2] = { 0, 0 };
	uint16_t Size = 0;
};




bool LoadStarFlight1(const std::filesystem::path& BasePath, VerboseLevel Verbose = VerboseLevel::None);
bool ProcessStarFlight(std::vector<NamingInformation>& NamingInformation, VerboseLevel Verbose = VerboseLevel::None);
bool DumpStarFlight(const std::filesystem::path& OutputPath, VerboseLevel Verbose = VerboseLevel::None);

