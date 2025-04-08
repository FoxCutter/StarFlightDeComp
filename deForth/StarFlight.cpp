#include <format>
#include <vector>
#include <iostream>

#include "starflight.h"

#include "String.h"
#include "StarflightData.h"
#include "Files.h"
#include "Instance.h"
#include "Output.h"
#include "Rehydrate.h"

#include "X86Disasm/Disassembler.h"
#include "X86Disasm/Results.h"


// ============================================================================ =
// FORTH Word layout
//                  ------ High bit set -----
//                  |                       |
//    +--- | --- | --- | --- | --- | --- | --- | --- | --- | --- | 
//    |  LINK    |  4  |  N     A     M     E  |  Code Ptr | Data ....
//    +--- | --- | --- | --- | --- | --- | --- | --- | --- | --- | 
//                  ^                             ^          ^
//                  +- Length & Flags             |		     + Body Ptr / Data Address
//                  +- Name Ptr                   +- XT Destination

uint16_t StartPtr = 0;
uint16_t UserBase = 0;
uint16_t ContextPtr = 0;
uint16_t DoesPtr = 0;

int ProcessWordFound = 0;
int ProcessDataFound = 0;
int ProcessCodeFound = 0;

VerboseLevel Verbose = VerboseLevel::None;

DiscoveryStage CurrentStage;

std::string GenerateName(uint16_t Address)
{
	std::string Ret;

	if(Address < 0x44C0)
		Ret = std::format("[{}:{:04X}]", "FORTHOUGHT", Address);
	else
		Ret = std::format("[{}:{:04X}]", GetOverlayName(Address), Address);

	return Ret;
}

void ResolveWord(ForthWord &Entry, std::vector<NamingInformation>& RenamingInformation)
{
	if (Entry.Type == WordType::Invalid)
		return;

	// Handle Truncated names
	int16_t ExpectedSize = ((uint16_t)Entry.Flags & 0x1F);
	size_t Size = Entry.Name.size();

	if (ExpectedSize != 0 && Size < ExpectedSize && !HasFlag(Entry.Flags, WordFlags::Truncated))
	{
		size_t size = Entry.Name.size();

		Entry.Name.append(ExpectedSize - size, '_');
		Entry.Flags |= WordFlags::Truncated;
		
		if(Verbose >= VerboseLevel::All)
			std::cout << std::format("   WARN: Truncated dictionary Entry {} was {} expected {}\n", Entry.Name, Size, ((int)Entry.Flags & 0x1F));
	}

	if (Entry.ExecutionToken == 0)
		Entry.ExecutionToken = Entry.BaseAddress;
	
	if (HasFlag(Entry.Flags, WordFlags::Nameless) && Entry.Name.length() == 0)
	{
		if(Entry.Type == WordType::Raw || Entry.Type == WordType::RawCode)
			Entry.Name = GenerateName(Entry.ExecutionToken);
		else
			Entry.Name = GenerateName(Entry.ExecutionToken + 2);
	}

	// Handle renaming
	std::string OverlayName = "";
	if (OverlayAddress(Entry.ExecutionToken))
	{
		OverlayName = GetOverlayName(Entry.ExecutionToken);
	}

	auto Match = std::find_if(RenamingInformation.begin(), RenamingInformation.end(), [Entry, OverlayName](const NamingInformation& obj) {
		if (obj.OverlayName != "*" && obj.OverlayName != OverlayName)
			return false;

		if (obj.MatchName != "")
			return obj.MatchName == Entry.Name;
		
		return obj.CodeFieldPtr == Entry.ExecutionToken;

	});

	if (Match != RenamingInformation.end())
	{
		if (HasFlag(Match->Action, NamingAction::Rename))
		{
			if (Verbose >= VerboseLevel::All && Entry.Name != Match->Name)
				std::cout << std::format("   INFO: Renaming {} to {} in {}\n", Entry.Name, Match->Name, GetOverlayName(Entry.ExecutionToken));
			Entry.Name = Match->Name;

			Entry.Flags &= ~(WordFlags::Nameless | WordFlags::Truncated);
		}

		if (HasFlag(Match->Action, NamingAction::Resize))
		{
			if (Verbose >= VerboseLevel::All)
				std::cout << std::format("   INFO: Resizing {} in {} to {}\n", Entry.Name, GetOverlayName(Entry.ExecutionToken), Match->Size);
			Entry.DataLength = Match->Size;
		}

		if (HasFlag(Match->Action, NamingAction::Retype))
		{
			if (Verbose >= VerboseLevel::All)
				std::cout << std::format("   INFO: Retyping {} in {}\n", Entry.Name, GetOverlayName(Entry.ExecutionToken));

			Entry.Type = Match->Type;
			Entry.SubType = Match->SubType;
			Entry.Param[0] = Match->TypeParam[0];
			Entry.Param[1] = Match->TypeParam[1];
		}

	}

	Entry.Stage = CurrentStage;

	if (Entry.Type == WordType::Raw || Entry.Type == WordType::RawCode)
	{
		// For Raw words, just make sure all the values are sane, and that's done with it
		Entry.DataAddress = Entry.ExecutionToken = Entry.CodePointer = Entry.BaseAddress;
		Entry.Flags |= WordFlags::Unproccessed;
		return;
	}

	if (Entry.DataAddress == 0)
	{
		Entry.DataAddress = Entry.ExecutionToken + 2;
	}

	if (Entry.CodePointer == Entry.DataAddress)
	{
		Entry.Type = WordType::Code;
	}

	// Flag any well known words with speical processing rules	
	if (Entry.Name == "EXIT")
	{
		Entry.Flags |= WordFlags::Exit;
	}
	else if (Entry.Name == "(;CODE)")
	{
		Entry.Flags |= WordFlags::Code;
	}
	else if (Entry.Name == "LIT")
	{
		Entry.Flags |= WordFlags::Lit;
		Entry.ParamCount = 1;
	}
	else if (Entry.Name == "2LIT")
	{
		Entry.Flags |= WordFlags::Lit;
		Entry.ParamCount = 2;
	}
	else if (Entry.Name.find("COMPILE") != -1)
	{
		Entry.Flags |= WordFlags::Lit | WordFlags::Compile;
		Entry.ParamCount = 1;
	}
	else if (Entry.Name == "BRANCH" || Entry.Name == "0BRANCH")
	{
		Entry.Flags |= WordFlags::Lit | WordFlags::Branch;
		Entry.ParamCount = 1;
	}
	else if (Entry.Name.find("LOOP)") != -1)
	{
		Entry.Flags |= WordFlags::Lit | WordFlags::Branch;
		Entry.ParamCount = 1;
	}
	else if (Entry.Name.find("\"") != -1)
	{
		Entry.Flags |= WordFlags::String;
	}
	
	Entry.Flags |= WordFlags::Unproccessed;
}


ForthWord ReadWordFromExecutionToken(uint16_t Address)
{
	if (Address == 0)
	{
		ForthWord Ret;
		Ret.Type = WordType::Invalid;
		return Ret;
	}

	ForthWord Entry;
	Entry.ExecutionToken = Address;
	Entry.DataAddress = Address + 2;
	Entry.CodePointer = ReadWord(Entry.ExecutionToken);

	if (Entry.CodePointer <= 0x100)
	{
		// Was probably a literal
		ForthWord Ret;
		Ret.Type = WordType::Invalid;
		return Ret;
	}

	// Try to find a name
	Address = Entry.ExecutionToken - 1;
	uint8_t Value = ReadByte(Address);
	Address--;

	bool Encoded = !OverlayAddress(Entry.ExecutionToken);

	std::string Name;
	
	// The last byte of the word has the high bit set, 
	// As does the flag byte.
	if ((Value & 0x80) != 0)
	{
		Value = Value & 0x7F;

		while ((Value & 0x80) == 0)
		{
			if (Encoded)
				Value ^= 0x7F;

			if (Value <= 0x20 || Value > 0x7F)
			{
				Name.clear();
				break;
			}

			Name.push_back(Value);
			Value = ReadByte(Address);
			Address--;
		}
	}

	std::reverse(Name.begin(), Name.end());
	
	// 1 charcater name words arn't encoded
	if (Name.size() == 1 && Encoded)
		Name[0] = Name[0] ^ 0x7F;

	// As we're starting from the codepointer, it's hard to be sure that the name
	// Is actually the name (the pattern matches just enough to be really anoying)
	// so we hard match the size. 
	int Size = Value & 0x1F;
	if (Name.empty() || Name.size() != Size)
	{
		Entry.Flags |= WordFlags::Nameless;

		Entry.BaseAddress = Entry.ExecutionToken;
	}
	else
	{
		Entry.Flags |= (WordFlags)Value;
		Address--;

		Entry.Name = Name;

		Entry.LinkPtr = ReadWord(Address);

		Entry.BaseAddress = Address;
	}

	return Entry;
}

