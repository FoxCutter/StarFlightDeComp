#include "String.h"
#include <format>
#include <algorithm>
#include <filesystem>

#include <list>
#include <vector>

#include <iostream>
#include <fstream>

#include <cctype>      // std::tolower
#include <algorithm>   // std::equal
#include <string_view> // std::string_view


#include "starflight.h"

bool ichar_equals(char a, char b)
{
	return std::tolower(static_cast<unsigned char>(a)) ==
		   std::tolower(static_cast<unsigned char>(b));
}

bool iequals(std::string_view lhs, std::string_view rhs)
{
	return std::ranges::equal(lhs, rhs, ichar_equals);
}


WordType ToWordType(std::string& Type)
{
	if (iequals(Type, "Raw"))
		return WordType::Raw;

	else if (iequals(Type, "RawCode"))
		return WordType::RawCode;

	else if (iequals(Type, "Word"))
		return  WordType::Word;

	else if (iequals(Type, "Data"))
		return WordType::Data;

	return WordType::Unknown;
}

WordSubType ToWordSubType(std::string& Type)
{
	if (Type == "")
		return WordSubType::None;
	
	else if (iequals(Type, "Byte"))
		return WordSubType::ByteValue;
	
	else if (iequals(Type, "Word"))
		return WordSubType::WordValue;

	else if (iequals(Type, "DoubleWord"))
		return WordSubType::DoubleWordValue;

	else if (iequals(Type, "2Word"))
		return WordSubType::TwoWordValue;

	else if (iequals(Type, "2DoubleWord"))
		return WordSubType::TwoWordValue;

	else if (iequals(Type, "ByteArray"))
		return WordSubType::ByteArray;

	else if (iequals(Type, "WordArray"))
		return WordSubType::WordArray;

	else if (iequals(Type, "DoubleWordArray"))
		return WordSubType::DoubleWordArray;

	else if (iequals(Type, "IAddrArray"))
		return WordSubType::IAddrArray;

	else if (iequals(Type, "Case"))
		return WordSubType::Case;

	else if (iequals(Type, "CaseEx"))
		return WordSubType::CaseEx;

	else if (iequals(Type, "Expert"))
		return WordSubType::Expert;

	else if (iequals(Type, "ProbabilityArray"))
		return WordSubType::ProbabilityArray;

	else if (iequals(Type, "BLT"))
		return WordSubType::BLT;

	std::cerr << std::format("Unknown subtype: {}", Type);
	
	return WordSubType::None;
}

std::vector<NamingInformation> ReadRenameData(const std::filesystem::path& renameData)
{
	std::vector<NamingInformation> results;

	if (renameData.empty())
	{
		return results;
	}

	std::ifstream input(renameData);
	if (!input.is_open())
	{
		std::cerr << std::format("Unable to read rename file {}\n", renameData.string());
		return results;
	}


	std::string InputLine;
	while (std::getline(input, InputLine))
	{
		InputLine = TrimWhitespaces(InputLine);

		if (InputLine.length() != 0)
		{
			if (InputLine[0] == '#')
				continue;
			
			if (InputLine.length() >= 2 && InputLine[0] == '/' && InputLine[1] == '/')
				continue;

			std::vector<std::string> Values = SplitString(InputLine, ',');
			if (Values.size() < 6)
			{
				std::cout << std::format("To few arguments in {}\n", InputLine);
				continue;
			}
			if (Values.size() > 6)
			{
				std::cout << std::format("To many arguments in {} (Did you use , when you meant to use : ?)\n", InputLine);
				continue;
			}

			NamingInformation NewName;

			if (Values[2] != "")
			{
				std::vector<std::string> FlagValues = SplitString(Values[2], '|');

				for (auto &Value : FlagValues)
				{
					if (iequals(Value, "Define"))
						NewName.Action |= NamingAction::Define;

					else if (iequals(Value, "Rename"))
						NewName.Action |= NamingAction::Rename;

					else if (iequals(Value, "Resize"))
						NewName.Action |= NamingAction::Resize;

					else if (iequals(Value, "Retype"))
						NewName.Action |= NamingAction::Retype;

					else
					{
						std::cout << std::format("Unknown action: {}\n", InputLine);
						NewName.Action = NamingAction::None;
						break;
					}
				}
			}

			if (Values[0] != "")
			{
				NewName.OverlayName = Values[0];
			}
			
			if (Values[1] != "")
			{
				if (Values[1]._Starts_with("0x"))
				{
					NewName.CodeFieldPtr = std::stoi(Values[1], nullptr, 16);
				}
				else
				{
					NewName.CodeFieldPtr = 0xFFFF;
					NewName.MatchName = Values[1];
				}
			}
			
			if (Values[3] != "")
			{
				NewName.Name = Values[3];
			}

			if (Values[4] != "")
			{
				auto TypeInfo = SplitString(Values[4], ':');
				while (TypeInfo.size() < 4)
					TypeInfo.emplace_back("");

				
				NewName.Type = ToWordType(TypeInfo[0]);
				NewName.SubType = ToWordSubType(TypeInfo[1]);
				if (TypeInfo[2] != "")
				{
					if (iequals(TypeInfo[2], "true"))
						NewName.TypeParam[0] = 0xFFFF;
					else if (iequals(TypeInfo[2], "false"))
						NewName.TypeParam[0] = 0;
					else
						NewName.TypeParam[0] = std::stoi(TypeInfo[2], nullptr, 10);
				}
				if (TypeInfo[3] != "")
				{
					if (iequals(TypeInfo[3], "true"))
						NewName.TypeParam[1] = 0xFFFF;
					else if (iequals(TypeInfo[3], "false"))
						NewName.TypeParam[1] = 0;
					else
						NewName.TypeParam[1] = std::stoi(TypeInfo[3], nullptr, 10);
				}

				if (NewName.Type == WordType::Unknown)
				{
					std::cout << std::format("Unknown Type: {}\n", InputLine);
					NewName.Action = NamingAction::None;
					break;
				}
			}

			if (Values[5] != "")
			{
				NewName.Size = std::stoi(Values[5], nullptr, 10);
			}

			if (HasFlag(NewName.Action, NamingAction::Define))
			{
				//if (NewName.Name == "")
				//{
				//	std::cout << std::format("Define: Missing name in {}\n", InputLine);
				//	NewName.Action = NamingAction::None;
				//}

				if (NewName.Type == WordType::Unknown)
				{
					std::cout << std::format("Define: Missing type in {}\n", InputLine);
					NewName.Action = NamingAction::None;
				}
			}

			if (HasFlag(NewName.Action, NamingAction::Rename))
			{
				if (NewName.Name == "")
				{
					std::cout << std::format("Rename: Missing name in {}\n", InputLine);
					NewName.Action = NamingAction::None;
				}
			}

			if (HasFlag(NewName.Action, NamingAction::Resize))
			{
				if (NewName.Size == 0)
				{
					std::cout << std::format("Resize: Missing size {}\n", InputLine);
					NewName.Action = NamingAction::None;
				}
			}

			if (HasFlag(NewName.Action, NamingAction::Retype))
			{
				if (NewName.Type == WordType::Unknown)
				{
					std::cout << std::format("Resize: Missing type {}\n", InputLine);
					NewName.Action = NamingAction::None;
				}
			}

			if (NewName.Action != NamingAction::None)
			{
				results.emplace_back(NewName);
			}
		}
	}

	return results;
}

