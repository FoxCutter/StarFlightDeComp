#include <format>
#include <iostream>
#include <fstream>

#include "Instance.h"

#include "StarflightData.h"
#include "starflight.h"
#include "files.h"
#include "images.h"


// Turn off the useless buffer overrun warnings so it dosn't claime that reading 22 bytes from the start of 300k buffer is an overrun
#pragma warning(disable:6386)
#pragma warning(disable:6385)

struct InstanceRecord
{
	std::string ClassName;
	uint32_t IAddress = 0;

	uint32_t DataAddress = 0;
	uint16_t DataLength = 0;

	uint32_t Owner = 0;
	uint32_t Sibling = 0;
	uint32_t Prev = 0;
	uint32_t Offset = 0;

	uint8_t Class = 0;
	uint8_t SubClass = 0;

	// String value or Box name
	std::string Value;
};

std::map<uint32_t, InstanceRecord> AllRecords;
static uint32_t StarBAddress;

std::vector<std::string> BoxNames;
std::vector<std::string> TransactionNames;
std::vector<ArtifactData> Artifacts;
std::vector<ElementData> Elements;

uint32_t FreeSpaceStart;
uint32_t MaxSpace;
uint32_t FillerIaddr;

void InitilizeInstanceData(FileInformation &InstanceFile)
{
	StarBAddress = InstanceFile.Address - 0x1000;
	FreeSpaceStart = Read3ByteDirect(StarBAddress + 0x1000);
	MaxSpace = Read3ByteDirect(StarBAddress + 0x1003);
	FillerIaddr = MaxSpace + 0x1000;

	Artifacts = ReadAllRecords<ArtifactData>(FindFile("ARTIFACT"));
	BoxNames = ReadAllStringRecords(FindFile("BOX"));
	TransactionNames = ReadAllStringRecords(FindFile("BANK-TRANS"), ' ', true);
	Elements = ReadAllRecords<ElementData>(FindFile("ELEMENT"));

	// Sometimes "EVALUATIONS" has the V and A replaced with NULL, so correct that here
	if (BoxNames[45][0] == 'E' && BoxNames[45][1] == 0x0)
		BoxNames[45] = "EVALUATIONS";

}



// 80 89 0C F1 47 98 18
// 10000000 10001001 00001100 11110001 01000111 10011000 00011000

// 1000 1001 1000 110001 110001 110001 110001 1000 1001 1000 0

//  LRRR S
//  LRRL 0
//	LRRR S
//  RRLLLR .
//  RRLLLR .
//  RRLLLR .
//  RRLLLR .
//  RRLLLR .
//  LRRR S
//  LRRL 0
//	LRRR S


// From [STARFLT:BCB1] / _huff-table
uint8_t HuffmanTable[100] = {						//		0, 1
	0x9C , 0x81 ,			// 0	1 - 1C,  1 - 1			38, 2
	0x96 , 0x81 ,			// 2	1 - 16,  1 - 1			2E, 4  
	0x81 , 0x20 ,			// 4	1 - 1,   0 ' '			6, ' '
	0x83 , 0x81 ,			// 6	1 - 3,   1 - 1			C, 8   
	0x81 , 0x48 ,			// 8	1 - 1,   0 - 'H'		A, 'H'
	0x59 , 0x43 ,			// A	0 - 'Y', 0 - 'C'		'Y', 'C'
	0x82 , 0x81 ,			// C	1 - 2,   1 - 1			10, E  
	0x4D , 0x57,			// E   0 - 'M'  0 - 'W'			'M', 'W'
	0x81 , 0x2E ,			// 10	1 - 1,	 0 - '.'		12, '.'   
	0x81 , 0x42 ,			// 12   1 - 1,   0 - 'B'		14, 'B'
	0x84 , 0x81 ,			// 14   1 - 4,   1 - 1			1C, 16
	0x31 , 0x81 ,			// 16	0 - '1', 1 - 1			'1', 18
	0x5A , 0x81 ,			// 18   0 - 'Z', 1 - 1			'Z', 1A
	0x34 , 0x33 ,			// 1A   0 - '4', 0 - '3'		'4', '3'
	0x86 , 0x81 ,			// 1C	1 - 6,	 1 - 1			28, 1E
	0x51 , 0x81,			// 1E	0 - 'Q', 1 - 1			'Q', 20
	0x81 , 0x32 ,			// 20	1 - 1,   0 - '2'		22, '2'
	0x81 , 0x25 ,			// 22   1 - 1,	 0 - '%'		24, '%'
	0x2B , 0x81 ,			// 24	0 - '+', 1 - 1			'+', 26
	0x23 , 0x28 ,			// 26	0 - '#', 0 - '('		'#', '('
	0x81 , 0x3F ,			// 28	1 - 1,	 0 - '?'		2A, '?'
	0x39 , 0x81 ,			// 2A   0 - '9', 1 - 1			'9', 2C
	0x37 , 0x2A ,			// 2C   0 - '7', 0 - '*'		'7', '*'
	0x84 , 0x81,			// 2E   1 - 4,	 1 - 1			36, 30    
	0x54 , 0x81 ,			// 30	0 - 'T', 1 - 1			'T', 32
	0x81 , 0x4C ,			// 32	1 - 1,	 0 - 'L'		34, 'L' 
	0x50 , 0x46 ,			// 34   0 - 'P', 0 - 'F'		'P', 'F'
	0x53 , 0x4F ,			// 36	0 - 'S', 0 - 'O'		'S', 'O'
	0x8D , 0x81 ,			// 38	1 - D,	 1 - 1,			52, 3A
	0x82 , 0x81 ,			// 3A	1 - 2,	 1 - 1			3E, 3C
	0x4E , 0x41 ,			// 3C	0 - 'N', 0 - 'A'		'N', 'A'
	0x49 , 0x81,			// 3E   0 - 'I', 1 - 1			'I', 40
	0x81 , 0x55 ,			// 40   1 - 1,	 0 - 'U'		42,	'U'
	0x81 , 0x47 ,			// 42	1 - 1,	 0 - 'G'		44,	'G'
	0x2C , 0x81 ,			// 44   0 - ',', 1 - 1			',', 46
	0x81 , 0x58 ,			// 46   1 - 1,	 0 - 'X'		48, 'X'
	0x27 , 0x81 ,			// 48	0 - ''', 1 - 1			''', 4A
	0x81 , 0x21 ,			// 4A   1 - 1,	 0 - '!'		4C, '!'
	0x35 , 0x81 ,			// 4C   0 - '5', 1 - 1			'5', 4E
	0x26 , 0x81,			// 4E   0 - '&', 1 - 1			'&', 50
	0x29 , 0x2F ,			// 50	0 - ')', 0 - '/'		')', '/'
	0x81 , 0x45 ,			// 52	1 - 1,   0 - 'E'		54, 'E'
	0x81 , 0x52 ,			// 54	1 - 1,	 0 - 'R'		56, 'R'
	0x81 , 0x44 ,			// 56	1 - 1,   0 - 'D'		58, 'D'
	0x81 , 0x56 ,			// 58   1 - 1,   0 - 'V'		5A, 'V'
	0x4B , 0x81 ,			// 5A   0 - 'K', 1 - 1			'K', 5C
	0x81 , 0x30 ,			// 5C   1 - 1,	 0 - '0'		5E, 'O'
	0x81 , 0x2D,			// 5E   1 - 1,	 0 - '-'		60, '-'
	0x81 , 0x4A ,			// 60	1 - 1,   0 - 'J'		62, 'J'
	0x36 , 0x38 };			// 62	0 - '6', 0 - '8'		'6', '8'


std::string DecompressString(uint32_t Address, uint16_t Len)
{
	// Get the length of the result string
	int OutputLen = ReadByteDirect(Address);
	Address++;

	int TableOffset = 0;
	int InputPos = 0;
	int OutputPos = 0;
	uint8_t HuffBitmask = 0x80;

	std::string Result;

	while (OutputPos < OutputLen)
	{
		do
		{
			uint8_t Value = ReadByteDirect(Address + InputPos);
			uint8_t Bit = Value & HuffBitmask;
			uint8_t TableValue;
			if (Bit == 0)
				TableValue = HuffmanTable[TableOffset];
			else
				TableValue = HuffmanTable[TableOffset + 1];

			if (TableValue & 0x80)
			{
				// If the high bit is set, it's a branch
				TableOffset += (TableValue & 0x7F) * 2;
			}
			else
			{
				// Otherwise it's a value
				Result += TableValue;
				OutputPos++;

				TableOffset = 0;
			}

			if (HuffBitmask == 0x80)
				InputPos++;

			HuffBitmask /= 2;
			if (HuffBitmask == 0)
				HuffBitmask = 0x80;

		} while (HuffBitmask != 0x80 && OutputPos < OutputLen);
	}

	return Result;
}