ForthWord ReadWordFromNameToken(uint16_t Address)
{
	if (Address == 0)
	{
		ForthWord Ret;
		Ret.Type = WordType::Invalid;
		return Ret;
	}

	ForthWord Entry;
	Entry.BaseAddress = Address - 2;
	Entry.LinkPtr = ReadWord(Address - 2);
	Entry.Flags = (WordFlags)ReadByte(Address);

	bool Encoded = !OverlayAddress(Address);

	Address++;

	if (HasFlag(Entry.Flags, WordFlags::Name))
	{
		uint16_t Length = ((uint16_t)Entry.Flags) & 0x1F;

		for (int x = 0; x < Length; x++)
		{
			uint8_t Value = ReadByte(Address);
			Address++;

			if (!Encoded || Length == 1)
				Entry.Name.push_back((Value & 0x7F));
			else
				Entry.Name.push_back((Value & 0x7F) ^ 0x7F);

			if ((Value & 0x80) != 0)
				break;
		}
	}

	if (Entry.Name.empty())
	{
		Address++;
		if (Verbose >= VerboseLevel::Basic)
			std::cout << std::format("   WARN: Blank dictionary Entry @{:04X}\n", Entry.BaseAddress);

		Entry.Name = "_blank";
		Entry.Flags |= WordFlags::Nameless;
	}

	Entry.ExecutionToken = Address;
	Entry.CodePointer = ReadWord(Address);
	Entry.DataAddress = Address + 2;

	Entry.DataLength = 0;

	return Entry;
}

// Returns the word, and a flag if it was an existing word or not 
std::pair <ForthWord, bool> FindWordFromAddress(uint16_t Address)
{
	// Lets start with the easiest thing, lets see if we're at the start of a word.

	ForthWord Match = Find(Address - 2);
	if (Match.IsValid())
		return std::make_pair(Match, true);

	// We'll have to do some digging then, first the closest word.
	ForthWord Prev;
	if(OverlayAddress(Address))	
		Prev = Files[CurrentOverlay].Words.FindBefore(Address);
	else
		Prev = Files[0x0000].Words.FindBefore(Address);

	Address -= 2;

	// Lets start going backwards until we find a valid codepointer.
	while (Address > Prev.BaseAddress + Prev.DataLength)
	{
		uint16_t Token = ReadWord(Address);
		
		if (FindCodePointer(Token).IsValid())
		{
			// Well, we found a code pointer, lets see if we have something we already know.
			Match = Find(Address);

			// If it's matched, we'll just return that.
			if(Match.IsValid())
				return std::make_pair(Match, true);

			// Read the word we found
			Match = ReadWordFromExecutionToken(Address);
			if (Match.IsValid())
				return std::make_pair(Match, false);
		}

		Address--;
	}



	ForthWord Ret;
	Ret.Type = WordType::Invalid;
	return std::make_pair(Ret, false);
}


void ReadLinkedDictionary(std::vector<NamingInformation>& NamingInformation, uint16_t Address)
{
	uint16_t LinkedFrom = Address;

	while (Address != 0)
	{
		ForthWord CurrentEntry = ReadWordFromNameToken(Address);

		if (CurrentEntry.IsValid())
		{
			ForthWord Existing = Find(CurrentEntry.ExecutionToken);

			if (Existing.IsValid())
			{
				// If we get to a point in the tree where we found an entry that is already defined
				// Just bail out as everything after it will be existing as well.
				if (!HasFlag(Existing.Flags, WordFlags::Undefined))
				{
					if (Verbose >= VerboseLevel::Debug)
						std::cout << std::format("  INFO: Exiting Early as {} is already a known word\n", Existing.Name.c_str());
					break;
				}

				CurrentEntry.Flags |= Existing.Flags;
				CurrentEntry.Flags &= ~(WordFlags::Undefined);
				CurrentEntry.DataLength = Existing.DataLength;
				CurrentEntry.Type = Existing.Type;
				if (Existing.Name.length() != 0)
					CurrentEntry.Name = Existing.Name;

				ResolveWord(CurrentEntry, NamingInformation);
				Update(CurrentEntry);
			}
			else
			{
				ResolveWord(CurrentEntry, NamingInformation);
				Insert(CurrentEntry);
			}
		}

		Address = CurrentEntry.LinkPtr;
	}
}


bool LoadStarFlight1(const std::filesystem::path& BasePath, VerboseLevel verbose)
{
	Verbose = verbose;
	CurrentStage = DiscoveryStage::Internal;

	std::filesystem::path StarFltPath = BasePath / "STARFLT.COM";
	std::filesystem::path StarAPath = BasePath / "STARA.COM";
	std::filesystem::path StarBPath = BasePath / "STARB.COM";

	LoadFiles(StarFltPath, StarAPath, StarBPath);

	LoadOverlay(0);

	// Startup Values
	StartPtr = ReadWord(0x0129);
	UserBase = ReadWord(0x012B) + 2;
	DoesPtr = 0x1649;
	ContextPtr = ReadWord(UserBase + 0x0040);

	// This one is manditory, as the (") word is nameless, there is no way to tell that it's string word, so we have to make sure to set that here
	// If we don't things will crash
	auto StringImp = ReadWordFromExecutionToken(0x3F39);
	StringImp.Name = "(\")";
	StringImp.Flags &= ~WordFlags::Nameless;
	auto Dummy = std::vector<NamingInformation>();

	ResolveWord(StringImp, Dummy);
	Insert(StringImp);

	Verbose = VerboseLevel::None;

	return true;
}

bool ProcessDictionaries(std::vector<NamingInformation>& NamingInformation)
{
	CurrentStage = DiscoveryStage::Dictionaries;

	LoadOverlay(0x0000);
	InsertCount = 0;

	if (Verbose >= VerboseLevel::Basic)
		std::cout << std::format(" INFO: Reading dictionaries in {}\n", GetOverlayName(ContextPtr));

	// Load the Dictionary lists via the ContextPtr
	for (int x = 0; x < 7; x++)
	{
		uint16_t DictionaryBase = ReadWord(ContextPtr);
		if (Verbose >= VerboseLevel::Debug)
			std::cout << std::format(" INFO: Dictionary Root Entry: {:04X}\n", DictionaryBase);

		ContextPtr += 2;
		if (DictionaryBase != 0)
			ReadLinkedDictionary(NamingInformation, DictionaryBase);
	}

	if (Verbose >= VerboseLevel::Basic)
		std::cout << std::format(" INFO: Inserted {} Words in {}\n", InsertCount, GetOverlayName(ContextPtr));


	for (auto& File : Files)
	{
		InsertCount = 0;

		if (File.second.Type == FileType::Overlay)
		{
			LoadOverlay(File.first);
			if (Verbose >= VerboseLevel::All)
				std::cout << std::format(" INFO: Reading dictionaries in {}\n", GetOverlayName(File.second.Base));

			ForthWord newHeader{ "_overlayHeader", WordType::Raw, WordSubType::OverlayHeader, WordFlags::None, File.second.Base, File.second.Base ,0, File.second.Base, sizeof(OverlayHeader) };
			ResolveWord(newHeader, NamingInformation);
			Insert(newHeader);
			OverlayHeader* Header = reinterpret_cast<OverlayHeader*>(GetPtrDirect(File.second.Address));

			// We can have more data in the file then what is loaded in the overlay, so just mark that now. 
			uint16_t OverlayEnd = Header->LoadAddress + (Header->Size << 4);
			uint16_t DataEnd = Header->DataPtr;
			ForthWord overlayJunk{ "_overlayJunk", WordType::Raw, WordSubType::OverlayJunk, WordFlags::None, DataEnd, DataEnd ,0, DataEnd, (uint16_t)(OverlayEnd - DataEnd) };
			ResolveWord(overlayJunk, NamingInformation);
			Insert(overlayJunk);

			for (int x = 0; x < 4; x++)
			{
				if (Verbose >= VerboseLevel::Debug)
					std::cout << std::format("  INFO: Dictionary Root Entry: {:04X}\n", Header->Link[x]);

				if (Header->Link[x] != 0)
					ReadLinkedDictionary(NamingInformation, Header->Link[x]);
			}
		
			if (Verbose >= VerboseLevel::Basic)
				std::cout << std::format(" INFO: Inserted {} Words in {}\n", InsertCount, GetOverlayName(File.second.Base));

			UnloadOverlay();
		}
		else if (File.second.Type == FileType::Video)
		{
			LoadOverlay(File.first);

			if (Verbose >= VerboseLevel::All)
				std::cout << std::format(" INFO: Reading video entries in {}\n", GetOverlayName(File.second.Base));

			uint16_t Address = File.second.Base;

			uint16_t Offsets[15];
			for (int x = 0; x < 15; x++)
			{
				Offsets[x] = ReadWord(Address);
				Address += 2;
			}

			ForthWord PointTable{ "_pointerTable", WordType::Raw, WordSubType::PointerTable, WordFlags::None, 0, 0, 0, 0, 30 };
			PointTable.BaseAddress = PointTable.DataAddress = PointTable.ExecutionToken = File.second.Base;
			ResolveWord(PointTable, NamingInformation);
			Insert(PointTable);

			Address = File.second.Base;
			for (int x = 0; x < 15; x++)
			{
				if (Offsets[x] != 0 && Offsets[x] != 0x2020)
				{
					ForthWord NewCodeEntry = ReadWordFromExecutionToken(Address + Offsets[x] - 2);

					ResolveWord(NewCodeEntry, NamingInformation);
					NewCodeEntry.Type = WordType::Code;
					Insert(NewCodeEntry);
				}
			}

			if (Verbose >= VerboseLevel::Basic)
				std::cout << std::format(" INFO: Inserted {} Words in {}\n", InsertCount, GetOverlayName(File.second.Base));

			UnloadOverlay();
		}
	}

	return true;
}

