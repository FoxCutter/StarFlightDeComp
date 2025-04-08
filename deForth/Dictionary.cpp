#include "Dictionary.h"
#include <algorithm>
#include <format>
#include <iostream>


static bool CodePointerSort(const ForthWord& lhs, const ForthWord& rhs)
{
	return lhs.ExecutionToken < rhs.ExecutionToken;
};

void Dictionary::Insert(ForthWord NewEntry)
{
	auto inst = std::lower_bound(DataSet.begin(), DataSet.end(), NewEntry, CodePointerSort);
	if (inst != DataSet.end() && inst->ExecutionToken == NewEntry.ExecutionToken)
	{
		std::cout << std::format("ERROR: Inserting an entry into the Dictionary that already exists {} -> {}.\n", inst->Name, NewEntry.Name);
		abort();
	}

	DataSet.insert(inst, NewEntry);
}

void Dictionary::Update(ForthWord NewEntry)
{
	auto inst = std::lower_bound(DataSet.begin(), DataSet.end(), NewEntry, CodePointerSort);
	if (inst == DataSet.end() || inst->ExecutionToken != NewEntry.ExecutionToken)
	{
		std::cout << std::format("ERROR: Unable to updated none existing item\n");
		abort();
	}

	*inst = NewEntry;
}

void Dictionary::Sort()
{
	std::sort(DataSet.begin(), DataSet.end(), CodePointerSort);
}

std::vector<ForthWord>::iterator Dictionary::Find(uint16_t CodeFieldPtr)
{
	auto value = std::find_if(DataSet.begin(), DataSet.end(), [CodeFieldPtr](const ForthWord& obj) {
		return obj.ExecutionToken == CodeFieldPtr;
		});

	return value;
}

ForthWord Dictionary::FindWord(uint16_t CodeFieldPtr)
{
	auto value = Find(CodeFieldPtr);

	if (value == DataSet.end())
	{
		ForthWord Ret;
		Ret.Type = WordType::Invalid;
		return Ret;
	}
	else
	{
		return *value;
	}
}

ForthWord Dictionary::FindName(const std::string Name)
{
	auto Match = std::find_if(DataSet.begin(), DataSet.end(), [Name](const ForthWord& obj)
	{
		return obj.Name == Name;
	});

	if (Match == DataSet.end())
	{
		ForthWord Ret;
		Ret.Type = WordType::Invalid;
		return Ret;
	}
	else
	{
		return *Match;
	}
}

// Find the first dictonary entry before the current address 
ForthWord Dictionary::FindBefore(uint16_t Address)
{
	auto Match = std::find_if(DataSet.begin(), DataSet.end(), [Address](const ForthWord& obj)
	{
		return obj.BaseAddress >= Address;
	});

	if (Match == DataSet.end())
	{
		ForthWord Ret;
		Ret.Type = WordType::Invalid;
		return Ret;
	}
	else if (Match == DataSet.begin())
	{
		return *Match;
	}

	while (Match->BaseAddress >= Address)
	{
		Match--;
		if (Match == DataSet.begin())
			break;
	}

	return *Match;
}

ForthWord Dictionary::FindAfter(uint16_t Address)
{
	auto Match = std::find_if(DataSet.begin(), DataSet.end(), [Address](const ForthWord& obj)
		{
			return obj.BaseAddress > Address;
		});

	if (Match == DataSet.end())
	{
		ForthWord Ret;
		Ret.Type = WordType::Invalid;
		return Ret;
	}
	else
	{
		return *Match;
	}
}


//DictionaryEntry Dictionary::NextUndefined()
//{
//	auto FindUndefined = [](const DictionaryEntry& obj)
//	{
//		return obj.FlagSet(DictionaryFlags::Undefined);
//	};
//
//	auto Match = std::find_if(DataSet.begin(), DataSet.end(), FindUndefined);
//
//	if (Match == DataSet.end())
//	{
//		DictionaryEntry Ret;
//		Ret.Type = DictionaryType::Invalid;
//		return Ret;
//	}
//	else
//	{
//		return *Match;
//	}
//}
//
//int Dictionary::CountNames(std::string& Name)
//{
//	auto Count = std::count_if(DataSet.begin(), DataSet.end(), [Name](const DictionaryEntry& obj)
//	{
//		return obj.Name == Name;
//	});
//
//	return Count;
//}