bool ValidRecord(InstanceRecord& Record)
{
	// Need to have a proper class
	if (Record.Class == 0)
		return false;

	// Can't have a pointer off past the end of the free space
	if (Record.Sibling > FreeSpaceStart || Record.Offset > FreeSpaceStart || Record.Prev > FreeSpaceStart)
		return false;


	// Anything that has a status length should be valid by this point
	if (Record.DataLength != 0)
		return true;

	// Handle any records without status length
	// 2 IROOT
	// 3 ISYSTEM        
	// 4 INACTIVE        
	// 5 FRAGMENT        
	if (Record.Class >= 2 && Record.Class <= 5)
		return true;

	// 7 IAPPLICATION  
	if (Record.Class == 7)
		return true;

	//	12  STARPORT    
	if (Record.Class == 12)
		return true;

	//	15  PERSONNEL   
	if (Record.Class == 15)
		return true;

	//	18  SHIP-CONFIG 
	if (Record.Class == 18)
		return true;

	//	19  TRADE-DEPOT 
	if (Record.Class == 19)
		return true;

	// 32 PLANET
	if (Record.Class == 32)
		return true;

	// 34 OPERATIONS
	if (Record.Class == 34)
		return true;

	// 48 STRING
	if (Record.Class == 48)
		return true;
	
	// 51 SUBJECT
	if (Record.Class == 51)
		return true;

	// 57 ANALYZE-TEXT
	if (Record.Class == 57)
		return true;

	return false;
}

InstanceRecord ReadInstanceRecord(uint32_t IAddress)
{
	InstanceRecord Result;
	const Instance *Data = reinterpret_cast<Instance*>(GetPtrDirect(StarBAddress + IAddress));
	
	Result.IAddress = IAddress;

	Result.Sibling = Data->Sibling.Full();
	Result.Prev = Data->Prev.Full();
	Result.Offset = Data->Offset.Full();
	Result.Class = Data->Class;
	Result.SubClass = Data->SubClass;

	Result.DataAddress = IAddress + 11;

	auto File = FindFileID(Result.Class);

	Result.ClassName = File.Name;
	Result.DataLength = File.InstanceLen;

	if (Result.ClassName == "STRING")
	{
		Result.DataLength = ReadByteDirect(StarBAddress + Result.DataAddress) + 1;
		uint8_t Mark = ReadByteDirect(StarBAddress + Result.DataAddress + 2);

		// Check for a compressed string
		if (Result.DataLength > 3 && Mark == 0x00 || Mark == 0x80)
		{
			Result.Value = DecompressString(StarBAddress + Result.DataAddress + 1, Result.DataLength - 1);
		}
		else
		{
			// normal string
			Result.Value = ReadStringDirect(StarBAddress + Result.DataAddress + 1, Result.DataLength - 1);
		}

	}
	else if (Result.ClassName == "CAPT-LOG")
	{
		Result.Value = ReadStringDirect(StarBAddress + Result.DataAddress, 36);
	}
	else if (Result.ClassName == "BOX")
	{
		if (Result.SubClass < BoxNames.size())
			Result.Value = BoxNames[Result.SubClass];
	}

	return Result;
}

uint32_t FindOwner(uint32_t OffsetAddress)
{
	// Brute force this one
	for (auto& record : AllRecords)
	{
		if (record.second.Offset == OffsetAddress)
		{
			return record.first;
		}
	}

	return 0;
}

void ProcessInstanceData()
{
	uint32_t FirstFiller = 0;
	uint32_t LastFiller = 0;
	int FillerCount = 0;

	uint32_t IAddress = 0x1006;
	uint32_t LastIAddress = 0x1006;
	
	bool Skip = false;
	// Brute force every record we can find
	while (IAddress < FreeSpaceStart)
	{
		auto Record = ReadInstanceRecord(IAddress);
		if (!ValidRecord(Record))
		{
			IAddress++;
			continue;
		}

		//if (LastIAddress != IAddress/* && IAddress - LastIAddress > 10*/)
		//{
		//	InstanceRecord Filler;
		//	Filler.IAddress = LastIAddress;
		//	Filler.DataAddress = LastIAddress;
		//	Filler.Class = 0xFF;
		//	Filler.SubClass = 0xFF;
		//	Filler.ClassName = "GAP";
		//	Filler.DataLength = IAddress - LastIAddress;
		//	Filler.Offset = 0;
		//	Filler.Sibling = 0;
		//	
		//	if (FirstFiller == 0)
		//	{
		//		FirstFiller = Filler.IAddress;
		//	}
		//	else
		//	{
		//		Filler.Prev = LastFiller;
		//		AllRecords[LastFiller].Sibling = Filler.IAddress;
		//	}
		//	
		//	LastFiller = Filler.IAddress;
		//
		//	//FillerRoot.Children.emplace_back(Filler);
		//	AllRecords[Filler.IAddress] = Filler;
		//	FillerCount++;
		//}

		IAddress += Record.DataLength + 11;
		
		//Skip = false;
	
		LastIAddress = IAddress;
		IAddress = BlockCorrection(IAddress, 11);

		//if(!Skip)
		AllRecords[Record.IAddress] = Record;
	}

	// Complete the loop of all the filler records
	//AllRecords[LastFiller].Sibling = FirstFiller;
	//AllRecords[FirstFiller].Prev = LastFiller;

	// And create a dummy root node to stick them under
	//InstanceRecord FillerRoot;
	//FillerRoot.DataAddress = FillerRoot.IAddress = FillerIaddr;
	//FillerRoot.Class = 0xFF;
	//FillerRoot.SubClass = 0xFF;
	//FillerRoot.Prev = FillerRoot.Sibling = 0x00000000;
	//FillerRoot.Offset = FirstFiller;
	//FillerRoot.ClassName = "GAP-ROOT";
	//FillerRoot.DataLength = 0;

	//AllRecords[FillerRoot.IAddress] = FillerRoot;
	//FillerCount++;

	// Sanity check all the cross refrences 
	for (auto& record : AllRecords)
	{	
		bool BadRecord = false;
		if (record.second.Sibling != 0 && !ValidInstanceRecord(record.second.Prev))
			BadRecord = true;

		if (record.second.Sibling != 0 && !ValidInstanceRecord(record.second.Sibling))
			BadRecord = true;

		if (record.second.Offset != 0 && !ValidInstanceRecord(record.second.Offset))
			BadRecord = true;

		if (record.second.ClassName == "MESSAGE" || record.second.ClassName == "MESSAGE'")
		{
			uint32_t Text = Read3ByteDirect(StarBAddress + record.second.DataAddress - 1);

			if(!ValidInstanceRecord(Text))
				BadRecord = true;
		}

		if (BadRecord)
		{
			std::cout << std::format(" ERROR: Instantace record {1:06X} {0} has bad links.\n", GetInstanceRecordValue(record.first), record.first);
		}
		else
		{
			record.second.Owner = FindOwner(record.first);
		}
	}
	
	std::cout << std::format(" INFO: Found {} instance records and made {} filler records\n", AllRecords.size() - FillerCount, FillerCount);
}

bool ValidInstanceRecord(uint32_t IAddress)
{
	if (AllRecords.find(IAddress) == AllRecords.end())
		return false;
	else
		return true;
}

std::string GetRace(int Type)
{
	switch (Type)
	{
	default:
		return "Unknown Race";
	case 1:
		return "Elowan";
	case 2:
		return "Gazurt";
	case 3:
		return "Mechan 9";
	case 4:
		return "Mysteron";
	case 5:
		return "Nomad";
	case 6:
		return "Spemin";
	case 7:
		return "Thryn";
	case 8:
		return "Velox";
	case 9:
		return "Uhlek";
	case 10:
		return "Velox Probe";
	case 11:
		return "Minstrel";
	case 16:
		return "Communication Officer";
	case 18:
		return "USS Enterprise";
	case 19:
		return "Last Hope";
	case 20:
		return "Debris?";
	case 21:
		return "Police";
	}
}

std::string GetInstanceRecordValue(uint32_t IAddress)
{
	if (AllRecords.find(IAddress) == AllRecords.end())
		return "";

	const auto& Entry = AllRecords[IAddress];

	if (Entry.ClassName == "STRING")
		return "\"" + Entry.Value + "\"";

	else if (Entry.ClassName == "BOX")
		return Entry.ClassName + ":" + Entry.Value;

	else if (Entry.ClassName == "ORIGINATOR")
		return Entry.ClassName + ":" + GetRace(Entry.SubClass);

	return "CLASS:" + Entry.ClassName;
}




struct InstanceTreeRecord
{
	uint32_t IAddr = 0;
	std::string ClassName;
	std::string BoxName;

	std::vector<InstanceTreeRecord> Children;
};

// Builds a simple tree of all the nodes under this one
void BuildRecordTree(InstanceTreeRecord& Parent)
{
	const auto& ParentNode = AllRecords[Parent.IAddr];

	Parent.ClassName = ParentNode.ClassName;
	if (Parent.ClassName == "BOX")
		Parent.BoxName = ParentNode.Value;

	// No offset no children
	if (ParentNode.Offset == 0)
		return;

	// Check to see if the offset is actually a valid record 
	if (AllRecords.find(ParentNode.Offset) == AllRecords.end())
		return;

	InstanceTreeRecord FirstChild;
	FirstChild.IAddr = ParentNode.Offset;
	const auto& FirstChildNode = AllRecords[FirstChild.IAddr];

	BuildRecordTree(FirstChild);
	Parent.Children.emplace_back(FirstChild);

	// If the address and Sibling match, it means there are no other child records
	if (FirstChildNode.IAddress == FirstChildNode.Sibling)
		return;

	uint32_t NextRecord = FirstChildNode.Sibling;
	do
	{
		if (AllRecords.find(NextRecord) == AllRecords.end())
			break;

		InstanceTreeRecord SibRecord;
		SibRecord.IAddr = NextRecord;
		const auto& SibRecordNode = AllRecords[SibRecord.IAddr];

		BuildRecordTree(SibRecord);
		Parent.Children.emplace_back(SibRecord);

		NextRecord = SibRecordNode.Sibling;
	} while (NextRecord != FirstChildNode.IAddress);
}