ForthWord CreateCodePointer(ForthWord Entry, std::vector<NamingInformation>& RenamingInformation)
{

	ForthWord Ret;
	Ret.Type = WordType::Invalid;

	// If the (;CODE) word isn't in the Dictionary already, we're in trouble. The (;CODE) sets the current 
	// word's code pointer to the following ASM, so it's the low level word that DOES> uses for creating 
	// code pointers. So it needs to be the word before the code we're looking at.
	ForthWord Code = Files[0x0000].Words.FindName("(;CODE)");

	if (Code.IsValid() && ReadWord(Entry.CodePointer - 2) == Code.ExecutionToken)
	{	
		ForthWord Match;

		ForthWord Prev;
		if(OverlayAddress(Entry.CodePointer))
			Prev = Files[CurrentOverlay].Words.FindBefore(Entry.CodePointer);
		else
			Prev = Files[0x0000].Words.FindBefore(Entry.CodePointer);
			
		// We need to work backwards from the code pointer to find the word. Which may be previouse word,
		// But might also not be.
		int Address = Entry.CodePointer - 2;
		while (Address > Prev.ExecutionToken)
		{
			auto Val = FindCodePointer(ReadWord(Address));
			if (Val.IsValid())
				break;

			Address--;
		};

		Match = Find(Address);

		if (!Match.IsValid())
		{
			ForthWord NewCodeEntry = ReadWordFromExecutionToken(Address);

			ResolveWord(NewCodeEntry, RenamingInformation);
			Insert(NewCodeEntry);

			Match = NewCodeEntry;
		}
				
		// Raw code/data can't be a DOES> style code pointer, so it's probably a word we havn't found yet.
		if (Match.Type != WordType::Raw && Match.Type != WordType::RawCode)
		{
			Ret = Match;
			Ret.Type = WordType::Data;
			Ret.CodePointer = Ret.ExecutionToken = Entry.CodePointer;

			if (Ret.Name == ":")
			{
				Ret.Type = WordType::Word;
			}
			else if (Ret.Name == "(CREATE)")
			{
				Ret.Name = "CREATE";
				// Shorten the length of the name by 2
				Ret.Flags = Ret.Flags - (WordFlags)(2); 
				Ret.Type = WordType::Data;
			}
			else if (Ret.Name == "USER")
			{
				Ret.Type = WordType::Data;
				Ret.Flags |= WordFlags::User;
			}
			else if (Ret.Name == "USEREXECUTOR")
			{
				Ret.Type = WordType::Word;
				Ret.Flags |= WordFlags::User;
			}
			else if (Ret.Name == "OVERLAY")
			{
				Ret.Type = WordType::Overlay;
			}
			else if (Ret.Name == "SYN")
			{
				Ret.Type = WordType::Data;
				Ret.SubType = WordSubType::Syn;
			}
			else if (Ret.Name == "VOCABULARY")
			{
				Ret.SubType = WordSubType::Vocabulary;
			}
			else if (Ret.Name == "AFIELD")
			{
				Ret.SubType = WordSubType::AField;
			}
			else if (Ret.Name == "IFIELD")
			{
				Ret.SubType = WordSubType::IField;
			}
			else if (Ret.Name == "EXPERT")
			{
				Ret.SubType = WordSubType::Expert;
			}
			else if (Ret.Name == "CASE")
			{
				Ret.SubType = WordSubType::Case;
			}
			else if (Ret.Name == "CASE:")
			{
				Ret.SubType = WordSubType::CaseEx;
			}
			else if (Ret.Name == "_iaddr_array")
			{
				Ret.SubType = WordSubType::IAddrArray;
			}
			else if (Ret.Name == "_probability-array")
			{
				Ret.SubType = WordSubType::ProbabilityArray;
			}
			else if (Ret.Name == "_color_c" ||
				     Ret.Name == "_dir_field" ||
				     Ret.Name == "_combat_field")
			{
				Ret.SubType = WordSubType::ByteValue;
			}
			else if (Ret.Name == "CONSTANT" || Ret.Name == "SIGFLD")
			{
				Ret.SubType = WordSubType::WordValue;
			}
			else if (Ret.Name == "2C=")
			{
				Ret.SubType = WordSubType::DoubleWordValue;
			}
			else if (Ret.Name == "TABLE" ||
					 Ret.Name == "_word_array")
			{
				Ret.SubType = WordSubType::WordArray;
			}
			else if (Ret.Name == "_one_or_many")
			{
				Ret.SubType = WordSubType::TwoDoubleWordValue;
			}
			else if (Ret.Name == "_?check")
			{
				Ret.SubType = WordSubType::TwoWordValue;
			}
		}
	}
	else
	{
		// Otherwise we're pointing to code that isn't attached to a normal word. 

		ForthWord Real;

		Real = Find(Entry.CodePointer - 2);
		if (Real.IsValid())
		{
			Entry.Type = WordType::Word;
			Entry.SubType = WordSubType::Alias;
			Update(Entry);

			if (Verbose >= VerboseLevel::All)
				std::cout << std::format("  INFO: Entry is an {} ALIAS of {} in {}\n", Entry.Name, Real.Name, GetOverlayName(Entry.BaseAddress));
		}
		else
		{

			Real = Files[CurrentOverlay].Words.FindWord(Entry.CodePointer - 2);

			if (!Real.IsValid())
				Real = Files[0x0000].Words.FindWord(Entry.CodePointer - 2);

			if (!Real.IsValid())
			{
				// A code pointer that went nowhere
				if (Verbose >= VerboseLevel::All)
					std::cout << std::format("  INFO: Bogus code pointer {} in {}\n", Ret.Name, Entry.Name);

			}
			else
			{
				// Or an alias.
				Ret = Real;
				Ret.DataAddress = 0;
				Ret.BaseAddress = Ret.ExecutionToken = Entry.CodePointer;

				Ret.Type = WordType::Word;
				Entry.SubType = WordSubType::Alias;

				if (Verbose >= VerboseLevel::All)
					std::cout << std::format("  INFO: Entry is an {} ALIAS of {} in {}\n", Entry.Name, Real.Name, GetOverlayName(Entry.ExecutionToken));
			}
		}
	}

	
	if (Ret.Type != WordType::Invalid)
	{
		ResolveWord(Ret, RenamingInformation);
	
		if (OverlayAddress(Entry.CodePointer))
		{
			Files[CurrentOverlay].CodeFields.Insert(Ret);
		}
		else
		{
			Files[0x0000].CodeFields.Insert(Ret);
		}

		if (Verbose >= VerboseLevel::Basic)
			std::cout << std::format("  INFO: Code Pointer Entry {} in {}\n", Ret.Name, GetOverlayName(Entry.CodePointer));
	}

	return Ret;
}

void ProcessCodePointers(std::vector<NamingInformation>& RenamingInformation)
{
	CurrentStage = DiscoveryStage::CodePointers;

	// Code Pointers are made by normal code words (defined by : ) this includes the : word itself. 
	// So we start by finding that word and making it a code word.
	{
		ForthWord Entry = Files[CurrentOverlay].Words.FindName(":");
		if (!Entry.IsValid())
		{
			std::cout << std::format("PANIC: Unable to find ':' word.");
			abort();
		}
		Entry.Type = WordType::Word;
		Entry.ExecutionToken = Entry.CodePointer;

		ResolveWord(Entry, RenamingInformation);
		Files[0x0000].CodeFields.Insert(Entry);

		if (Verbose >= VerboseLevel::Basic)
			std::cout << std::format("  INFO: Code Pointer Entry {} in {}\n", Entry.Name, Files[0].Name);
	}

	for (auto& File : Files)
	{
		if (File.second.Type == FileType::Main || File.second.Type == FileType::Overlay/* || File.second.Type == FileType::Video*/)
		{
			LoadOverlay(File.first);

			// We need to make a deep copy of the word list because we will be adding to it
			// which will break the foreach loop below.   
			auto WordList = Files[CurrentOverlay].Words;

			for (auto& Entry : WordList)
			{
				if (HasFlag(Entry.Flags, WordFlags::Undefined))
				{
					if (Verbose >= VerboseLevel::Debug)
						std::cout << std::format(" WARN: {} is marked Undefined\n", Entry.Name);
				}
				else
				{
					// At this point, all the word entries should be valid, so all the code pointers should be valid, so if we can't find them
					// We'll create them.
					if (Entry.Type != WordType::Code && Entry.Type != WordType::RawCode && Entry.Type != WordType::Raw)
					{
						auto CodePointer = FindCodePointer(Entry.CodePointer);
						if (!CodePointer.IsValid())
						{
							CreateCodePointer(Entry, RenamingInformation);
						}
					}
				}
			}

			UnloadOverlay();
		}
	}
}

ForthWord LastUserData;

