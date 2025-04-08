#include "String.h"


const char* WhiteSpace = " \t\v\r\n";

std::string TrimWhitespaces(const std::string& Input)
{
	auto Start = Input.find_first_not_of(WhiteSpace);
	auto End = Input.find_last_not_of(WhiteSpace);
	
	// If we didn't find any none whitespace charcaters, just return blank string
	if (Start == std::string::npos)
		return "";
	
	return Input.substr(Start, End - Start + 1);
}

std::string TrimString(std::string Value, char Trim)
{
	auto End = Value.find_last_not_of(Trim);
	return Value.substr(0, End + 1);
}


std::vector<std::string> SplitString(const std::string& Input, char Delim)
{
	std::vector<std::string> Results;

	int StartPos = 0;
	for (int x = 0; x < Input.length(); x++)
	{
		if (Input[x] == Delim)
		{
			std::string Val = Input.substr(StartPos, x - StartPos);
			StartPos = x + 1;

			Results.emplace_back(TrimWhitespaces(Val));
		}
	}

	std::string Val = Input.substr(StartPos);
	Results.emplace_back(TrimWhitespaces(Val));

	return Results;
}