// ===================================================================

// Based on HYPER-OV SC>C
std::string StarType(int Type)
{
	switch (Type)
	{
		case 77: // M
			return "M (RED)";
		case 75: // K
			return "K (ORANGE)";
		case 71: // G
			return "G (YELLOW)";
		case 70: // F
			return "F (WHITE)";
		case 65: // A
			return "A (GREEN)";
		case 66: // B
			return "B (LT-BLUE)";
		default:
		case 79: // O
			return "O (BLUE)";
	}

	//return std::string(1, (char)Type);
}

// Based on HYPER-OV SC>C
ScreenColor StarColor(int Type)
{
	switch (Type)
	{
		case 77:
			return ScreenColor::Red;
		case 75:
			return ScreenColor::Orange;
		case 71:
			return ScreenColor::Yellow;
		case 70:
			return ScreenColor::White;
		case 65:
			return ScreenColor::Green;
		case 66:
			return ScreenColor::LtBlue;

		default:
		case 79:
			return ScreenColor::Blue;

	}
}

std::string GetSubject(int Type)
{
	switch (Type)
	{
		default:
		case 0:
			return "Unknown subject";
		case 1:
			return "Hailing";
		case 2:
			return "Response";

		//case 3:
		//case 4:

		case 5:
			return "Don't Know";
		case 6:
			return "Awaiting Response";
		case 7:
			return "Themselves";
		case 8:
			return "Other Races";
		case 9:
			return "Clues";
		case 10:
			return "Old Empire";
		case 11:
			return "Ancient Ones";

		//case 12:

		case 13:
			return "General Info";
		case 14:
			return "Questions";
		case 15:
			return "End Communication";
	}
}

std::string GetCrewmemberSpecies(int Type)
{
	switch (Type)
	{
		default:
			return "None";
		case 1:
			return "Human";
		case 2:
			return "Velox";
		case 3:
			return "Thyrnn";
		case 4:
			return "Elowan";
		case 5:
			return "Android";
	}
}

std::string GetContext(int Type)
{
	std::string Ret;

	if ((Type & 0x02) != 0)
	{
		if (Ret.size() != 0)
			Ret += " | ";

		Ret += "Homeplanet";
	}

	if ((Type & 0x01) != 0)
	{
		if (Ret.size() != 0)
			Ret += " | ";

		Ret += "Space";
	}

	if ((Type > 0x03 ) != 0)
	{
		if (Ret.size() != 0)
			Ret += " | ";
		
		Ret += "Huh?";
	}

	return Ret;
}

std::string GetPosture(int Type)
{
	std::string Ret;

	if ((Type & 0x08) != 0)
	{
		if (Ret.size() != 0)
			Ret += " | ";

		Ret += "Hostile";
	}

	if ((Type & 0x04) != 0)
	{
		if (Ret.size() != 0)
			Ret += " | ";

		Ret += "Neutral";
	}

	if ((Type & 0x02) != 0)
	{
		if (Ret.size() != 0)
			Ret += " | ";

		Ret += "Friendly";
	}

	if ((Type & 0x01) != 0)
	{
		if (Ret.size() != 0)
			Ret += " | ";

		Ret += "Obsequious";
	}

	if(Type > 0xF)
	{
		if (Ret.size() != 0)
			Ret += " | ";

		Ret += "Huh?";
	}

	return Ret;
}

std::string GetLatLongString(int16_t Lat, int16_t Long)
{
	// from _pos>lat_long
	if (Lat == 0 && Long == 0)
		return "Random";
	
	Lat -= 480;
	Long -= 1152;

	Lat = (Lat * 10);
	Long = (Long * 10);

	char NS = ' ';
	if (Lat < 0)
	{
		NS = 'S';
		Lat = -Lat;
	}
	else if (Lat > 0)
	{
		NS = 'N';
	}

	char EW = ' ';
	if (Long < 0)
	{
		EW = 'W';
		Long = -Long;
	}
	else if (Long > 0)
	{
		EW = 'E';
	}

	return std::format("{}{} x {}{}", Lat / 53, NS, Long / 64, EW);
}

std::string GetCoordinatestring(int16_t PosX, int16_t PosY)
{
	return std::format("{} x {}", PosX / 8, PosY / 8);
}

std::string GetStardateString(int16_t Date)
{
	if (Date < 0)
		return "unknown";

	int16_t Year = (Date / 300);
	int16_t Days = (Date % 300);

	return std::format("{}-{}-{}", (Days % 30) + 1, (Days / 30) + 1, Year + 4620);
}

int FindOrbitNumber(const InstanceRecord& StarSystem, const InstanceRecord& Planet)
{
	if (StarSystem.ClassName != "STARSYSTEM")
		return -1;

	StarsytemInstance Data = *reinterpret_cast<StarsytemInstance*>(GetPtrDirect(StarBAddress + StarSystem.DataAddress));

	int OrbitNumber = 1;
	uint8_t OrbitMask = 0x01;

	uint32_t NextRecord = StarSystem.Offset;
	do {
		const auto& Child = AllRecords[NextRecord];
		if (Child.ClassName == "PLANET")
		{
			while ((Data.Orbits & OrbitMask) == 0)
			{
				OrbitNumber++;
				OrbitMask <<= 1;
			}

			if (Child.IAddress == Planet.IAddress)
				return OrbitNumber;

			OrbitNumber++;
			OrbitMask <<= 1;
		}
		else if (Child.ClassName == "BOX" && Child.Value == "ORBIT")
		{
			uint32_t NextSubRecord = Child.Offset;
			do {
				auto& SubChild = AllRecords[NextSubRecord];
				if (SubChild.ClassName == "PLANET")
				{
					while ((Data.Orbits & OrbitMask) == 0)
					{
						OrbitNumber++;
						OrbitMask <<= 1;
					}

					if (SubChild.IAddress == Planet.IAddress)
						return OrbitNumber;

					OrbitNumber++;
					OrbitMask <<= 1;
				}

				NextSubRecord = SubChild.Sibling;
			} while (NextSubRecord != Child.Offset);
		}

		NextRecord = Child.Sibling;
	} while (NextRecord != StarSystem.Offset);

	return -1;
}

uint32_t FindParent(const InstanceRecord& BaseRecord)
{
	uint32_t Parent = BaseRecord.Owner;
	uint32_t NextRecord = BaseRecord.Sibling;

	if (Parent == 0 && NextRecord != 0)
	{
		do
		{
			const auto& SibRecordNode = AllRecords[NextRecord];

			Parent = SibRecordNode.Owner;

			if (Parent != 0)
				break;

			NextRecord = SibRecordNode.Sibling;


		} while (NextRecord != BaseRecord.IAddress);
	}

	return Parent;
}

std::pair<int, StarsytemInstance> FindStarSystemForPlanet(const InstanceRecord& Planet)
{
	StarsytemInstance ReturnData;
	int OrbitNumber = -1;

	if (Planet.ClassName != "PLANET")
		return std::make_pair(-1, ReturnData);

	const auto& ParentStar = AllRecords[FindParent(Planet)];
	if (ParentStar.ClassName == "STARSYSTEM")
	{
		ReturnData = *reinterpret_cast<StarsytemInstance*>(GetPtrDirect(StarBAddress + ParentStar.DataAddress));
		OrbitNumber = FindOrbitNumber(ParentStar, Planet);

	}
	else if (ParentStar.ClassName == "BOX" && ParentStar.Value == "ORBIT")
	{
		const auto& NextParentStar = AllRecords[FindParent(ParentStar)];
		ReturnData = *reinterpret_cast<StarsytemInstance*>(GetPtrDirect(StarBAddress + NextParentStar.DataAddress));
		OrbitNumber = FindOrbitNumber(NextParentStar, Planet);
	}

	return std::make_pair(OrbitNumber, ReturnData);
}

bool IsInactive(InstanceRecord& Record)
{
	int Parent = FindParent(Record);
	while (Parent != 0)
	{
		const auto& ParentRecord = AllRecords[Parent];

		if (ParentRecord.ClassName == "INACTIVE")
			return true;

		int NewParent = FindParent(ParentRecord);
		if (NewParent == Parent)
			return false;

		Parent = NewParent;
	};

	return false;
}

uint32_t FindFirstClassName(std::string ClassName)
{
	for (auto& Entry : AllRecords)
	{
		if (Entry.second.ClassName == ClassName)
			return Entry.first;
	}

	return -1;
}

uint32_t FindBox(std::string BoxName)
{
	for (auto& Entry : AllRecords)
	{
		if (Entry.second.ClassName == "BOX" && Entry.second.Value == BoxName)
			return Entry.first;
	}

	return -1;
}

// ===================================================================