void ProcessUserWord(ForthWord &Entry, std::vector<NamingInformation>& RenamingInformation)
{
	if (!HasFlag(Entry.Flags, WordFlags::User))
		return;

	// User words have an extra level of indirection, the value is the offset into the user data 
	uint16_t Index = ReadWord(Entry.DataAddress);
	uint16_t Address = UserBase + Index;

	Entry.DataLength = 2;
	
	// First off, we want to make a 'raw' entry for where the user data is being stored for this word
	auto DataEntry = Find(Address);
	if (!DataEntry.IsValid())
	{
		if (Entry.Type == WordType::Word)
			DataEntry.Name = std::format("USER_PTR:{}", Entry.Name);
		else
			DataEntry.Name = std::format("USER:{}", Entry.Name);

		DataEntry.Type = WordType::Raw;
		DataEntry.Flags = WordFlags::None;
		DataEntry.BaseAddress = Address;
		DataEntry.DataLength = 2;

		ResolveWord(DataEntry, RenamingInformation);
		Insert(DataEntry);
	}

	if (LastUserData.BaseAddress != 0 && LastUserData.DataAddress + LastUserData.DataLength != DataEntry.BaseAddress)
	{
		if (LastUserData.BaseAddress != DataEntry.BaseAddress)
		{
			// The defined user data wasn't actualy two bytes
			LastUserData.DataLength = DataEntry.BaseAddress - LastUserData.DataAddress;
			Update(LastUserData);
		}
	}

	LastUserData = DataEntry;

	// For Data, the user data entry is the actuall data, but for words it's just the XT to use
	// So we want to make sure it exists
	if (Entry.Type == WordType::Word)
	{
		uint16_t IndirectXT = ReadWord(Address) - 2;
		auto IndirectEntry = Find(IndirectXT);

		if (!IndirectEntry.IsValid())
		{
			IndirectEntry = ReadWordFromExecutionToken(IndirectXT);
			if (IndirectEntry.IsValid())
			{
				// Looks like we found an entry, so added it to the Dictionary
				if (HasFlag(IndirectEntry.Flags, WordFlags::Nameless))
				{
					IndirectEntry.Name = std::format("USER:{}", Entry.Name);
					IndirectEntry.Flags &= ~WordFlags::Nameless;
				}

				ResolveWord(IndirectEntry, RenamingInformation);
				Insert(IndirectEntry);
			}
		}
	}
}

std::vector<FieldInfo> KnowFields;

void ProcessKnowDataTypes(ForthWord& Entry, std::vector<NamingInformation>& RenamingInformation)
{
	uint16_t Address = Entry.DataAddress;
	ForthWord Next = Files[CurrentOverlay].Words.FindAfter(Entry.BaseAddress);

	if (Address > Next.BaseAddress)
	{
		std::cout << std::format(" ERROR: Bogus Data found {}\n", Entry.Name);
		return;
	}

	switch (Entry.SubType)
	{
		case WordSubType::Alias:
			Entry.DataLength = 0;
			break;

		case WordSubType::ByteValue:
			Entry.DataLength = 1;
			break;

		case WordSubType::WordValue:
		case WordSubType::Syn:
			Entry.DataLength = 2;
			break;

		case WordSubType::TwoWordValue:
			Entry.DataLength = 4;
			break;

		case WordSubType::TwoDoubleWordValue:
			Entry.DataLength = 8;
			break;


		case WordSubType::Vocabulary:
			// The FORTH vocabulary word is smaller, even if it uses the same code pointer  
			if (Entry.Name == "FORTH")
				Entry.DataLength = 7 * 2;
			else
				Entry.DataLength = 10 * 2;

			break;
		
		case WordSubType::WordArray:
			if (Entry.Param[1] != 0)
			{
				uint32_t Len = ReadWord(Entry.DataAddress);
				Entry.DataLength = (Len + 1) * 2;
			}
			break;

		case WordSubType::DoubleWordArray:
			if (Entry.Param[1] != 0)
			{
				uint32_t Len = ReadDoubleWord(Entry.DataAddress);
				Entry.DataLength = (Len + 1) * 4;
			}
			break;

		case WordSubType::IAddrArray:
			// Annoyingly, the version of _iaddr_array in HP-OV is slightly different then in other places
			// In other locations the array starts with a 3 byte long length, but in HP-OV it does not.
			if (GetOverlayName(Entry.BaseAddress) != "HP-OV")
			{
				// Mark it has haveing the length
				Entry.Param[1] = 0xFFFF;
			}

			if (Entry.Param[1] != 0)
			{
				uint32_t Len = Read3Byte(Entry.DataAddress);
				Entry.DataLength = (Len + 1) * 3;
			}

			break;

		case WordSubType::AField:
		case WordSubType::IField:
			{
				FieldInfo Field;

				Field.Type = (Entry.SubType == WordSubType::AField ? 'A' : 'I');

				Field.File = ReadByte(Entry.DataAddress);
				Field.Offset = ReadByte(Entry.DataAddress + 1);
				Field.Length = ReadByte(Entry.DataAddress + 2);

				if (HasFlag(Entry.Flags, WordFlags::Nameless))
				{
					auto File = FindFileID(Field.File);
					Entry.Name = std::format("[{}-{}:{}-{}]", (Entry.SubType == WordSubType::AField ? 'A' : 'I'), File.Name, Field.Offset, Field.Offset + Field.Length);

					ResolveWord(Entry, RenamingInformation);
				}

				Field.Overlay = CurrentOverlay;
				Field.Name = Entry.Name;

				KnowFields.emplace_back(Field);

				Entry.DataLength = (Entry.SubType == WordSubType::AField ? 6 : 3);

			}
			break;

		case WordSubType::ProbabilityArray:
			{
				int Count = ReadWord(Entry.DataAddress) / 6;
				Address = Entry.DataAddress + 2;

				uint16_t LastAddress = 0;

				// Find the last address in the probablity table
				for (int x = 0; x < Count; x++)
				{
					uint16_t SubAddress = ReadWord(Address + 4);
					if (SubAddress > LastAddress)
						LastAddress = SubAddress;

					Address += 6;
				}

				// Then find the 100% chance in the table
				while (true)
				{
					uint8_t Percent = ReadByte(LastAddress);
					LastAddress += 4;

					if (Percent >= 100)
						break;
				}

				Entry.DataLength = LastAddress - Entry.DataAddress;
			}
			break;
		
		case WordSubType::Case:
			{
				uint16_t Count = ReadWord(Address);
				Address += 2;

				// Get the word for the 'other' case
				uint16_t Value = ReadWord(Address) - 2;
				Address += 2;

				auto Match = Find(Value);
				if (!Match.IsValid())
				{
					Match = ReadWordFromExecutionToken(Value);

					// Valid Word
					if (Match.IsValid())
					{
						ResolveWord(Match, RenamingInformation);

						Insert(Match);

						ProcessDataFound++;
					}
				}

				for (int x = 0; x < Count; x++)
				{
					// Skip the case value
					Address += 2;

					// And get the code pointer of the word to execute
					Value = ReadWord(Address) - 2;
					Address += 2;

					Match = Find(Value);

					if (!Match.IsValid())
					{
						Match = ReadWordFromExecutionToken(Value);

						// Valid Word
						if (Match.IsValid())
						{
							ResolveWord(Match, RenamingInformation);

							Insert(Match);

							ProcessDataFound++;
						}
					}
				}

				Entry.DataLength = (4 * (Count + 1));
			}
			break;

		case WordSubType::CaseEx:
			{
				uint16_t Count = (Next.BaseAddress - Entry.DataAddress) / 2;

				for (int x = 0; x < Count; x++)
				{
					uint16_t Value = ReadWord(Address);
					Address += 2;

					auto Match = Find(Value);

					if (!Match.IsValid())
					{
						Match = ReadWordFromExecutionToken(Value);

						// Valid Word
						if (Match.IsValid())
						{
							ResolveWord(Match, RenamingInformation);

							Insert(Match);

							ProcessDataFound++;
						}
					}
				}

				Entry.DataLength = (Next.BaseAddress - Entry.DataAddress);
			}
			break;

		case WordSubType::Expert:
			{
				uint8_t RuleLimit = ReadByte(Entry.DataAddress);
				uint8_t RuleCount = ReadByte(Entry.DataAddress + 2);

				uint16_t RuleArray = Entry.DataAddress + 3;
				uint16_t ConditionArray = RuleArray + (RuleLimit * 2);

				uint16_t LastAddress = 0;

				for (int x = 0; x < RuleCount; x++)
				{
					uint16_t Address = ReadWord(RuleArray + (x * 2));
					uint8_t Count = ReadByte(Address);
					Address++;

					uint16_t WordValue = ReadWord(Address) - 2;
					Address += 2;

					auto Match = Find(WordValue);
					if (!Match.IsValid())
					{
						Match = ReadWordFromExecutionToken(WordValue);

						// Is Valid
						if (Match.IsValid())
						{
							ResolveWord(Match, RenamingInformation);

							Insert(Match);

							ProcessDataFound++;
						}
					}


					for (int Pos = 0; Pos < Count; Pos++)
					{
						uint8_t Condition = ReadByte(Address) & 0x7F;
						Address++;

						uint16_t WordValue = ReadWord(ConditionArray + (Condition * 2)) - 2;

						auto Match = Find(WordValue);
						if (!Match.IsValid())
						{
							Match = ReadWordFromExecutionToken(WordValue);

							// Is Valid
							if (Match.IsValid())
							{
								ResolveWord(Match, RenamingInformation);

								Insert(Match);

								ProcessDataFound++;
							}
						}

					}

					if (Address > LastAddress)
						LastAddress = Address;
				}

				Entry.DataLength = LastAddress - Entry.DataAddress;
			}
			break;

		default:
			// Simple quick test to see if there's a field after word sized data, just checking for a code pointer.
			uint16_t Token = ReadWord(Entry.DataAddress + 2);
			if (FindCodePointer(Token).IsValid())
			{
				auto Match = ReadWordFromExecutionToken(Entry.DataAddress + 2);
				if (Match.IsValid() && !Find(Match.ExecutionToken).IsValid())
				{
					ResolveWord(Match, RenamingInformation);

					Insert(Match);

					ProcessDataFound++;
				}
			}
			break;
	}
}