VerboseLevel verbose = VerboseLevel::None;
int Version = 1;
std::filesystem::path BasePath;
std::filesystem::path OutputPath;
std::filesystem::path RenameData;
bool ShowUsage = false;

void Usage()
{
	std::cout << "usage: DEFORTH input_path [options]" << "\n";
	std::cout << "\n";
	std::cout << "  input_path: The path to the starfilght data files" << "\n";
	std::cout << "\n";
	std::cout << "The following options are available:" << "\n";
	std::cout << "  -o, --out       out_path       Path to write the output files (default: current directory)" << "\n";
	std::cout << "  -v, --version   version        Version of Starfilght to process (default: 1)" << "\n";
	std::cout << "  -r, --rename    rename_file    File to use for renameing fileds (default: .\\rename[version].csv)" << "\n";
	std::cout << "  -verbose[:level]               Verbose mode level. Leve can be: None, Basic (default), All, Debug" << "\n";
	std::cout << "\n";
	std::cout << " example: DEFORTH .\\PLAY -o .\\output -r rename1.csv -v 1 -verbose" << "\n";
	std::cout << "\n";
}

bool ProcessCommandLine(int argc, char* argv[])
{
	for (int x = 1; x < argc; x++)
	{
		std::string Value(argv[x]);
		if (iequals(Value, "--help") || iequals(Value, "-h"))
		{
			ShowUsage = true;
			return true;
		}
		else if (iequals(Value, "--out") || iequals(Value, "-o"))
		{
			x++;
			if (x == argc)
			{
				std::cerr << "Missing command line argument\n";
				return false;
			}

			OutputPath = argv[x];
		}
		else if (iequals(Value, "--rename") || iequals(Value, "-r"))
		{
			x++;
			if (x == argc)
			{
				std::cerr << "Missing command line argument\n";
				return false;
			}

			RenameData = argv[x];
		}
		else if (iequals(Value, "--version") || iequals(Value, "-v"))
		{
			x++;
			if (x == argc)
			{
				std::cerr << "Missing command line argument\n";
				return false;
			}

			Version = std::stoi(argv[x]);
		}
		else if (iequals(Value, "--verbose"))
		{
			verbose = VerboseLevel::Basic;
		}
		else if (iequals(Value, "--verbose:none"))
		{
			verbose = VerboseLevel::None;
		}
		else if (iequals(Value, "--verbose:basic"))
		{
			verbose = VerboseLevel::Basic;
		}
		else if (iequals(Value, "--verbose:all"))
		{
			verbose = VerboseLevel::All;
		}
		else if (iequals(Value, "--verbose:debug"))
		{
			verbose = VerboseLevel::Debug;
		}
		else if (Value.starts_with("-"))
		{
			std::cerr << std::format("Unknown flag: {}\n", Value);
			return false;
		}
		else
		{
			if (BasePath.empty())
			{
				BasePath = Value;			
			}
			else
			{
				std::cerr << std::format("Unknown flag: {}\n", Value);
				return false;
			}
		}
	}

	return true;
}



int main(int argc, char* argv[])
{
	if (!ProcessCommandLine(argc, argv))
		return -1;

	if (ShowUsage || argc == 1)
	{
		Usage();
		return 0;
	}

	if (OutputPath.empty())
		OutputPath = "./";

	if (RenameData.empty())
		RenameData = std::format("./rename{}.csv", Version);

	if (!std::filesystem::is_directory(BasePath))
	{
		std::cerr << std::format("ERROR: Argument {} is not a valid path\n", BasePath.string());
		return -1;
	}

	BasePath.make_preferred();
	OutputPath.make_preferred();
	RenameData.make_preferred();

	std::cout << std::format("Starflight {}\nInput path: {}\nOutput path: {}\nRename file: {}\n", Version, BasePath.string(), OutputPath.string(), RenameData.string());

	
	auto NameData = ReadRenameData(RenameData);

	LoadStarFlight1(BasePath, verbose);
	ProcessStarFlight(NameData, verbose);

	DumpStarFlight(OutputPath, verbose);

	return 0;

}