void WriteTextInstanceRecord(std::ofstream& Output, InstanceRecord& Record, bool Long = false)
{
	const auto& Parent = AllRecords[FindParent(Record)];

	if (IsInactive(Record))
	{
		Output << std::format("{} {} ICREATE ( {} @{:06X} INACTIVE)", Record.ClassName, Record.SubClass, Record.Class, Record.IAddress);
	}
	else if (Record.ClassName == "BOX" && Record.Value == "ORBIT")
	{
		uint16_t Orbit = ReadWordDirect(StarBAddress + Record.DataAddress);
		int16_t PosX = ReadWordDirect(StarBAddress + Record.DataAddress + 2);
		int16_t PosY = ReadWordDirect(StarBAddress + Record.DataAddress + 4);

		Output << std::format("{} {} ICREATE ( {} {} @{:06X} )", Record.ClassName, Record.SubClass, Record.Class, Record.Value, Record.IAddress);
		Output << std::format(" Orbit#: {}, MapY: {} MapX: {}\n", Orbit, PosY, PosX);
	}
	else if (Record.ClassName == "BOX" && Record.Value == "TERRAIN VEHICLE")
	{
		uint16_t Mass = ReadWordDirect(StarBAddress + Record.DataAddress);
		uint16_t Hold = ReadWordDirect(StarBAddress + Record.DataAddress + 6);

		Output << std::format("{} {} ICREATE ( {} {} @{:06X} )", Record.ClassName, Record.SubClass, Record.Class, Record.Value, Record.IAddress);
		Output << std::format(" Mass: {} TVHold: {}\n", Mass, Hold);
	}
	else if (Record.ClassName == "BOX")
	{
		Output << std::format("{} {} ICREATE ( {} {} @{:06X} )", Record.ClassName, Record.SubClass, Record.Class, Record.Value, Record.IAddress);
	}
	else if (Record.ClassName == "STARSYSTEM")
	{
		StarsytemInstance Data = *reinterpret_cast<StarsytemInstance*>(GetPtrDirect(StarBAddress + Record.DataAddress));

		Output << std::format("STARSYSTEM: ( @{:06X} )\n", Record.IAddress);
		Output << std::format(" Type: {}", (char)(Record.SubClass));
		Output << std::format(" Star_X: {} Star_Y: {}\n", Data.PosX / 8, Data.PosY / 8);
		Output << std::format(" FlareDay: {} ( {} )\n", (int16_t)Data.FlareDate, GetStardateString(Data.FlareDate));
		if (Long)
		{
			Output << std::format(" Orbits: 0b{:08b}", Data.Orbits);
			Output << std::format(" Reported: 0b{:08b}\n", Data.Reports);
		}
	}
	else if (Record.ClassName == "STAR")
	{
		uint16_t FlareDate = ReadWordDirect(StarBAddress + Record.DataAddress);

		Output << std::format("STAR: ( @{:06X} )\n", Record.IAddress);
		Output << std::format(" Type: {}", (char)(Record.SubClass));
		Output << std::format(" FlareDay: {} ( {} )\n", (int16_t)FlareDate, GetStardateString(FlareDate));
	}
	else if (Record.ClassName == "PLANET")
	{
		Output << std::format("PLANET: ( @{:06X} )\n", Record.IAddress);

		auto StarData = FindStarSystemForPlanet(Record);
		Output << std::format(" Type: {}\n", Record.SubClass);
		Output << std::format(" Star_X: {} Star_Y: {}", StarData.second.PosX / 8, StarData.second.PosY / 8);
		Output << std::format(" Orbit#: {}\n", StarData.first);

	}
	else if (Record.ClassName == "ARTIFACT" || Record.ClassName == "RUIN")
	{
		uint16_t Long = ReadWordDirect(StarBAddress + Record.DataAddress + 2);
		uint16_t Lat = ReadWordDirect(StarBAddress + Record.DataAddress + 4);

		if (Record.ClassName == "ARTIFACT")
		{
			std::string Name = TrimString(std::string(Artifacts[Record.SubClass].Name, 24), '.');

			Output << std::format("ARTIFACT: ( @{:06X} )\n", Record.IAddress);
			Output << std::format(" Type: {} ( {} )\n", Record.SubClass, Name);
		}
		else
		{
			Output << std::format("RUIN: ( @{:06X} )\n", Record.IAddress);
			Output << std::format(" Type: {}\n", Record.SubClass);
		}

		if (Parent.ClassName == "PLANET")
		{
			auto StarData = FindStarSystemForPlanet(Parent);
			Output << std::format(" Star_X: {} Star_Y: {}", StarData.second.PosX / 8, StarData.second.PosY / 8);
			Output << std::format(" Orbit#: {}\n", StarData.first);
			Output << std::format(" Lat: {} Long: {} ( {} )\n", Lat, Long, GetLatLongString(Lat, Long));
		}
		else if (Parent.ClassName == "BOX")
		{
			Output << std::format(" InBox: {}\n", Parent.Value);
		}
	}
	else if (Record.ClassName == "FLUX-NODE")
	{
		uint16_t DestX = ReadWordDirect(StarBAddress + Record.DataAddress + 2);
		uint16_t DestY = ReadWordDirect(StarBAddress + Record.DataAddress + 4);

		uint16_t OriginX = ReadWordDirect(StarBAddress + Record.DataAddress);
		uint16_t OriginY = ReadWordDirect(StarBAddress + Record.DataAddress + 6);

		Output << std::format("FULX-NODE: ( @{:06X} )\n", Record.IAddress);
		Output << std::format(" Origin_X: {} Origin_Y: {}\n", OriginX / 8, OriginY / 8);
		Output << std::format(" Dest_X: {} Dest_Y: {}\n", DestX / 8, DestY / 8);
	}
	else if (Record.ClassName == "NEBULA")
	{
		uint16_t PosY = ReadWordDirect(StarBAddress + Record.DataAddress + 2);
		uint16_t PosX = ReadWordDirect(StarBAddress + Record.DataAddress + 4);

		Output << std::format("NEBULA: ( @{:06X} )\n", Record.IAddress);
		Output << std::format(" Star_X: {} Star_Y: {}", PosX / 8, PosY / 8);
		Output << std::format(" Size: {}\n", Record.SubClass);
	}
	else if (Record.ClassName == "ENCOUNTER")
	{
		EcounterInstance Data = *reinterpret_cast<EcounterInstance*>(GetPtrDirect(StarBAddress + Record.DataAddress));
		Output << std::format("ENCOUNTER: ( @{:06X} )\n", Record.IAddress);
		Output << std::format(" Type: {} ( {} )", Record.SubClass, GetRace(Record.SubClass));
		Output << std::format(" Star_X: {} Star_Y: {}", Data.PosX / 8, Data.PosY / 8);

		if (Parent.ClassName == "PLANET")
		{
			auto StarData = FindStarSystemForPlanet(Parent);
			//Output << std::format(" Star_X: {} Star_Y: {}", StarData.second.PosX / 8, StarData.second.PosY / 8);
			Output << std::format(" Orbit#: {}", StarData.first);
		}
		Output << "\n";
		if (Long)
			Output << std::format(" Distance_X: {} Distance_Y: {}\n", Data.DistX, Data.DistY);

		Output << std::format(" Enabled: {} Homeword: {}", (Data.Enabled ? "TRUE" : "FALSE"), (Data.Homeword ? "TRUE" : "FALSE"));
		Output << std::format(" Fleet Size: {} Max: {}", (int8_t)Data.FleetSize, Data.MaxShipsAtATime);
		if (Data.MaxShipsAtATime == 0xFF)
			Output << " (Infinit)";

		if(Long)
			Output << std::format(" Ship Distroyed: {}", (int8_t)Data.ShipsDestroyed);

		Output << "\n";
		Output << std::format(" Vessels: {} {} {} {}\n", Data.Vessels[0], Data.Vessels[1], Data.Vessels[2], Data.Vessels[3]);
	}
	else if (Record.ClassName == "STRING")
	{
		uint8_t Mark = ReadByteDirect(StarBAddress + Record.DataAddress + 2);

		// Check for a compressed string
		if (Record.DataLength > 3 && Mark == 0x00 || Mark == 0x80)
		{
			Output << std::format("COMPRESSED STRING: ( @{:06X} )\n", Record.IAddress);
		}
		else
		{
			Output << std::format("STRING: ( @{:06X} )\n", Record.IAddress);
		}

		Output << " \"" << Record.Value << "\"\n";
	}
	else if (Record.ClassName == "MESSAGE")
	{
		uint32_t Text = Read3ByteDirect(StarBAddress + Record.DataAddress - 1);

		Output << std::format("MESSAGE: ( @{:06X} )\n", Record.IAddress);

		if (Parent.ClassName == "PLANET")
		{
			auto StarData = FindStarSystemForPlanet(Parent);
			Output << std::format(" Star_X: {} Star_Y: {}", StarData.second.PosX / 8, StarData.second.PosY / 8);
			Output << std::format(" Orbit#: {}\n", StarData.first);
		}

		uint16_t Long = ReadWordDirect(StarBAddress + Record.DataAddress + 2);
		uint16_t Lat = ReadWordDirect(StarBAddress + Record.DataAddress + 4);
		uint16_t Stardate = ReadWordDirect(StarBAddress + Record.DataAddress + 8);
		Output << std::format(" Lat: {} Long: {} ( {} )\n", Lat, Long, GetLatLongString(Lat, Long));
		Output << std::format(" Stardate: {} ( {} )\n", Stardate, GetStardateString(Stardate));

		Output << std::format(" Value: ( @{:06X})", Text);
		if (Long)
		{
			const auto& TextRecord = AllRecords[Text];
			Output << " \"" << TextRecord.Value << '"';
		}
		Output << "\n";
	}
	else if (Record.ClassName == "MESSAGE'")
	{
		uint32_t Text = Read3ByteDirect(StarBAddress + Record.DataAddress - 1);

		Output << std::format("MESSAGE': ( @{:06X} )\n", Record.IAddress);

		Output << std::format(" Value: ( @{:06X})", Text);
		if (Long)
		{
			const auto& TextRecord = AllRecords[Text];
			Output << " \"" << TextRecord.Value << '"';
		}
		Output << "\n";
	}
	else if (Record.ClassName == "CREWMEMBER")
	{
		CrewmemberInstance Data = *reinterpret_cast<CrewmemberInstance*>(GetPtrDirect(StarBAddress + Record.DataAddress));

		Output << std::format("CREWMEMBER: ( @{:06X} )\n", Record.IAddress);
		Output << std::format(" Index: {} Status: {}\n", Data.Index, Data.Status);

		if (Long)
		{
			if ((Data.Status & 0x1) == 1)
			{
				Output << std::format(" Name: {}", std::string(Data.Name + 1, Data.Name[0]));
				Output << std::format(" Species: {} ({})", Record.SubClass, GetCrewmemberSpecies(Record.SubClass + 1));

				Output << std::format(" Health: {}\n", Data.Heath);
				Output << std::format(" Stats: ");
				for (int x = 0; x < 5; x++)
				{
					if (x != 0)
						Output << ",";
					Output << std::format(" {}", Data.Stats[x]);
				}
			}
			else
			{
				Output << " Name: N/A";
				Output << " Species: N/A";
				Output << " Health: N/A\n";
				Output << " Stats: N/A";
			}
			Output << "\n";
		}
	}
	else if (Record.ClassName == "ASSIGN-CREW")
	{
		AssignCrewInstance Data = *reinterpret_cast<AssignCrewInstance*>(GetPtrDirect(StarBAddress + Record.DataAddress));
		Output << std::format("ASSIGN-CREW: ( @{:06X} )\n", Record.IAddress);

		Output << std::format(" Captian: ( @{:06X} )\n", Data.Captian.Full());
		Output << std::format(" Science: ( @{:06X} )\n", Data.Science.Full());
		Output << std::format(" Navigator: ( @{:06X} )\n", Data.Navigator.Full());
		Output << std::format(" Enginer: ( @{:06X} )\n", Data.Engineer.Full());
		Output << std::format(" Communication: ( @{:06X} )\n", Data.Communication.Full());
		Output << std::format(" Doctor: ( @{:06X} )\n", Data.Doctor.Full());
	}
	else if (Record.ClassName == "SHIP")
	{
		ShipInstance Data = *reinterpret_cast<ShipInstance*>(GetPtrDirect(StarBAddress + Record.DataAddress));
		std::string Name = std::string(Data.Name + 1, Data.Name[0]);

		Output << std::format("SHIP: ( @{:06X} )\n", Record.IAddress);
		Output << std::format(" Name: {}\n", Name);
		Output << std::format(" Pos_X: {} Pos_Y: {}\n", (int16_t)Data.PosX, (int16_t)Data.PosY);
		Output << std::format(" Engine: {} Shield: {}", Data.EngineClass, Data.ShieldClass);
		Output << std::format(" Armor: {} Missile: {} Laser: {}\n", Data.ArmorClass, Data.MissileClass, Data.LaserClass);
		Output << std::format(" Acceleration: {} Mass: {} Cargo: {} Pods: {:016B}\n", Data.Acceleration, Data.Mass, Data.Cargo, Data.Pods);

		if (Long)
		{
			Output << std::format(" Flags: {:08B}", Data.Flags[0]);
			Output << std::format(" Sensors: {} Comm: {}", Data.Sensors, Data.Comm);
			Output << std::format(" ShieldVal: {} ArmorVal: {}\n", Data.Shields, Data.Armor);
			Output << std::format(" Repairs:");
			for (int x = 0; x < 7; x++)
			{
				if (x != 0)
					Output << ",";
				Output << std::format(" {}", Data.Repairs[x]);
			}
			Output << "\n";
		}
	}
	else if (Record.ClassName == "TVEHICLE")
	{
		Output << std::format("TVEHICLE: ( @{:06X} )\n", Record.IAddress);
		int Weapon = ReadByteDirect(StarBAddress + Record.DataAddress + 6);
		Output << std::format(" Weapon: {}\n", Weapon);

	}
	else if (Record.ClassName == "ELEMENT")
	{
		uint16_t Ammount = ReadWordDirect(StarBAddress + Record.DataAddress);
		std::string Name = TrimString(std::string(Elements[Record.SubClass].Name, 16), '.');

		Output << std::format("ELEMENT: ( @{:06X} )\n", Record.IAddress);
		Output << std::format(" Type: {} ( {} )", Record.SubClass, Name);
		if (Ammount != 0x2020)
			Output << std::format(" Qty: {}", Ammount);
		Output << "\n";
	}
	else if (Record.ClassName == "NOTICE")
	{
		uint16_t Stardate = ReadWordDirect(StarBAddress + Record.DataAddress - 1);
		Output << std::format("Notice: ( @{:06X} )\n", Record.IAddress);
		Output << std::format(" Stardate: {} ( {} )\n", Stardate, GetStardateString(Stardate));

	}
	else if (Record.ClassName == "ORIGINATOR")
	{
		OriginatorInstance Data = *reinterpret_cast<OriginatorInstance*>(GetPtrDirect(StarBAddress + Record.DataAddress));

		Output << std::format("ORIGINATOR: ( @{:06X} )\n", Record.IAddress);
		Output << std::format(" Origin: {} ( {} )", Record.SubClass, GetRace(Record.SubClass));
		Output << std::format(" Picture: {}", Data.PictureNumber);
		Output << std::format(" Color: ${:02X}\n", Data.Color);
		Output << std::format(" Talk: {}", Data.TalkChance);
		Output << std::format(" Flag: {} ( {} )\n", (Data.Flag ? "TRUE" : "FALSE"), Data.Flag);
		//Output << std::format(" ( Filler: {} {} {} {} )\n", Data.Filler[0], Data.Filler[1], Data.Filler[2], Data.Filler[3]);
		Output << std::format(" InitalPosture: {}", Data.InitalPosture);
		Output << std::format(" NeutralThreshold: {}", Data.NeutralThreshold);
		Output << std::format(" FriendlyThreshold: {}", Data.FriendlyThreshold);
		Output << std::format(" HostileThreshold: {}\n", Data.HostileEffect);
		Output << std::format(" ObsequiousEffect: {}", Data.ObsequiousEffect);
		Output << std::format(" FriendlyEffect: {}", Data.FriendEffect);
		Output << std::format(" HostileEffect: {}", Data.HostileEffect);
		Output << std::format(" MaxWords: {}\n", Data.MaxWords);
		Output << std::format(" Words: (@{:06X})\n", Data.Words.Full());
	}
	else if (Record.ClassName == "SUBJECT")
	{
		Output << std::format("SUBJECT: ( @{:06X} )\n", Record.IAddress);
		Output << std::format(" Type: {} ( {} )\n", Record.SubClass, GetSubject(Record.SubClass));
	}
	else if (Record.ClassName == "PHRASECONTRL")
	{
		PhaseControlInstance Data = *reinterpret_cast<PhaseControlInstance*>(GetPtrDirect(StarBAddress + Record.DataAddress));

		Output << std::format("PHRASECONTRL: ( @{:06X} )\n", Record.IAddress);
		Output << std::format(" Context: {} ( {} )", Data.Context, GetContext(Data.Context));
		Output << std::format(" SentCount: {}\n", Data.SentCount);
		Output << std::format(" OriginatorPostureType : {} ( {} )\n", Data.OriginatorPostureType, GetPosture(Data.OriginatorPostureType));
		Output << std::format(" PostureEffectAlways: {}", Data.Posture_Always);

		if (Data.Posture_Always == -127)
			Output << " ( Nothing )";
		else if (Data.Posture_Always == 127)
			Output << " ( End Call )";

		Output << "\n";

		Output << std::format(" PostureEffectYes: {}\n", Data.Posture_Yes);
		Output << std::format(" PostureEffectNo: {}\n", Data.Posture_No);
	}
	else if (Record.ClassName == "BANK")
	{
		Output << std::format("BANK: ( @{:06X} )\n", Record.IAddress);

		BankInstance Data = *reinterpret_cast<BankInstance*>(GetPtrDirect(StarBAddress + Record.DataAddress));
		Output << std::format(" Balance: {}", (Data.Ballance[0] << 16) | Data.Ballance[1]);
		Output << std::format(" PreTransactionBalance: {}", (Data.PreTransactionBallance[0] << 16) | Data.PreTransactionBallance[1]);
		Output << std::format(" TransactionFlag: {} ( {} )\n", (Data.TransactionFlag ? "TRUE" : "FALSE"), Data.TransactionFlag);
	}
	else if (Record.ClassName == "BANK-TRANS")
	{
		BankTransactionInstance Data = *reinterpret_cast<BankTransactionInstance*>(GetPtrDirect(StarBAddress + Record.DataAddress));

		Output << std::format("BANK-TRANS: ( @{:06X} )\n", Record.IAddress);
		Output << std::format(" Stardate: {} ( {} )", Data.Stardate, GetStardateString(Data.Stardate));
		Output << std::format(" Type: {} ( {} )\n", Record.SubClass, TransactionNames[Record.SubClass]);
		Output << std::format(" Ammount: {}\n", -((Data.Amount[0] << 16) | Data.Amount[1]));
	}
	else if (Record.ClassName == "CAPT-LOG")
	{
		Output << std::format("CAPT-LOG: ( @{:06X} )\n", Record.IAddress);
		Output << std::format(" Value: {}\n", Record.Value);
	}
	else if (Record.DataLength == 0)
	{
		Output << std::format("{} {} ICREATE ( {} @{:06X} )", Record.ClassName, Record.SubClass, Record.Class, Record.IAddress);
	}
	else
	{
		Output << "HUH? " << GetInstanceRecordValue(Record.IAddress);
	}

	//Output << std::format(" ( @{:06X} - @{:06X} )", Record.IAddress, Record.DataAddress + Record.DataLength);
	Output << "\n";
}