int ProcessASM(ForthWord& Entry, std::vector<NamingInformation>& RenamingInformation, uint16_t StartAt)
{
	uint16_t Address = StartAt;

	uint16_t JumpAddress = 0xFFFF;
	uint16_t JumpSetAddress = 0xFFFF;
	bool Done = false;

	do
	{
		if (Address == JumpAddress)
			JumpAddress = 0xFFFF;

		auto Opcode = X86Disasm::ReadOpcode(GetPtr(Address), 16);

		Address += Opcode.Length;
		
		if (IsJump(Opcode))
		{
			if (HasFarAddress(Opcode))
			{
				// Far Jumps end the function
				Done = true;
			}
			else if (HasFlag(Opcode.OpcodeData->Flags, X86Disasm::OpcodeFlags::NoModRM))
			{
				// We only care if we're jumping forwards
				if (Opcode.Immidate.Word[0] > 0)
				{
					JumpAddress = Address + (int16_t)Opcode.Immidate.Word[0];
					JumpSetAddress = Address - Opcode.Length;
				}
			}
			else
			{
				// Jump via an register or indirect via register is usually the NEXT token
				Done = true;
			}

		} 
		else if (IsCall(Opcode))
		{
			if (!HasFarAddress(Opcode) && HasFlag(Opcode.OpcodeData->Flags, X86Disasm::OpcodeFlags::NoModRM))
			{
				uint16_t CallAddress = Address + (uint16_t)Opcode.Immidate.Word[0];
				
				auto CallEntry = Find(CallAddress - 2);

				if (!CallEntry.IsValid())
					CallEntry = Find(CallAddress);
				
				if (CallEntry.IsValid())
				{
					if (CallEntry.Type == WordType::Data)
					{
						if (Verbose >= VerboseLevel::Basic)
							std::cout << std::format("  INFO: {} in {} is really Code\n", CallEntry.Name, GetOverlayName(CallEntry.BaseAddress));

						CallEntry.Type = WordType::Code;
						CallEntry.DataLength = 0;
						CallEntry.Flags |= WordFlags::Unproccessed;

						Update(CallEntry);
					}
				}
				else
				{
					// We're calling code or Raw Code we've never run into before, so lets create it.
					if (CallAddress > 0x100)
					{
						CallEntry = ReadWordFromExecutionToken(CallAddress - 2);

						if (CallEntry.DataAddress == CallEntry.CodePointer)
						{
							CallEntry.Type = WordType::Code;
							ResolveWord(CallEntry, RenamingInformation);
						}
						else
						{
							auto CodePointer = FindCodePointer(CallEntry.CodePointer);

							if (!CodePointer.IsValid())
							{
								CallEntry = ReadWordFromExecutionToken(CallAddress);
								CallEntry.Type = WordType::RawCode;
								CallEntry.ExecutionToken = CallEntry.BaseAddress = CallEntry.CodePointer = CallEntry.DataAddress = CallAddress;

								ResolveWord(CallEntry, RenamingInformation);
							}
							else
							{
								ResolveWord(CallEntry, RenamingInformation);

								if (Verbose >= VerboseLevel::Basic && CallEntry.Type != WordType::Code)
									std::cout << std::format("  INFO: found {} in {} that is really Code\n", CallEntry.Name, GetOverlayName(CallEntry.BaseAddress));

								CallEntry.Type = WordType::Code;
							}
						}


						ProcessCodeFound++;
						Insert(CallEntry);
					}
				}
			}
		} 
		else 
		{
			uint16_t OpcodeValue = 0;
			for (int x = 0; x < 5; x++)
			{
				auto Param = X86Disasm::GetParam(Opcode, x);

				if (Param.Base != X86Disasm::Register::None)
					continue;

				if (Param.ValueType == X86Disasm::ValueType::Address)
				{
					OpcodeValue = (uint16_t)Param.Value;
					break;
				}
				else if (Param.ValueType == X86Disasm::ValueType::Relative)
				{
					OpcodeValue = Address + (uint16_t)Param.Value - Opcode.Length;
					break;
				}
				else if (Param.ValueType == X86Disasm::ValueType::FarPointer)
				{
					OpcodeValue = (uint16_t)(Param.Value & 0xFFFF);
					break;
				}
			}

			if (OpcodeValue >= 0x100)
			{
				auto Match = FindWordFromAddress(OpcodeValue);

				// If second is false, we found a new word
				if (!Match.second && Match.first.IsValid())
				{
					ResolveWord(Match.first, RenamingInformation);

					ProcessCodeFound++;
					Insert(Match.first);

					if (Verbose >= VerboseLevel::Basic)
						std::cout << std::format("  INFO: found {} in {}\n", Match.first.Name, GetOverlayName(Match.first.BaseAddress));
				}

			}
		}

		if (Opcode.Length == 1)
		{
			if (Opcode.Opcode == 0xCF || Opcode.Opcode == 0xC3 || Opcode.Opcode == 0xCB)
			{
				// IRET, RET, RETF
				Done = true;
			}
		}

		if (Done && JumpAddress != 0xFFFF)
		{
			if (Verbose >= VerboseLevel::Basic)
				std::cout << std::format("  INFO: {} in {} JMPed past NEXT @ {:X} -> {:X}\n", Entry.Name, Files[CurrentOverlay].Name, JumpSetAddress, JumpAddress);

			Done = false;
		}

	} while (!Done);


	return Address - StartAt;
}

void ProcessCode(ForthWord& Entry, std::vector<NamingInformation>& RenamingInformation)
{
	if (Entry.Type == WordType::Code)
		Entry.DataLength = ProcessASM(Entry, RenamingInformation, Entry.DataAddress);
	else
		Entry.DataLength = ProcessASM(Entry, RenamingInformation, Entry.BaseAddress);
}

