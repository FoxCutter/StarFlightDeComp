#pragma once
#include <fstream>
#include "Dictionary.h"

namespace X86Disasm
{
	struct ResultData;
}

void DumpOpcode(std::ofstream& Output, uint16_t Address, X86Disasm::ResultData& Opcode, bool Video, std::string Remark = "");
bool IsJump(X86Disasm::ResultData& Opcode);
bool IsCall(X86Disasm::ResultData& Opcode);
bool HasFarAddress(X86Disasm::ResultData& Opcode);

void RehydrateCode(std::ofstream& Output, ForthWord& Entry, bool Video);
void RehydrateWord(std::ofstream& Output, ForthWord& Entry);