void WriteJsonInstanceRecord(std::ofstream& Output, InstanceRecord& Record)
{
	const auto& Parent = AllRecords[FindParent(Record)];

	Output << std::format("\"iaddr\" : \"0x{:06X}\", ", Record.IAddress);

	Output << std::format("\"sibling\" : \"0x{:06X}\", ", Record.Sibling);
	Output << std::format("\"prev\" : \"0x{:06X}\", ", Record.Prev);
	Output << std::format("\"offset\" : \"0x{:06X}\", ", Record.Offset);

	Output << std::format("\"class\" : \"{}\", ", Record.ClassName);
	Output << std::format("\"classID\" : {}, ", Record.Class);
	Output << std::format("\"subclass\" : {}, ", Record.SubClass);

	Output << std::format("\"data\" : \"0x{:06X}\", ", Record.DataAddress);
	Output << std::format("\"length\" : {}", Record.DataLength);

	if (IsInactive(Record))
	{
		Output << ", \"inactive\" : true";
	}
	else if (Record.ClassName == "BOX" && Record.Value == "ORBIT")
	{
		uint16_t Orbit = ReadWordDirect(StarBAddress + Record.DataAddress);
		int16_t PosX = ReadWordDirect(StarBAddress + Record.DataAddress + 2);
		int16_t PosY = ReadWordDirect(StarBAddress + Record.DataAddress + 4);

		Output << ", ";
		Output << std::format("\"boxName\": \"{}\", ", Record.Value);		
		Output << std::format("\"orbitNumber\" : {}, ", Orbit);
		Output << std::format("\"pos\" : \"{} x {}\"", PosY, PosX);
	}
	else if (Record.ClassName == "BOX" && Record.Value == "TERRAIN VEHICLE")
	{
		uint16_t Mass = ReadWordDirect(StarBAddress + Record.DataAddress);
		uint16_t Hold = ReadWordDirect(StarBAddress + Record.DataAddress + 6);

		Output << ", "; 
		Output << std::format("\"boxName\": \"{}\", ", Record.Value);
		Output << std::format("\"terrainVehicleMass\" : {}, ", Mass);
		Output << std::format("\"terrainVehicleHold\" : {}", Hold);
	}
	else if (Record.ClassName == "BOX")
	{
		Output << ", "; 
		Output << std::format("\"boxName\": \"{}\"", Record.Value);
	}
	else if (Record.ClassName == "STARSYSTEM")
	{
		Output << ", \"starsystem\" : { ";

		StarsytemInstance Data = *reinterpret_cast<StarsytemInstance*>(GetPtrDirect(StarBAddress + Record.DataAddress));

		Output << std::format("\"starPos\" : \"{} x {} ({})\", ", Data.PosX, Data.PosY, GetCoordinatestring(Data.PosX, Data.PosY));
		Output << std::format("\"type\" : \"{} ({})\", ", Record.SubClass, StarType(Record.SubClass));
		Output << std::format("\"flareStardate\" : \"{} ({})\", ", Data.FlareDate, GetStardateString(Data.FlareDate));
		Output << std::format("\"orbits\" : \"0b{:08b}\", ", Data.Orbits);
		Output << std::format("\"reported\" : \"0b{:08b}\"", Data.Reports);

		Output << "}";
	}
	else if (Record.ClassName == "STAR")
	{
		Output << ", \"star\" : { ";

		uint16_t FlareDate = ReadWordDirect(StarBAddress + Record.DataAddress);

		Output << std::format("\"type\" : \"{} ({})\", ", Record.SubClass, StarType(Record.SubClass));
		Output << std::format("\"flareStardate\" : \"{} ({})\"", FlareDate, GetStardateString(FlareDate));

		Output << "}";
	}
	else if (Record.ClassName == "PLANET")
	{
		Output << ", \"planet\" : { ";

		auto StarData = FindStarSystemForPlanet(Record);
		Output << std::format("\"type\" : {}, ", Record.SubClass);
		Output << std::format("\"starPos\" : \"{} x {} ({})\", ", StarData.second.PosX, StarData.second.PosY, GetCoordinatestring(StarData.second.PosX, StarData.second.PosY));
		Output << std::format("\"orbitNumber\" : {}", StarData.first);

		Output << "}";
	}
	else if (Record.ClassName == "ARTIFACT")
	{
		std::string Name = TrimString(std::string(Artifacts[Record.SubClass].Name, 24), '.');

		Output << ", \"artifact\" : { ";
		Output << std::format("\"type\" : \"{} ({})\"", Record.SubClass, Name);

		if (Parent.ClassName == "PLANET")
		{
			uint16_t Long = ReadWordDirect(StarBAddress + Record.DataAddress + 2);
			uint16_t Lat = ReadWordDirect(StarBAddress + Record.DataAddress + 4);

			auto StarData = FindStarSystemForPlanet(Parent);
			Output << ", ";
			Output << std::format("\"starPos\" : \"{} x {} ({})\", ", StarData.second.PosX, StarData.second.PosY, GetCoordinatestring(StarData.second.PosX, StarData.second.PosY));
			Output << std::format("\"orbitNumber\" : {}, ", StarData.first);
			Output << std::format("\"pos\" : \"{} x {} ({})\"", Lat, Long, GetLatLongString(Lat, Long));
		}
		else if (Parent.ClassName == "BOX")
		{
			Output << std::format(", \"inBox\" : \"{}\"", Parent.Value);
		}

		Output << "}";
	}
	else if (Record.ClassName == "RUIN")
	{
		Output << ", \"ruin\" : { ";
		Output << std::format("\"type\" : \"{}\"", Record.SubClass);

		uint16_t Long = ReadWordDirect(StarBAddress + Record.DataAddress + 2);
		uint16_t Lat = ReadWordDirect(StarBAddress + Record.DataAddress + 4);

		auto StarData = FindStarSystemForPlanet(Parent);
		Output << ", ";
		Output << std::format("\"starPos\" : \"{} x {} ({})\", ", StarData.second.PosX, StarData.second.PosY, GetCoordinatestring(StarData.second.PosX, StarData.second.PosY));
		Output << std::format("\"orbitNumber\" : {}, ", StarData.first);
		Output << std::format("\"pos\" : \"{} x {} ({})\"", Lat, Long, GetLatLongString(Lat, Long));

		Output << "}";
	}
	else if (Record.ClassName == "FLUX-NODE")
	{
		Output << ", \"flux_node\" : { ";

		uint16_t DestXPos = ReadWordDirect(StarBAddress + Record.DataAddress + 2);
		uint16_t DestYPos = ReadWordDirect(StarBAddress + Record.DataAddress + 4);

		uint16_t OrigXPos = ReadWordDirect(StarBAddress + Record.DataAddress);
		uint16_t OrigYPos = ReadWordDirect(StarBAddress + Record.DataAddress + 6);

		Output << std::format("\"origin\" : \"{} x {} ({})\", ", OrigXPos, OrigYPos, GetCoordinatestring(OrigXPos, OrigYPos));
		Output << std::format("\"destination\" : \"{} x {} ({})\"", DestXPos, DestYPos, GetCoordinatestring(DestXPos, DestYPos));

		Output << "}";
	}
	else if (Record.ClassName == "NEBULA")
	{
		Output << ", \"nebula\" : { ";

		uint16_t XPos = ReadWordDirect(StarBAddress + Record.DataAddress + 2);
		uint16_t YPos = ReadWordDirect(StarBAddress + Record.DataAddress + 4);

		Output << std::format("\"starPos\" : \"{} x {} ({})\", ", XPos, YPos, GetCoordinatestring(XPos, YPos));
		Output << std::format("\"size\" : {}", Record.SubClass);

		Output << "}";
	}	
	else if (Record.ClassName == "ENCOUNTER")
	{
		EcounterInstance Data = *reinterpret_cast<EcounterInstance*>(GetPtrDirect(StarBAddress + Record.DataAddress));

		Output << ", \"encounter\" : {";

		Output << std::format(" \"species\": \"{} ({})\",", Record.SubClass, GetRace(Record.SubClass));
		Output << std::format(" \"starPos\": \"{} x {} ({})\",", Data.PosX, Data.PosY, GetCoordinatestring(Data.PosX, Data.PosY));
		if (Parent.ClassName == "PLANET")
		{
			auto StarData = FindStarSystemForPlanet(Parent);
			//Output << std::format("\"starPos\" : \"{} x {} ({})\", ", StarData.second.PosX, StarData.second.PosY, GetCoordinatestring(StarData.second.PosX, StarData.second.PosY));
			Output << std::format("\"orbitNumber\" : {}, ", StarData.first);
		}

		Output << std::format(" \"distance\" : \"{} x {}\",", Data.DistX, Data.DistY);
		Output << std::format(" \"enabled\": {}, \"homeworld\": {},", (Data.Enabled ? "true" : "false"), (Data.Homeword ? "true" : "false"));
		Output << std::format(" \"fleetSize\" : {}, \"maxAtATime\": {}, \"shipsDestroyed\": {}, ", Data.FleetSize, Data.MaxShipsAtATime, Data.ShipsDestroyed);
		Output << std::format(" \"vessels\" : [ {},{},{},{}]", Data.Vessels[0], Data.Vessels[1], Data.Vessels[2], Data.Vessels[3]);

		Output << "}";
	}
	else if (Record.ClassName == "STRING")
	{
		uint8_t mark = ReadByteDirect(StarBAddress + Record.DataAddress + 2);

		if (Record.DataLength >= 3 && (mark == 0x00 || mark == 0x80))
		{
			Output << " , \"compressed string\" : ";
		}
		else
		{
			Output << " , \"string\" : ";
		}
		Output << "\"" << Record.Value << "\"";
	}
	else if (Record.ClassName == "MESSAGE")
	{
		// Messages use the Subclass as part of the IADDR for the actual message text
		uint32_t Text = Read3ByteDirect(StarBAddress + Record.DataAddress - 1);

		Output << ", \"message\" : {";

		auto StarData = FindStarSystemForPlanet(Parent);
		Output << std::format("\"starPos\" : \"{} x {} ({})\", ", StarData.second.PosX, StarData.second.PosY, GetCoordinatestring(StarData.second.PosX, StarData.second.PosY));
		Output << std::format("\"orbitNumber\" : {}, ", StarData.first);

		uint16_t Long = ReadWordDirect(StarBAddress + Record.DataAddress + 2);
		uint16_t Lat = ReadWordDirect(StarBAddress + Record.DataAddress + 4);
		uint16_t Stardate = ReadWordDirect(StarBAddress + Record.DataAddress + 8);

		Output << std::format("\"Pos\" : \"{} x {} ({})\", ", Lat, Long, GetLatLongString(Lat, Long));
		Output << std::format("\"stardate\": \"{} ({})\", ", Stardate, GetStardateString(Stardate));

		Output << std::format(" \"value\" : \"0x{:06X}\"", Text);

		Output << "}";
	}
	else if (Record.ClassName == "MESSAGE'")
	{
		// Messages use the Subclass as part of the IADDR for the actual message text
		uint32_t Text = Read3ByteDirect(StarBAddress + Record.DataAddress - 1);

		Output << ", \"message_contiuned\" : {";

		Output << std::format(" \"value\" : \"0x{:06X}\"", Text);

		Output << "}";
	}
	else if (Record.ClassName == "CREWMEMBER")
	{
		CrewmemberInstance Data = *reinterpret_cast<CrewmemberInstance*>(GetPtrDirect(StarBAddress + Record.DataAddress));
		Output << ", \"crewmember\" : {";

		Output << std::format(" \"index\": {},", Data.Index);
		Output << std::format(" \"status\": \"0b{:04B}\",", Data.Status);
		if ((Data.Status & 0x1) == 1)
		{
			Output << std::format(" \"name\": \"{}\",", std::string(Data.Name + 1, Data.Name[0]));
			Output << std::format(" \"species\": \"{} ({})\",", Record.SubClass, GetCrewmemberSpecies(Record.SubClass + 1));
		}
		else
		{
			Output << " \"name\": \"\",";
			Output << " \"species\": \"none\",";
		}
		Output << std::format(" \"health\": {},", Data.Heath);
		Output << std::format(" \"stats\": [");
		for (int x = 0; x < 5; x++)
		{
			if (x != 0)
				Output << ",";
			Output << std::format(" {}", Data.Stats[x]);
		}

		Output << "] ";

		Output << "}";
	}
	else if (Record.ClassName == "ASSIGN-CREW")
	{
		AssignCrewInstance Data = *reinterpret_cast<AssignCrewInstance*>(GetPtrDirect(StarBAddress + Record.DataAddress));
		Output << ", \"assign-crew\" : {";

		Output << std::format(" \"captian\": \"0x{:06X}\",", Data.Captian.Full());
		Output << std::format(" \"science\": \"0x{:06X}\",", Data.Science.Full());
		Output << std::format(" \"navigator\": \"0x{:06X}\",", Data.Navigator.Full());
		Output << std::format(" \"enginer\": \"0x{:06X}\",", Data.Engineer.Full());
		Output << std::format(" \"communication\": \"0x{:06X}\",", Data.Communication.Full());
		Output << std::format(" \"doctor\": \"0x{:06X}\" ", Data.Doctor.Full());

		Output << "}";
	}
	else if (Record.ClassName == "SHIP")
	{
		ShipInstance Data = *reinterpret_cast<ShipInstance*>(GetPtrDirect(StarBAddress + Record.DataAddress));
		std::string Name = std::string(Data.Name + 1, Data.Name[0]);

		Output << ", \"ship\" : {";
		
		Output << std::format(" \"name\": \"{}\",", Name);
		Output << std::format(" \"flags\": \"0b{:08B} ({}, {})\",", Data.Flags[0], Data.Flags[0], Data.Flags[1]);
		Output << std::format(" \"pos\": \"{} x {}\",", (int16_t)Data.PosX, (int16_t)Data.PosY);
		Output << std::format(" \"pods\": \"0b{:016B}\",", Data.Pods);
		Output << std::format(" \"cargo\": {},", Data.Cargo);
		Output << std::format(" \"mass\": {},", Data.Mass);
		Output << std::format(" \"acceleration\": {},", Data.Acceleration);

		Output << std::format(" \"engine_class\": {},", Data.EngineClass);
		Output << std::format(" \"shield_class\": {},", Data.ShieldClass);
		Output << std::format(" \"armor_class\": {},", Data.ArmorClass);
		Output << std::format(" \"missile_class\": {},", Data.MissileClass);
		Output << std::format(" \"laser_class\": {},", Data.LaserClass);

		Output << std::format(" \"sensors\": {},", Data.Sensors);
		Output << std::format(" \"communication\": {},", Data.Comm);
		Output << std::format(" \"shields\": {},", Data.Shields);
		Output << std::format(" \"armor\": {},", Data.Armor);
		Output << std::format(" \"repairs\": [");
		for (int x = 0; x < 7; x++)
		{
			if (x != 0)
				Output << ",";
			Output << std::format(" {}", Data.Repairs[x]);
		}
		Output << "]";

		Output << "}";
	}
	else if (Record.ClassName == "TVEHICLE")
	{
		Output << ", \"tvehicle\" : {";
		int Weapon = ReadByteDirect(StarBAddress + Record.DataAddress + 6);
		Output << std::format(" \"weapon\" : {}", Weapon);
		Output << "}";
	}
	else if (Record.ClassName == "ELEMENT")
	{
		uint16_t Ammount = ReadWordDirect(StarBAddress + Record.DataAddress);
		std::string Name = TrimString(std::string(Elements[Record.SubClass].Name, 16), '.');

		Output << ", \"element\" : {";
		Output << std::format(" \"type\": \"{} ({})\"", Record.SubClass, Name);
		if (Ammount != 0x2020)
			Output << std::format(", \"qty\": {}", Ammount);
		Output << "}";
	}
	else if (Record.ClassName == "NOTICE")
	{
		// Notices use the subclass as part of the stardate 
		uint16_t stardate = ReadWordDirect(StarBAddress + Record.DataAddress - 1);
		Output << std::format(", \"stardate\": \"{} ({})\"", stardate, GetStardateString(stardate));
	}
	else if (Record.ClassName == "ORIGINATOR")
	{
		OriginatorInstance Data = *reinterpret_cast<OriginatorInstance*>(GetPtrDirect(StarBAddress + Record.DataAddress));

		Output << ", \"originator\" : { ";
		Output << std::format("\"origin\" : \"{} ({})\", ", Record.SubClass, GetRace(Record.SubClass));
		Output << std::format("\"picture\" : {}, ", Data.PictureNumber);
		Output << std::format("\"color\" : \"0x{:02X}\", ", (Data.Color));
		Output << std::format("\"talk_chance\" : {}, ", Data.TalkChance);
		Output << std::format("\"flag\" : {}, ", Data.Flag);
		Output << std::format("\"filler\" : [{}, {}, {}, {}], ", Data.Filler[0], Data.Filler[1], Data.Filler[2], Data.Filler[3]);
		Output << std::format("\"inital_posture\" : {}, ", Data.InitalPosture);
		Output << std::format("\"neutral_threshold\" : {}, ", Data.NeutralThreshold);
		Output << std::format("\"friendly_threshold\" : {}, ", Data.FriendlyThreshold);
		Output << std::format("\"hostile_threshold\" : {}, ", Data.HostileEffect);
		Output << std::format("\"obsequious_effect\" : {}, ", Data.ObsequiousEffect);
		Output << std::format("\"friendly_effect\" : {}, ", Data.FriendEffect);
		Output << std::format("\"hostile_effect\" : {}, ", Data.HostileEffect);
		Output << std::format("\"max_words\" : {}, ", Data.MaxWords);
		Output << std::format("\"words\" : \"0x{:06X}\" ", Data.Words.Full());

		Output << "}";
	}
	else if (Record.ClassName == "SUBJECT")
	{
		Output << std::format(", \"subject\" : \"{} ({})\"", Record.SubClass, GetSubject(Record.SubClass));
	}
	else if (Record.ClassName == "PHRASECONTRL")
	{
		PhaseControlInstance Data = *reinterpret_cast<PhaseControlInstance*>(GetPtrDirect(StarBAddress + Record.DataAddress));

		Output << ", \"phase-control\" : { ";
		Output << std::format("\"context\" : \"0b{:04B} ({})\", ", Data.Context, GetContext(Data.Context));
		Output << std::format("\"sent_count\" : {}, ", Data.SentCount);
		Output << std::format("\"originator_posture_type\" : \"0b{:04B} ( {} )\", ", Data.OriginatorPostureType, GetPosture(Data.OriginatorPostureType));
		Output << std::format("\"posture_effect_always\" : \"{}", Data.Posture_Always);

		if (Data.Posture_Always == -127)
			Output << " (Nothing)";
		else if (Data.Posture_Always == 127)
			Output << " (End Call)";

		Output << "\", ";

		Output << std::format("\"posture_effect_yes\" : {}, ", Data.Posture_Yes);
		Output << std::format("\"posture_effect_no\" : {}", Data.Posture_No);

		Output << "}";
	}
	else if (Record.ClassName == "BANK")
	{
		BankInstance Data = *reinterpret_cast<BankInstance*>(GetPtrDirect(StarBAddress + Record.DataAddress));
		Output << ", \"bank\" : {";

		Output << std::format(" \"balance\": {},", (Data.Ballance[0] << 16) | Data.Ballance[1]);
		Output << std::format(" \"pre_transaction_balance\": {},", (Data.PreTransactionBallance[0] << 16) | Data.PreTransactionBallance[1]);
		Output << std::format(" \"transaction_flag\": {}", Data.TransactionFlag);

		Output << "}";
	}
	else if (Record.ClassName == "BANK-TRANS")
	{
		BankTransactionInstance Data = *reinterpret_cast<BankTransactionInstance*>(GetPtrDirect(StarBAddress + Record.DataAddress));
		Output << ", \"bank-trans\" : {";

		Output << std::format(" \"stardate\": \"{} ({})\",", Data.Stardate, GetStardateString(Data.Stardate));
		Output << std::format(" \"type\": \"{}\", ", Record.SubClass);
		Output << std::format(" \"ammount\": {}", -((Data.Amount[0] << 16) | Data.Amount[1]));

		Output << "}";
	}
	else if (Record.ClassName == "CAPT-LOG")
	{
		Output << ", \"logEntry\" : \"" << Record.Value << "\"";
	}
	else if (Record.DataLength != 0)
	{
		Output << ", \"huh\" : \"Huh?\"";
	}

	if (Record.DataLength != 0 && Record.ClassName != "STRING")
	{
		Output << ", \"hex\" : \"";

		bool First = true;
		for (int x = 0; x < Record.DataLength; x++)
		{
			if (!First)
				Output << " ";
			Output << std::format("{:02X}", ReadByteDirect(StarBAddress + Record.DataAddress + x));

			First = false;
		}

		Output << "\"";
	}
}