void ProcessWord(ForthWord& Entry, std::vector<NamingInformation>& RenamingInformation)
{
	uint16_t Address = Entry.DataAddress;
	ForthWord Next = Files[CurrentOverlay].Words.FindAfter(Entry.BaseAddress);
	uint16_t SavedOverlay = CurrentOverlay;

	uint16_t ForwardBranch = 0;
	
	uint16_t NextAddress = 0xFFFF;
	if (Next.IsValid())
	{
		if (Address == Next.BaseAddress && Entry.SubType != WordSubType::Alias)
		{
			//if (HasFlag(Next.Flags, WordFlags::Unproccessed) && Next.Type == WordType::Raw)
			//{
			//	// Looks like we put a gap here already, so just ignore it.
			//	Next = Files[CurrentOverlay].Words.FindAfter(Next.BaseAddress);
			//}
			//else
			//{
				// Zero length words are impossable. 
			std::cout << std::format("PANIC: Found a zero length word {} in {} ({})!!!\n", Entry.Name, Files[CurrentOverlay].Name, Next.Name);
				abort();
			//}
			

		}
		NextAddress = Next.BaseAddress;
	}


	while (Address < NextAddress)
	{
		if (Address == ForwardBranch)
			ForwardBranch = 0;

		uint16_t Token = ReadWord(Address);
		Address += 2;

		ForthWord CurrentWord = Find(Token);

		if (!CurrentWord.IsValid())
		{
			CurrentWord = ReadWordFromExecutionToken(Token);

			if (CurrentWord.IsValid())
			{
				if (CurrentOverlay == 0 && OverlayAddress(CurrentWord.BaseAddress))
				{
					if(Verbose >= VerboseLevel::All)
						std::cout << std::format("   WARN: Impossable word found in {}, probably junk data\n", Entry.Name);

					continue;
				}

				ResolveWord(CurrentWord, RenamingInformation);			

				if (Verbose >= VerboseLevel::Debug)
					std::cout << std::format(" INFO: Found new word {} while processing {}\n", CurrentWord.Name, Entry.Name);
				ProcessWordFound++;
				Insert(CurrentWord);
			}
		}

		if (HasFlag(CurrentWord.Flags, WordFlags::Branch))
		{
			int16_t NewBranch = ReadWord(Address);

			if (NewBranch > 0)
			{
				ForwardBranch = Address + NewBranch;
			}


			Address += 2;

		}		
		else if (HasFlag(CurrentWord.Flags, WordFlags::Lit))
		{
			// While Branch words are also lit words, it will be caught above.
			for (int x = 0; x < CurrentWord.ParamCount; x++)
			{
				// Read the values in the lit, just incase it's a Body pointer
				uint16_t Value = ReadWord(Address + (x * 2));

				if (Value > 0x100)
				{
					// ' returns the pointe to the body, so we have to subtract two to get an XT
					// It's valid, so move on
					if (Find(Value - 2).IsValid())
						continue;

					// Try to read the word.
					auto PossibleWord = ReadWordFromExecutionToken(Value - 2);
					if (PossibleWord.IsValid())
					{
						if (FindCodePointer(PossibleWord.CodePointer).IsValid() || PossibleWord.CodePointer == PossibleWord.DataAddress)
						{
							if (CurrentOverlay == 0 && OverlayAddress(PossibleWord.BaseAddress))
							{
								if (Verbose >= VerboseLevel::All)
									std::cout << std::format("   WARN: Impossable word found in {}, probably junk data\n", Entry.Name);
							}
							else
							{

								ResolveWord(PossibleWord, RenamingInformation);
								if (Verbose >= VerboseLevel::Debug)
									std::cout << std::format(" INFO: Found new word {} via Data Pointer, while processing {}\n", PossibleWord.Name, Entry.Name);

								ProcessWordFound++;
								Insert(PossibleWord);
							}
						}
					}
				}
			}

			Address += CurrentWord.ParamCount * 2;
		}
		
		if (HasFlag(CurrentWord.Flags, WordFlags::String))
		{
			uint8_t Len = ReadByte(Address);
			Address += 1 + Len;
		}

		if (CurrentWord.Type == WordType::Overlay)
		{
			if (CurrentOverlay != 0 && Verbose >= VerboseLevel::Debug)
				std::cout << std::format(" INFO: Huh? Overlay {} found in {}\n", CurrentWord.Name, Files[CurrentOverlay].Name);

			// Mimic loading in the overlay when an overlay word is found
			LoadOverlay(ReadWord(CurrentWord.DataAddress));
		}
		
		if (HasFlag(CurrentWord.Flags, WordFlags::Code))
		{
			// Look for the 'DOES>' pattern (;Code) JMP 1649
			if (ReadByte(Address) == 0xE8)
			{
				int16_t Dest = ReadWord(Address + 1);
				if ((uint16_t)(Address + Dest + 3) == DoesPtr)
				{
					// If we have a DOES>, lets make sure it's correctly registered as a code Field
					auto CodeField = FindCodePointer(Address);					
					if (CodeField.Name != Entry.Name && Entry.Name != "(CREATE)")
					{						
						ForthWord UpdateCodeField{ Entry.Name, WordType::Data, WordSubType::None, WordFlags::None };
						UpdateCodeField.BaseAddress = UpdateCodeField.CodePointer = UpdateCodeField.ExecutionToken = Address;

						ResolveWord(UpdateCodeField, RenamingInformation);

						// We found a valid entry, but it dosn't match, 
						if (CodeField.IsValid())
						{
							if (Verbose >= VerboseLevel::Debug)
								std::cout << std::format("  INFO: Bogus Code Pointer Entry {} in {}, really is {}\n", CodeField.Name, Files[CurrentOverlay].Name, UpdateCodeField.Name);

							if (OverlayAddress(UpdateCodeField.CodePointer))
							{
								Files[CurrentOverlay].CodeFields.Update(UpdateCodeField);
							}
							else
							{
								Files[0x0000].CodeFields.Update(UpdateCodeField);
							}

						}
						else
						{
							if (Verbose >= VerboseLevel::Basic)
								std::cout << std::format("  INFO: Code Pointer Entry {} in {}\n", UpdateCodeField.Name, Files[CurrentOverlay].Name);

							if (OverlayAddress(UpdateCodeField.CodePointer))
							{
								Files[CurrentOverlay].CodeFields.Insert(UpdateCodeField);
							}
							else
							{
								Files[0x0000].CodeFields.Insert(UpdateCodeField);
							}
						}
 					}

					Address += 3;
					if (ForwardBranch != 0 && Verbose >= VerboseLevel::Basic)
						std::cout << std::format("  INFO: {} in {} BRANCHed past DOES>\n", Entry.Name, Files[CurrentOverlay].Name);


				}
 			}
			else 
			{
				// It wasn't a DOES> so it's straight ASM
				Address += ProcessASM(Entry, RenamingInformation, Address);
				break;
			}
		}

		if (HasFlag(CurrentWord.Flags, WordFlags::Exit))
		{
			if (ForwardBranch == 0)
			{
				break;
			}
			else
			{
				if (Verbose >= VerboseLevel::All)
					std::cout << std::format("  INFO: {} in {} BRANCHed past EXIT\n", Entry.Name, Files[CurrentOverlay].Name);
			}
		}
	};

	if (ForwardBranch != 0)
	{
		std::cout << std::format("PANIC: Forward BRANCH Error in {}\n", Entry.Name);
		abort();
	}

	if (SavedOverlay != CurrentOverlay)
		LoadOverlay(SavedOverlay);

	if(Entry.DataLength == 0)
		Entry.DataLength = Address - Entry.DataAddress;
}

void ProcessForthWord(ForthWord &Entry, std::vector<NamingInformation>& RenamingInformation)
{
	if (HasFlag(Entry.Flags, WordFlags::User))
	{
		ProcessUserWord(Entry, RenamingInformation);
	} 
	else switch (Entry.Type)
	{
		case WordType::Bad:
		case WordType::Unknown:
			break;
	
		case WordType::Word:
			ProcessWord(Entry, RenamingInformation);
			break;

		case WordType::Data:
			ProcessKnowDataTypes(Entry, RenamingInformation);
			break;

		case WordType::RawCode:
		case WordType::Code:
			ProcessCode(Entry, RenamingInformation);
			break;

		case WordType::Overlay:
			Entry.DataLength = 2;
			break;
		
		case WordType::Raw:
			break;

		default:
			std::cout << std::format("ERROR: Unexpected Entry type {:X}\n", (int)Entry.Type);
			abort();
			break;
	}

	Entry.Flags &= ~(WordFlags::Unproccessed);
	Update(Entry);

}

void ProcessWords(std::vector<NamingInformation>& RenamingInformation)
{
	CurrentStage = DiscoveryStage::Words;

	ProcessWordFound = 0;
	ProcessDataFound = 0;
	ProcessCodeFound = 0;

	int InsertTotal = 0;

	int Total = 0;
	do
	{
		InsertCount = 0;
		Total = 0;
		for (auto &File : Files)
		{
			if (File.second.Type == FileType::Main || File.second.Type == FileType::Overlay || File.second.Type == FileType::Video)
			{
				LoadOverlay(File.first);

				// We need to make a deep copy of the word list because we will be adding to it
				// which will break the foreach loop below.   
				auto WordList = Files[CurrentOverlay].Words;

				for (auto &Entry : WordList)
				{
					if (HasFlag(Entry.Flags, WordFlags::Undefined))
					{
						if (Verbose >= VerboseLevel::Debug)
							std::cout << std::format(" WARN: {} is marked Undefined\n", Entry.Name);
					}
					else
					{
						if (HasFlag(Entry.Flags, WordFlags::Unproccessed))
						{
							if (Entry.Type == WordType::Unknown)
							{
								auto CodePointer = FindCodePointer(Entry.CodePointer);
								if (!CodePointer.IsValid() && File.second.Type != FileType::Video && Entry.Type != WordType::Code && Entry.Type != WordType::RawCode && Entry.Type != WordType::Raw)
								{
									CodePointer = CreateCodePointer(Entry, RenamingInformation);
								}

								if (!CodePointer.IsValid())
								{
									if (Verbose >= VerboseLevel::Debug)
										std::cout << std::format(" WARN: {} Has an invaid code pointer, bogus word?\n", Entry.Name);
								}

								if (Entry.Type == WordType::Unknown && CodePointer.Type != WordType::Invalid)
								{
									Entry.Type = CodePointer.Type;
									Entry.SubType = CodePointer.SubType;
									if (HasFlag(CodePointer.Flags, WordFlags::User))
									{
										Entry.Flags |= WordFlags::User;
									}
									Update(Entry);
								}
							}
							
							ProcessForthWord(Entry, RenamingInformation);

							Total++;
						}
					}
				}

				UnloadOverlay();
			}
		}

		if (Verbose >= VerboseLevel::All && (Total != 0 || InsertCount !=0 ))
			std::cout << std::format(" INFO: Found {} unprocesesed words, created {} new words.\n", Total, InsertCount);

		InsertTotal += InsertCount;

	} while (Total != 0);

	InsertCount = InsertTotal;

	if (Verbose >= VerboseLevel::Basic && ProcessWordFound != 0 && ProcessDataFound != 0 && ProcessCodeFound != 0)
		std::cout << std::format(" INFO: Found {} in Words, Found {} in Data, Found {} in code.\n", ProcessWordFound, ProcessDataFound, ProcessCodeFound);
}

ForthWord FindValidWord(uint16_t Value)
{
	auto Match = Find(Value);

	if (!Match.IsValid())
		Match = Find(Value - 2);

	if (Value > 0x100 && Match.IsValid())
		return Match;

	ForthWord Ret;
	Ret.Type = WordType::Invalid;
	return Ret;
}

