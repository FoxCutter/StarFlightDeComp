#pragma once
#include <stdint.h>
#include "String.h"
#include <vector>
#include "Enum.h"

enum class WordType
{
	Unknown,		// We have no idea what the type is yet

	Word,			// General compiled Word
	Data,			// General Data
	Code,			// CODE word, followed by ASM code
	RawCode,		// Pure ASM, no headers or CodeFields provided
	Raw,			// Pointer to pure data, no headers or CodeFields provided

	Overlay,		// Overlay Word (Not a subtype as it needs speical handling when encontered)

	Bad,			// Probably not a real dictionary entry
	Invalid,		// Not really a word
};

// Sub types are for data words that way have extra work needed to process them, or to dump them
enum class WordSubType
{
	None,
	// All of these are just 'value codeponter name' the only different is the size of the data
	ByteValue,			 // The data type is for a single byte	(color_c, dir_field, combat_field)
	WordValue,			 // The data type is for a word value	(Constant, SIGFLD)
	DoubleWordValue,	 // The data type is for a double word 	(2C= ) (Stored High Word, Low Word, and not as a DWORD)
	Variable,			 // for word data with just the Create codepointer
	TwoWordValue,		 // The data type is for two words	(_?check)
	TwoDoubleWordValue,	 // Two doublesWords (one_or_many)


	// Arrays of variouse sizes, does not start with the array size
	ByteArray,		// An array of all bytes, dumped as HEX
	WordArray,		// An array of words (TABLE, word_arry)
	DoubleWordArray,	// An Array of Doubles
	IAddrArray,		// An array of Three byte Iaddrs, with an inital count, Param[0] = True if it has a count


	// Meta data types
	OverlayHeader,
	OverlayJunk,
	PointerTable,
	Gap,

	// Fields
	AField,
	IField,

	// Speical types
	Vocabulary,
	Syn,			

	// Complicated Types
	Case,
	CaseEx,				// CASE:
	Expert,
	ProbabilityArray,

	// Extra data
	BLT,

	// Word Subtypes
	Alias,			

};

enum class WordFlags : uint32_t
{
	None				= 0x00,
	Smudge				= 0x20,
	Immediate			= 0x40,
	Name				= 0x80,

	Truncated			= 0x00080000,	// A Truncated word

	Branch				= 0x00400000,	// A branching word
	Code				= 0x00800000,	// A ;Code Field
	String				= 0x01000000,	// Following by a string 
	Exit				= 0x02000000,	// Exits a function
	Lit					= 0x04000000,	// The next value is a literal
	Compile				= 0x08000000,   // Next word is a lit that will be compiled 

	Unproccessed		= 0x10000000,	// This word still needs to be processed
	Nameless			= 0x20000000,	// Nameless Function
	User				= 0x40000000,	// Indirect link to use data (Not a subtype as any data type can be a user word)
	Undefined			= 0x80000000,	// We are aware of this word, but it dosn't actually 'exist' yet
};

enum class DiscoveryStage
{
	None,
	Internal,
	External,
	Dictionaries,
	CodePointers,
	Words,
	Data,
	Memory,
	MemoryGap,
};

struct ForthWord
{
	std::string Name;
	WordType Type = WordType::Unknown;
	WordSubType SubType = WordSubType::None;
	WordFlags Flags = WordFlags::None;

	// Address of the start of the word	
	uint16_t BaseAddress = 0;

	// Address of the Code Pointer, This is what's stored in a word list
	uint16_t ExecutionToken = 0;

	// The Value of the Code Ponter
	uint16_t CodePointer = 0;

	// Address of the data 
	uint16_t DataAddress = 0;

	// The length of the data associtaed with the word
	uint16_t DataLength = 0;

	// The link Pointer of this word
	uint16_t LinkPtr = 0;

	// The Number of cells follwing the word
	uint16_t ParamCount = 0;

	// what stage of processing this was discovered
	DiscoveryStage Stage = DiscoveryStage::None;

	// Used for BLT Height, Width
	uint16_t Param[2] = { 0, 0 };

	bool IsValid() const
	{
		return Type != WordType::Invalid;
	}

};

//#include <initializer_list>

class Dictionary
{
private:
	std::vector<ForthWord> DataSet;

public:
	Dictionary() = default;
	//Dictionary(std::initializer_list<ForthWord> List) : DataSet(List)
	//{
	//	Sort();
	//};

	void Insert(ForthWord NewEntry);
	void Update(ForthWord NewEntry);

	void Sort();

	std::vector<ForthWord>::iterator Find(uint16_t CodeFieldPtr);
	ForthWord FindWord(uint16_t CodeFieldPtr);
	ForthWord FindName(const std::string Name);


	ForthWord FindBefore(uint16_t Address);
	ForthWord FindAfter(uint16_t Address);


	//ForthWord NextUndefined();

	//int CountNames(std::string& Name);

	auto begin()
	{
		return DataSet.begin();
	}

	auto end()
	{
		return DataSet.end();
	}
};