void DumpTreeAsJson(std::ofstream& Output, InstanceTreeRecord& Root)
{
	WriteJsonInstanceRecord(Output, AllRecords[Root.IAddr]);

	if (Root.Children.size() != 0)
	{
		Output << " , \"children\" : [\n";
		bool First = true;

		for (auto& Child : Root.Children)
		{
			if (!First)
				Output << ",\n";

			Output << "{ ";
			DumpTreeAsJson(Output, Child);
			Output << " }";

			First = false;
		}

		Output << "]";
	}
}

void DumpUniverseDataRecord(std::ofstream& Output, InstanceTreeRecord& Root, Image& ImageData)
{
	auto& Record = AllRecords[Root.IAddr];

	if (Record.ClassName == "STARSYSTEM")
	{
		WriteTextInstanceRecord(Output, Record);

		StarsytemInstance Data = *reinterpret_cast<StarsytemInstance*>(GetPtrDirect(StarBAddress + Record.DataAddress));
		ImageData.DrawCircle(Data.PosX, ImageData.Height - Data.PosY, 3, GetScreenColor(StarColor(Record.SubClass), ScreenType::EGA), true);
	}
	else if (Record.ClassName == "NEBULA")
	{
		WriteTextInstanceRecord(Output, Record);

		uint16_t PosX = ReadWordDirect(StarBAddress + Record.DataAddress + 2);
		uint16_t PosY = ReadWordDirect(StarBAddress + Record.DataAddress + 4);
		ImageData.DrawCircle(PosX, ImageData.Height - PosY, Record.SubClass * 8, GetScreenColor(ScreenColor::DkGreen, ScreenType::EGA), false);
	}
	else if (Record.ClassName == "FLUX-NODE")
	{
		WriteTextInstanceRecord(Output, Record);

		uint16_t DestX = ReadWordDirect(StarBAddress + Record.DataAddress + 2);
		uint16_t DestY = ReadWordDirect(StarBAddress + Record.DataAddress + 4);

		uint16_t OriginX = ReadWordDirect(StarBAddress + Record.DataAddress);
		uint16_t OriginY = ReadWordDirect(StarBAddress + Record.DataAddress + 6);

		ImageData.DrawLine(OriginX, ImageData.Height - OriginY, DestX, ImageData.Height - DestY, GetScreenColor(ScreenColor::White, ScreenType::EGA));
	}
	else if (Record.ClassName == "PLANET")
	{
		WriteTextInstanceRecord(Output, Record);
		
		// The planet has something interestig on it, so mark the star system
		if (Root.Children.size() != 0)
		{
			auto StarData = FindStarSystemForPlanet(Record);
			ImageData.DrawCircle(StarData.second.PosX, ImageData.Height - StarData.second.PosY, 7, GetScreenColor(ScreenColor::White, ScreenType::EGA), false);
		}
	}
	else if (Record.ClassName == "ENCOUNTER")
	{
		WriteTextInstanceRecord(Output, Record);
		EcounterInstance Data = *reinterpret_cast<EcounterInstance*>(GetPtrDirect(StarBAddress + Record.DataAddress));

		//ImageData.DrawCircle(Data.PosX, ImageData.Height - Data.PosY, 16, GetScreenColor(ScreenColor::Brown, ScreenType::EGA), false);
	}
	else if (Record.ClassName == "MESSAGE" || Record.ClassName == "MESSAGE'" ||
			 Record.ClassName == "RUIN" || Record.ClassName == "ARTIFACT")
	{
		WriteTextInstanceRecord(Output, Record);
	}

	if (Root.Children.size() != 0)
	{
		for (auto& Child : Root.Children)
		{
			DumpUniverseDataRecord(Output, Child, ImageData);
		}
	}
}

