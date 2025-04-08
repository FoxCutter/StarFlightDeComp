#pragma once

#include <string>
#include <vector>

std::string TrimWhitespaces(const std::string& Input);
std::string TrimString(std::string Value, char Trim = ' ');
std::vector<std::string> SplitString(const std::string& Input, char Delim);