void ProcessData(std::vector<NamingInformation>& RenamingInformation)
{
	CurrentStage = DiscoveryStage::Data;

	for (auto& File : Files)
	{
		if (File.second.Type == FileType::Main || File.second.Type == FileType::Overlay || File.second.Type == FileType::Video)
		{
			LoadOverlay(File.first);

			ForthWord LastWord;

			for (auto& Entry : Files[CurrentOverlay].Words)
			{
				// Simply just size the data for the moment
				if (LastWord.IsValid() && LastWord.DataLength == 0 && (LastWord.Type == WordType::Data || LastWord.Type == WordType::Raw))
				{
					LastWord.DataLength = Entry.BaseAddress - LastWord.DataAddress;

					if (LastWord.Type == WordType::Data && LastWord.SubType == WordSubType::None)
					{
						if (LastWord.DataLength == 2)
						{
							LastWord.SubType = WordSubType::Variable;
						} 
						
						if ((LastWord.DataLength & 0x02) == 0 && LastWord.SubType == WordSubType::None)
						{
							// If it's divisable by 4, and the first two values are a valid iaddrs, assume it's a list of iaddrs
							uint32_t Value1 = ReadDoubleWord(Entry.DataAddress);
							uint32_t Value2 = ReadDoubleWord(Entry.DataAddress + 4);

							if (ValidInstanceRecord(Value1) && ValidInstanceRecord(Value2))
							{
								LastWord.SubType = WordSubType::DoubleWordArray;
								if (Verbose >= VerboseLevel::Debug)
									std::cout << std::format(" INFO: Data {} is an Double Array\n", LastWord.Name);

							}
						}

						if ((LastWord.DataLength % 3) == 0 && LastWord.SubType == WordSubType::None)
						{
							// If it's divisable by 3, and the first two values are a valid iaddrs, assume it's a list of iaddrs
							uint32_t Value1 = Read3Byte(LastWord.DataAddress);
							uint32_t Value2 = Read3Byte(LastWord.DataAddress + 3);

							if (ValidInstanceRecord(Value1) && ValidInstanceRecord(Value2))
							{
								LastWord.SubType = WordSubType::IAddrArray;
								LastWord.Param[0] = 0;
								if (Verbose >= VerboseLevel::Debug)
									std::cout << std::format(" INFO: Data {} is an IADDR Array\n", LastWord.Name);
							}
						}
						
						if ((LastWord.DataLength & 0x01) == 0 && LastWord.SubType == WordSubType::None)
						{
							// If it's divisable by 2, and the first two values are a valid match, assume it's a list of words
							uint16_t Value = ReadWord(LastWord.DataAddress);
							uint16_t Value2 = ReadWord(LastWord.DataAddress + 2);

							if (FindValidWord(ReadWord(LastWord.DataAddress)).IsValid() && FindValidWord(ReadWord(LastWord.DataAddress + 2)).IsValid())
							{
								LastWord.SubType = WordSubType::WordArray;
								if (Verbose >= VerboseLevel::Debug)
									std::cout << std::format(" INFO: Data {} is an Word Array\n", LastWord.Name);
							}
						}
					}
					
					if (Verbose >= VerboseLevel::Debug)
						std::cout << std::format(" INFO: Sized data {} in {} to {}\n", LastWord.Name, GetOverlayName(LastWord.BaseAddress), LastWord.DataLength);

					Update(LastWord);
				}

				LastWord = Entry;
			}

			UnloadOverlay();
		}
	}

}


void ExamineMemoryLayout(std::vector<NamingInformation>& RenamingInformation)
{
	CurrentStage = DiscoveryStage::Memory;
	int StartInsertCount = InsertCount;
	
	for (auto &File : Files)
	{
		if (File.second.Type == FileType::Main || File.second.Type == FileType::Overlay || File.second.Type == FileType::Video)
		{
			LoadOverlay(File.first);

			std::vector<std::pair<uint16_t, uint16_t>> Gaps;
			ForthWord LastWord;

			for (auto& Entry : Files[CurrentOverlay].Words)
			{
					if (LastWord.IsValid())
					{
						uint16_t LastWordEnd = LastWord.DataAddress + LastWord.DataLength;

						// Check to see if the end of the last word lines up with the start of the current word.
						if (LastWordEnd != 0 && LastWordEnd != Entry.BaseAddress)
						{
							// So we have a discrepancy. 

							// The Last word was too long.
							if (LastWordEnd > Entry.BaseAddress)
							{
								if (LastWord.DataLength == 0)
								{
									// If a word without data is overlapping the next word, something is bogus, probably the last word
									if (Verbose >= VerboseLevel::Basic)
										std::cout << std::format("  INFO: Bogus Word Found {} in {}\n", LastWord.Name, Files[CurrentOverlay].Name);
								}								
								else if (Entry.Type == WordType::Unknown && HasFlag(Entry.Flags, WordFlags::Nameless))
								{
									if (Verbose >= VerboseLevel::Basic)
										std::cout << std::format("  INFO: Bogus Word Found {} in {}\n", Entry.Name, Files[CurrentOverlay].Name);

									Entry.Type = WordType::Bad;
									Update(Entry);
								}
								else if (LastWord.Type == WordType::Data || LastWord.Type == WordType::Raw) //if (Entry.Type == WordType::Code || Entry.Type == WordType::RawCode)
								{
									if (Verbose >= VerboseLevel::Basic)
										std::cout << std::format("  INFO: {} in {} size reduced from {} to {}.\n", LastWord.Name, Files[CurrentOverlay].Name, LastWord.DataLength, Entry.BaseAddress - LastWord.DataAddress);

									LastWord.DataLength = Entry.BaseAddress - LastWord.DataAddress;

									Update(LastWord);
								}
							}
							else
							{
								// There's a gap
								Gaps.push_back(std::make_pair(LastWordEnd, Entry.BaseAddress - LastWordEnd));
							}

						}
					}

					if (Entry.IsValid())
						LastWord = Entry;
				}

			for (auto [Start, Length] : Gaps)
			{
				bool GapFound = true;
				uint16_t Token = ReadWord(Start);
				auto Match = FindCodePointer(Token);

				// We found a code pointer, (or a code word) so lets try reading a word
				if (Match.IsValid() || Token == Start + 2)
				{
					auto CurrentWord = ReadWordFromExecutionToken(Start);

					// Looks like we have a new word.
					if (CurrentWord.IsValid())
					{
						auto Matched = Find(CurrentWord.ExecutionToken);

						if (CurrentOverlay == 0 && OverlayAddress(CurrentWord.BaseAddress))
						{									
							continue;
						}

						ResolveWord(CurrentWord, RenamingInformation);
							
						if (Verbose >= VerboseLevel::All)
							std::cout << std::format("   INFO: Unreferenced word found {} in {}\n", CurrentWord.Name, Files[CurrentOverlay].Name);

						if (CurrentWord.Type == WordType::Unknown)
						{
							auto CodePointer = FindCodePointer(CurrentWord.CodePointer);

							if (CodePointer.Type != WordType::Invalid)
							{
								CurrentWord.Type = CodePointer.Type;
								CurrentWord.SubType = CodePointer.SubType;
								if (HasFlag(CodePointer.Flags, WordFlags::User))
								{
									CurrentWord.Flags |= WordFlags::User;
								}
							}
						}

						Insert(CurrentWord);

						GapFound = false;
					}
				}

				if (GapFound)
				{
					ForthWord GapWord = { "", WordType::Raw, WordSubType::Gap, WordFlags::Nameless, Start };
					ResolveWord(GapWord, RenamingInformation);
					GapWord.Stage = DiscoveryStage::MemoryGap;
					GapWord.DataLength = Length;

					if (Verbose >= VerboseLevel::All)
						std::cout << std::format("   INFO: Gap {} Found @{:04X} - @{:04X}\n", GapWord.Name, Start, Start + Length);

					Insert(GapWord);
				}

			}

			UnloadOverlay();
		}		
	}

	if (Verbose >= VerboseLevel::Basic && InsertCount != StartInsertCount)
		std::cout << std::format(" INFO: Found {} unrefenced words and gaps.\n", InsertCount - StartInsertCount);
}



void ProcessNamingInformation(std::vector<NamingInformation>& RenamingInformation)
{
	CurrentStage = DiscoveryStage::External;
	
	for (auto Entry : RenamingInformation)
	{
		if (HasFlag(Entry.Action, NamingAction::Define))
		{
			if (Entry.OverlayName != "")
			{
				int OverlayID = LookupOverlay(Entry.OverlayName);
				if (OverlayID == -1)
				{
					std::cout << std::format("ERROR: Unknown overlay {} in rename rules\n", Entry.OverlayName);
					continue;
				}

				LoadOverlay(OverlayID);
			}
			else
			{
				LoadOverlay(0x0000);
			}

			ForthWord NewWord;

			if (Entry.Type == WordType::Word)
			{
				NewWord = ReadWordFromExecutionToken(Entry.CodeFieldPtr - 2);
				if(!NewWord.IsValid())
					NewWord = ReadWordFromExecutionToken(Entry.CodeFieldPtr);
				
				if(Entry.Name != "")
					NewWord.Name = Entry.Name;
				NewWord.Type = Entry.Type;
				
				
				if (NewWord.Name.size() == 0)
					NewWord.Flags |= WordFlags::Nameless;
				else
					NewWord.Flags &= ~WordFlags::Nameless;

			}
			else
			{
				NewWord = ReadWordFromExecutionToken(Entry.CodeFieldPtr - 2);
				if (!NewWord.IsValid())
					NewWord = ReadWordFromExecutionToken(Entry.CodeFieldPtr);

				if (Entry.Name != "")
					NewWord.Name = Entry.Name;
				NewWord.Type = Entry.Type;
				NewWord.SubType = Entry.SubType;
				NewWord.Param[0] = Entry.TypeParam[0];
				NewWord.Param[1] = Entry.TypeParam[1];

				if (NewWord.Name.size() == 0)
					NewWord.Flags |= WordFlags::Nameless;
				else
					NewWord.Flags &= ~WordFlags::Nameless;

				if (NewWord.Type == WordType::RawCode || NewWord.Type == WordType::Raw)
					NewWord.BaseAddress = Entry.CodeFieldPtr;
				else
					NewWord.ExecutionToken = Entry.CodeFieldPtr;
			}

			if (Entry.Size != 0)
				NewWord.DataLength = Entry.Size;

			ResolveWord(NewWord, RenamingInformation);
			Insert(NewWord);

			if (Verbose >= VerboseLevel::Basic)
				std::cout << std::format(" INFO: Defining word {}\n", NewWord.Name);
			
		}
	}
	UnloadOverlay();

	if (Verbose >= VerboseLevel::Basic)
		std::cout << std::format(" INFO: ProcessNamingInformation: {} Words defined.\n", InsertCount);
}