void DumpInstanceData(const std::filesystem::path& OutputPath)
{
	{
		std::filesystem::path OutputFile = OutputPath / "instance" / "INSTANCE.txt";

		std::ofstream Output;
		Output.open(OutputFile);

		if (!Output.is_open())
			abort();

		for (auto& Record : AllRecords)
		{
			WriteTextInstanceRecord(Output, Record.second);
		}

		Output.close();
	}

	{
		std::filesystem::path OutputFile = OutputPath / "INSTANCE.json";

		std::ofstream Output;
		Output.open(OutputFile);

		if (!Output.is_open())
			abort();

		bool First = true;
		Output << "{ \"Records\" : [\n";

		for (auto& Record : AllRecords)
		{
			// Skip the GAP records
			if (Record.second.ClassName != "GAP" && Record.second.ClassName != "GAP-ROOT")
			{
				if (!First)
					Output << ",\n";

				Output << "{ ";
				WriteJsonInstanceRecord(Output, Record.second);
				Output << " }";

				First = false;
			}

		}

		Output << "]\n}\n";

		Output.close();
	}
}

void DumpBoxDataTree(const std::filesystem::path& OutputPath, std::string BoxName)
{
	uint32_t BoxAddr = FindBox(BoxName);
	InstanceTreeRecord BoxRoot = { BoxAddr };

	BuildRecordTree(BoxRoot);

	std::filesystem::path OutputFile = OutputPath / "instance" / (BoxName + ".json");

	std::ofstream Output;
	Output.open(OutputFile);

	if (!Output.is_open())
		abort();

	Output << "{ ";
	DumpTreeAsJson(Output, BoxRoot);
	Output << "}\n";

	Output.close();
}


void DumpClassDataTree(const std::filesystem::path& OutputPath, std::string ClassName)
{
	uint32_t ClassAddr = FindFirstClassName(ClassName);
	InstanceTreeRecord ClassRoot = { ClassAddr };

	BuildRecordTree(ClassRoot);

	std::filesystem::path OutputFile = OutputPath / "instance" / (ClassName + ".json");

	std::ofstream Output;
	Output.open(OutputFile);

	if (!Output.is_open())
		abort();

	Output << "{ ";
	DumpTreeAsJson(Output, ClassRoot);
	Output << "}\n";

	Output.close();
}

void DumpUniverseData(const std::filesystem::path& OutputPath)
{
	Image ImageData(250 * 8, 220 * 8, 0x00, 1);

	uint32_t SecsIAddr = FindBox("SECS");

	InstanceTreeRecord SecsRoot = { SecsIAddr };

	BuildRecordTree(SecsRoot);

	{
		std::filesystem::path OutputFile = OutputPath / "instance" / "UNIVERSE_DATA.json";

		std::ofstream Output;
		Output.open(OutputFile);

		if (!Output.is_open())
			abort();

		Output << "{ ";
		DumpTreeAsJson(Output, SecsRoot);
		Output << "}\n";

		Output.close();
	}


	std::filesystem::path OutputFile = OutputPath / "instance" / "UNIVERSE_DATA.txt";

	std::ofstream Output;
	Output.open(OutputFile);

	if (!Output.is_open())
		abort();

	DumpUniverseDataRecord(Output, SecsRoot, ImageData);

	Output.close();

	WriteImageFile(OutputPath / "img", "MAP", ImageData);

}