bool ProcessStarFlight(std::vector<NamingInformation>& NamingInformation, VerboseLevel verbose)
{
	Verbose = verbose;
	CurrentStage = DiscoveryStage::Internal;

	ForthWord StartEntry = ReadWordFromExecutionToken(StartPtr);
	StartEntry.Name = "_start";
	StartEntry.Flags &= ~WordFlags::Nameless;
	ResolveWord(StartEntry, NamingInformation);
	Insert(StartEntry);
	

	std::cout << std::format("INFO: Processing Instance Data\n");
	auto InstanceFile = FindFile("INSTANCE");
	InitilizeInstanceData(InstanceFile);
	ProcessInstanceData();


	std::cout << std::format("INFO: Creating Externally Defined Words\n");
	ProcessNamingInformation(NamingInformation);

	// Process all the Dictionaries in the main file, overlays and video files
	std::cout << std::format("INFO: Processing Dictionaries\n");
	ProcessDictionaries(NamingInformation);

	// Process all the words to build the code pointers 
	std::cout << std::format("INFO: Processing Code Pointers\n");
	ProcessCodePointers(NamingInformation);
	
	do
	{
		InsertCount = 0;

		// Process all the words to find any words not in the Dictionary, and 
		std::cout << std::format("INFO: Processing Words\n");
		ProcessWords(NamingInformation);

		// Process all the words to find any words not in the Dictionary, and 
		std::cout << std::format("INFO: Processing Data\n");
		ProcessData(NamingInformation);

		//// Check if there anything in the gaps that could be unnamed code
		std::cout << std::format("INFO: Examining Memory\n");
		ExamineMemoryLayout(NamingInformation);
		
	} while (InsertCount != 0);

	Verbose = VerboseLevel::None;
	return true;
}

bool DumpStarFlight(const std::filesystem::path& OutputPath, VerboseLevel verbose)
{
	Verbose = verbose;

	// Create the output locations
	std::filesystem::create_directories(OutputPath);
	std::filesystem::create_directories(OutputPath / "hex");
	std::filesystem::create_directories(OutputPath / "img");
	std::filesystem::create_directories(OutputPath / "instance");
	std::filesystem::create_directories(OutputPath / "meta");
	
	DumpInstanceData(OutputPath);
	DumpUniverseData(OutputPath);
	//DumpClassDataTree(OutputPath, "INACTIVE");
	DumpClassDataTree(OutputPath, "STARPORT");
	DumpBoxDataTree(OutputPath, "DEBRIS");
	DumpBoxDataTree(OutputPath, "COMMUNICATIONS");

	DumpKnownFields(OutputPath / "meta", "FIELDS");
	DumpRenames(OutputPath / "meta", "RENAMES");

	LoadOverlay(0);

	for (auto File : Files)
	{
		const std::string BadFile = "<>:\"/\\|?*";

		auto name = File.second.Name;
		for (size_t x = 0; x < name.length(); x++)
		{
			if (name[x] < ' ' || BadFile.find(name[x]) != -1)
				name[x] = '_';
		}

		if (File.second.Name == "DIRECTORY")
		{
			DumpDirectory(OutputPath, name, File.second);
		}
		else if (File.second.Name == "BOOT/KERNEL" || File.second.Name == "INSTALL" || 
				  File.second.Name == "INSTANCE" || File.second.Name == "SAVE")
		{
			// BOOT/KERNEL and Install don't seemed to be used.
			// Instance is dumped elsewhere and SAVE is saving stacks and buffers, and dosn't really have a format.
		}
		else if (File.second.Name == "CREATURE")
		{
			// CREATURES seems to be fully populated by the game on demand
			DumpGeneric(OutputPath, name, File.second);
		}
		else if (File.second.Name == "FACET" || File.second.Name == "GPOLY" || File.second.Name == "VERTEX")
		{
			// FACET, GPOLY and VERTEX appear to be used in generating the way planets look in orbit and during landing 
			DumpGeneric(OutputPath, name, File.second);
		}
		else if (File.second.Name == "SPLASH" || File.second.Name == "CREDITS")
		{
			DumpFullScreenImage(OutputPath / "img", name, File.second);
		}
		else if (File.second.Name == "PHAZES")
		{
			DumpPhazes(OutputPath, name, File.second);
		}
		else if (File.second.Name == "GALAXY")
		{
			DumpGalaxy(OutputPath, name, File.second);
		}
		else if (File.second.Name == "HUM-PIC" || File.second.Name == "VEL-PIC" || File.second.Name == "THR-PIC" ||
			     File.second.Name == "ELO-PIC" || File.second.Name == "AND-PIC")
		{
			DumpCrewPic(OutputPath / "img", name, File.second);
		}
		else if (File.second.Name == "MED-PIC" || File.second.Name.find("-CPIC") != -1 || File.second.Name.find("PORT-PIC") != -1)
		{
			DumpLayerPic(OutputPath / "img", name, File.second);
		}
		else if (File.second.Name == "ICON1:1" || File.second.Name == "ICON1:2" || File.second.Name == "ICON1:4")
		{
			DumpIcons(OutputPath / "img", name, File.second);
		}
		else if (File.second.Name == "LSYSICON" || File.second.Name == "MSYSICON" || File.second.Name == "SSYSICON")
		{
			DumpSystemIcon(OutputPath / "img", name, File.second);
		}
		else if (File.second.Name == "VESSEL")
		{
			DumpVessel(OutputPath, name, File.second);
		}
		else if (File.second.Name == "VES-BLT")
		{
			DumpVesBlt(OutputPath, name, File.second);
		}
		else if (File.second.Name == "ARTIFACT")
		{
			DumpArtifact(OutputPath, name, File.second);
		}
		else if (File.second.Name == "ANALYZE-TEXT")
		{
			DumpAnalyzeText(OutputPath, name, File.second);
		}
		else if (File.second.Name == "BUTTONS")
		{
			DumpButtons(OutputPath, name, File.second);
		}
		else if (File.second.Name == "FONTS")
		{
			DumpFonts(OutputPath, name, File.second);
		}
		else if (File.second.Name == "REGIONS")
		{
			DumpRegions(OutputPath, name, File.second);
		}
		else if (File.second.Name == "PLANET")
		{
			DumpPlanetData(OutputPath, name, File.second);
		}
		else if (File.second.Name == "BOX" || File.second.Name == "ICON-NAME" || File.second.Name == "SPECIMEN")
		{
			DumpStringData(OutputPath, name, File.second);
		}
		else if (File.second.Name == "BIO-DATA")
		{
			DumpStringData(OutputPath, name, File.second, '.');
		}
		else if (File.second.Name == "BANK-TRANS")
		{
			DumpStringData(OutputPath, name, File.second, ' ', true);
		}
		else if (File.second.Name == "CMAP")
		{
			DumpColorMap(OutputPath, name, File.second);
		}
		else if (File.second.Name == "EARTH")
		{
			DumpEarth(OutputPath, name, File.second);
		}
		else if (File.second.Name == "ELEMENT")
		{
			DumpElements(OutputPath, name, File.second);
		}
		else if (File.second.Name == "COMPOUNDS")
		{
			DumpCompounds(OutputPath, name, File.second);
		}
		else if (File.second.Name == "CREWMEMBER")
		{
			DumpCrewmember(OutputPath, name, File.second);
		}
		else if (File.second.Name == "PSTATS")
		{
			DumpPStats(OutputPath, name, File.second);
		}
		else if (File.second.Type == FileType::Main || File.second.Type == FileType::Overlay || File.second.Type == FileType::Video)
		{
			DumpWordlist(OutputPath, name, File.second);
		}
		else if(/*File.second.Disk != 0 && */File.second.RecordLength != 0)
		{
			// Generic dump of any not otherwise dumped file 
			DumpGeneric(OutputPath / "meta", name + "_gen", File.second);
		}

		if(File.second.Length != 0)
		{
			if (File.first == 0)
			{
				DumpMain(OutputPath / "hex", name, File.second);
			}
			else
			{
				DumpOverlay(OutputPath / "hex", name, File.second);
			}
		}
	}

	Verbose = VerboseLevel::None;
	return true;
}