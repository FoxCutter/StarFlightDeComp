#include <format>
#include <algorithm>
#include <iostream>

#include "Rehydrate.h"
#include "Files.h"
#include "Instance.h"

#include "X86Disasm/Disassembler.h"
#include "X86Disasm/Results.h"

extern uint16_t UserBase;

bool IsJump(X86Disasm::ResultData& Opcode)
{
	// All Conditional branchs are jumps
	if (HasFlag(Opcode.OpcodeData->Flags, X86Disasm::OpcodeFlags::Conditional))
		return true;

	if (!HasFlag(Opcode.OpcodeData->Flags, X86Disasm::OpcodeFlags::Branch))
		return false;

	uint8_t OpcodeValue = Opcode.Opcode;

	if (OpcodeValue == 0xE9 ||  // 0xE9 Jump Opword Offset
		OpcodeValue == 0xEA ||  // 0xEA Far Jump
		OpcodeValue == 0xEB)    // 0xEB Jump Byte Offset
		return true;

	if (OpcodeValue == 0xFF)
	{
		uint8_t SecOpcode = Opcode.SecondaryOpcode & 0b00111000;
		if (SecOpcode == 0x20 ||	// 0xFF /4 Indirect Jump Mod/RM
			SecOpcode == 0x28)		// 0xFF /5 Indirect Far Jump 
			return true;
	}

	return false;
}

bool IsCall(X86Disasm::ResultData& Opcode)
{
	if (!HasFlag(Opcode.OpcodeData->Flags, X86Disasm::OpcodeFlags::Branch))
		return false;

	uint8_t OpcodeValue = Opcode.Opcode;

	if (OpcodeValue == 0x9A ||		// 0x9A FAR Call
		OpcodeValue == 0xE8)		// 0xEB Call Opword Offset
		return true;

	if (OpcodeValue == 0xFF)
	{
		uint8_t SecOpcode = Opcode.SecondaryOpcode & 0b00111000;
		if (SecOpcode == 0x10 ||	// 0xFF /2 Indirect Call Mod/RM
			SecOpcode == 0x18)		// 0xFF /3 Indirect Far Call
			return true;
	}

	return false;
}

bool HasFarAddress(X86Disasm::ResultData& Opcode)
{
	for (unsigned int x = 0; x < 5; x++)
	{
		auto Param = X86Disasm::ParamList[Opcode.OpcodeData->Params[x]];
		if (Param.Type == X86Disasm::ParamType::Immidate && Param.Size == X86Disasm::ParamSize::FarPointer)
		{
			return true;
		}
	}

	return false;
}

void DumpOpcode(std::ofstream& Output, uint16_t Address, X86Disasm::ResultData& Opcode, bool Video, std::string Remark)
{
	std::string Bytes;
	std::string Memonic;
	std::string Params;
	std::string Notes = Remark;

	Bytes = X86Disasm::OpcodeByteString(Opcode);
	Memonic = X86Disasm::OpcodeMemonicString(Opcode);

	uint16_t DestAddress = 0;
	bool ExactDest = false;
	
	// Building the param output ourselves so we can lookup address
	for (int x = 0; x < 5; x++)
	{
		if (Opcode.OpcodeData->Params[x] == 0)
			break;

		if (x != 0)
		{
			Params += ", ";
		}

		auto ParamInfo = X86Disasm::GetParam(Opcode, x);
		bool First = true;

		if (ParamInfo.ValueType == X86Disasm::ValueType::Address)
		{
			std::string Value = X86Disasm::ParamSizeString(ParamInfo);
			if (Value.size() != 0)
				Value += " ";

			Value += "[";
			Value += X86Disasm::SegmentOverrideString(Opcode);

			if (ParamInfo.Base != X86Disasm::Register::None)
			{
				Value += X86Disasm::GetRegisterName(ParamInfo.Base);
				First = false;

				// Most of the time, DS:DI Addresses are the user data (but not always)
				if (ParamInfo.Base == X86Disasm::Register::DI && Opcode.SegmentOverride == 0)
				{
					if (Notes.length() != 0)
						Notes += " ";

					ForthWord UserWord = Find(UserBase + (uint16_t)ParamInfo.Value);
					Notes += UserWord.Name;
				}
			}

			if (ParamInfo.Index != X86Disasm::Register::None)
			{
				if (!First)
					Value += "+";

				Value += X86Disasm::GetRegisterName(ParamInfo.Index);

				if (ParamInfo.Scale != 1)
					Value += std::format("*{0}", ParamInfo.Scale);
			}

			// always print out the value if nothing else has been printed
			if (ParamInfo.Value != 0 || First)
			{
				DestAddress = (uint16_t)ParamInfo.Value;
				// If Base and Index were blank, always print the displacment, even if 0
				if (!First)
				{
					// We want to alwasy have the sign for this, as it's applying a displacment 
					if (ParamInfo.Value >= -64 && ParamInfo.Value <= 64)
						Value += std::format("{0:+}", ParamInfo.Value);
					else
						Value += std::format("{0:+0{1}X}h", ParamInfo.Value, Opcode.Mode / 4);
				}
				else
				{
					// Just a straight address 
					if (Opcode.AddressSize == 16)
						Value += std::format("{0:04X}h", (uint16_t)ParamInfo.Value);
					else if (Opcode.AddressSize == 32)
						Value += std::format("{0:08X}h", (uint32_t)ParamInfo.Value);
					else if (Opcode.AddressSize == 64)
						Value += std::format("{0:016X}h", (uint64_t)ParamInfo.Value);
				}
			}

			Value += "]";

			Params += Value;
			//Pos += Value.length();
			//Output << Value;
		}
		else
		{
			std::string Value = "(BAD)";
			if (ParamInfo.Base == X86Disasm::Register::None)
			{
				if (ParamInfo.ValueType == X86Disasm::ValueType::Relative)
				{
					DestAddress = (Address + Opcode.Length + (int16_t)ParamInfo.Value);
					Value = std::format("{0:04X}h", DestAddress);
				}
				else if (ParamInfo.ValueType == X86Disasm::ValueType::FarPointer)
				{
					Value = std::format("{0:04X}:{1:04X}", (uint16_t)(ParamInfo.Value >> 32), (uint16_t)ParamInfo.Value);
				}
				else
				{
					if (ParamInfo.Value >= -64 && ParamInfo.Value <= 64 && Opcode.OpcodeData->Name != "INT")
						Value = std::format("{0}", ParamInfo.Value);
					else if (ParamInfo.DataSize == 1)
						Value = std::format("{0:02X}h", (uint16_t)ParamInfo.Value);
					else if (ParamInfo.DataSize == 2)
						Value = std::format("{0:04X}h", (uint16_t)ParamInfo.Value);

					DestAddress = (uint16_t)ParamInfo.Value;
					ExactDest = true;
				}
			}
			else
			{
				Value = X86Disasm::GetRegisterName(ParamInfo.Base);
			}

			Params += Value;
		}
	}

	if ((uint16_t)DestAddress > 0 && !IsJump(Opcode))
	{
		if (IsCall(Opcode))
		{
			if (Video)
				DestAddress += 0xE000;

			ForthWord Var = Find(DestAddress);
			if (!Var.IsValid())
				Var = Find(DestAddress - 2);

			if (Var.IsValid())
			{
				if (Notes.length() != 0)
					Notes += " ";

				Notes += Var.Name;
			}
		}
		else if (DestAddress > 0x100)
		{
			ForthWord Var;

			if (!OverlayAddress((uint16_t)DestAddress))
				Var = Files[0x0000].Words.FindBefore((uint16_t)DestAddress + 1);
			else
				Var = Files[CurrentOverlay].Words.FindBefore((uint16_t)DestAddress + 1);

			if (Var.IsValid())
			{
				if (!ExactDest || Var.DataAddress == DestAddress)
				{
					if (Notes.length() != 0)
						Notes += " ";

					Notes += Var.Name;

					if (Var.DataAddress != DestAddress)
					{
						Notes += std::format("{0:+}", DestAddress - (int16_t)Var.DataAddress);
					}

				}
			}
		}
	}


	std::string Value = std::format("{0:04X} {1:20} {2:10} {3}", Address, Bytes, Memonic, Params);
	if (Notes.length() != 0)
	{
		Value = std::format("{0:70}; {1}", Value, Notes);
	}

	Output << "   " << Value << "\n";
}

struct RehydrateCodeData
{
	// Address 
	uint16_t Address = 0;
	std::string Label;
	X86Disasm::ResultData Opcode;
};

void RehydrateCode(std::ofstream& Output, ForthWord& Entry, bool Video)
{
	uint16_t BaseAddress = 0;

	if (Entry.Type == WordType::RawCode)
		BaseAddress = Entry.BaseAddress;
	else
		BaseAddress = Entry.DataAddress;


	uint16_t Address = BaseAddress;

	std::vector<RehydrateCodeData> RehydrateData;

	while (Address < BaseAddress + Entry.DataLength)
	{
		RehydrateCodeData DataEntry;

		DataEntry.Opcode = X86Disasm::ReadOpcode(GetPtr(Address), 16);

		if (Video)
			DataEntry.Address = Address - 0xE000;
		else
			DataEntry.Address = Address;

		Address += DataEntry.Opcode.Length;

		if (IsJump(DataEntry.Opcode))
		{
			if (!HasFarAddress(DataEntry.Opcode) && HasFlag(DataEntry.Opcode.OpcodeData->Flags, X86Disasm::OpcodeFlags::NoModRM))
			{
				if (DataEntry.Opcode.Immidate.Word[0] != 0)
				{
					RehydrateCodeData LabelEntry;
					LabelEntry.Address = Address + (int16_t)DataEntry.Opcode.Immidate.Word[0];
					if (Video)
						LabelEntry.Address -= 0xE000;

					LabelEntry.Label = std::format(".{}_{:04X}", Files[CurrentOverlay].Name, LabelEntry.Address);
					LabelEntry.Opcode.Clear();

					DataEntry.Label = LabelEntry.Label;

					RehydrateData.emplace_back(LabelEntry);
				}
			}
		}

		RehydrateData.emplace_back(DataEntry);
	};


	Address = BaseAddress;

	std::sort(RehydrateData.begin(), RehydrateData.end(), [](const RehydrateCodeData& rhs, const RehydrateCodeData& lhs) {
		if (rhs.Address == lhs.Address)
			return rhs.Label.length() > lhs.Label.length();
		return rhs.Address < lhs.Address;
		});

	if (Entry.CodePointer == Entry.DataAddress)
		Output << "CODE " << Entry.Name;
	else
		Output << "CREATE " << Entry.Name;

	if (HasFlag(Entry.Flags, WordFlags::Immediate))
		Output << " IMMEDIATE";

	Output << "\n";

	if (Video)
		Output << std::format("(Address @{:04X}\n", Address - 0xE000);
	else
		Output << std::format("(Address @{:04X}\n", Address);


	for (auto &DataEntry : RehydrateData)
	{
		if (DataEntry.Opcode.OpcodeData == X86Disasm::OpcodeBad && DataEntry.Label != "")
			Output << " " << DataEntry.Label << ":\n";
		else
		{
			DumpOpcode(Output, DataEntry.Address, DataEntry.Opcode, Video, DataEntry.Label);			
			//Output << "        " << X86Disasm::OpcodeTabbedString(DataEntry.Opcode, DataEntry.Address, DataEntry.Label) << "\n";
		}
	}

	Output << ") CODE-END\n";
}


struct RehydrateWordData
{
	// Address 
	uint16_t Address = 0;
	ForthWord Word;

	uint16_t BranchDest = 0;

	std::string Name;
	uint16_t Lit[2] = {};
	std::string StringLit;

	bool FakeWord = false;
};

static bool RehydrateWordSort(const RehydrateWordData& lhs, const RehydrateWordData& rhs)
{
	if (lhs.Address == rhs.Address)
	{
		if (lhs.Name == "ELSE" && rhs.Name == "THEN")
			return false;

		if (lhs.Name == "THEN" && rhs.Name == "ELSE")
			return true;

		if (lhs.Name == "THEN" && rhs.Name == "AGAIN")
			return true;

		if (lhs.Name == "AGAIN" && rhs.Name == "THEN")
			return false;

		if (lhs.Name == "THEN" && rhs.Name == "REPEAT")
			return true;

		if (lhs.Name == "REPEAT" && rhs.Name == "THEN")
			return false;

		if (rhs.FakeWord == true)
			return false;

		else
			return true;
	}

	return lhs.Address < rhs.Address;
};

static std::vector<RehydrateWordData>::iterator FindRehydrateWord(std::vector<RehydrateWordData>& RehydratedData, uint16_t Address)
{
	auto value = std::find_if(RehydratedData.begin(), RehydratedData.end(), [Address](const RehydrateWordData& obj) {
		return obj.Address == Address;
		});

	return value;
}

extern uint16_t DoesPtr;


void RehydrateWord(std::ofstream& Output, ForthWord& Entry)
{
	uint16_t Address = 0;
	uint16_t SavedOverlay = CurrentOverlay;

	uint16_t CodeStart = 0xFFFF;

	std::vector<RehydrateWordData> RehydratedData;

	// Build the Rehydration data 
	while (Address < Entry.DataLength)
	{
		RehydrateWordData DataEntry;
		DataEntry.Lit[0] = DataEntry.Lit[1] = DataEntry.BranchDest = 0;

		DataEntry.Address = Address;
		DataEntry.FakeWord = false;

		uint16_t Token = ReadWord(Entry.DataAddress + Address);
		Address += 2;

		ForthWord CurrentWord = Find(Token);
		if (!CurrentWord.IsValid())
		{
			DataEntry.Name = std::format("_?[{:04X}]", Token);
		}
		else if (CurrentWord.Type == WordType::Bad)
		{
			DataEntry.Name = std::format("_?[{:04X}]", Token);
		}
		else

		{
			DataEntry.Name = CurrentWord.Name;
			DataEntry.Word = CurrentWord;
		}

		if (CurrentWord.Name == "(\")")
			DataEntry.Name = "\"";
		if (CurrentWord.Name == "(.\")")
			DataEntry.Name = ".\"";
		if (CurrentWord.Name == "(ABORT\")")
			DataEntry.Name = "ABORT\"";
		if (CurrentWord.Name == "(DO)")
			DataEntry.Name = "DO";
		if (CurrentWord.Name.find("LOOP)") != -1)
			DataEntry.Name = CurrentWord.Name.substr(1, CurrentWord.Name.length() - 2);


		if (HasFlag(CurrentWord.Flags, WordFlags::Exit))
		{
			if (Address == Entry.DataLength)
				DataEntry.Name = ";";
		}

		if (HasFlag(CurrentWord.Flags, WordFlags::Compile))
		{
			DataEntry.Lit[0] = ReadWord(Entry.DataAddress + Address);
			Address += 2;
		}
		else if (HasFlag(CurrentWord.Flags, WordFlags::Branch))
		{
			DataEntry.Lit[0] = ReadWord(Entry.DataAddress + Address);
			DataEntry.BranchDest = Address + DataEntry.Lit[0];

			Address += 2;
		}
		else if (HasFlag(CurrentWord.Flags, WordFlags::Lit))
		{
			for (int x = 0; x < CurrentWord.ParamCount; x++)
			{
				DataEntry.Lit[x] = ReadWord(Entry.DataAddress + Address);
				Address += 2;
			}
		}
		else if (HasFlag(CurrentWord.Flags, WordFlags::String))
		{
			uint8_t Len = ReadByte(Entry.DataAddress + Address);
			Address += 1;

			std::string Value;

			for (int x = 0; x < Len; x++)
			{
				DataEntry.StringLit += ReadByte(Entry.DataAddress + Address);
				Address += 1;
			}
		}

		if (CurrentWord.Type == WordType::Overlay)
		{
			LoadOverlay(ReadWord(CurrentWord.DataAddress));
		}

		if (HasFlag(CurrentWord.Flags, WordFlags::Code))
		{
			// Look for the 'DOES>' pattern (;Code) JMP 1649
			if (ReadByte(Entry.DataAddress + Address) == 0xE8)
			{
				int16_t Dest = ReadWord(Entry.DataAddress + Address + 1);
				if ((uint16_t)(Entry.DataAddress + Address + Dest + 3) == DoesPtr)
				{
					DataEntry.Name = std::format("DOES> ( CFA @{:04X} )", Entry.DataAddress + Address);
					Address += 3;
				}
			}
			else
			{
				CodeStart = Address;
				RehydratedData.emplace_back(DataEntry);
				break;
			}
		}

		RehydratedData.emplace_back(DataEntry);
	};


	if (SavedOverlay != CurrentOverlay)
	{
		LoadOverlay(SavedOverlay);
	}

	// Process Branches
	std::vector<RehydrateWordData> BranchData;

	for (auto& WordEntry : RehydratedData)
	{
		// IF		0BRANCH forwarad, Word before dest is a BRANCH forward (ELSE) or normal word (THEN)
		// ELSE		BRANCH forward 
		// THEN		Result of IF/ELSE
		// BEGIN	found from a branch backwards
		// AGAIN	BRANCH Backwards
		// WHILE	0BRANCH forwards, word before DEST is BRANCH Backwards
		// UNTIL	0BRANCH Backwards
		// REPEAT	BRANCH Backwards with an WHILE

		if (WordEntry.Name == "0BRANCH" && WordEntry.BranchDest > WordEntry.Address)
		{
			// Forward 0BRANCH can be an IF or an WHILE
			auto Pos = FindRehydrateWord(RehydratedData, WordEntry.BranchDest);
			Pos--;
			if (Pos->Word.Name == "BRANCH" && Pos->BranchDest < Pos->Address)
			{
				WordEntry.Name = "WHILE";
				if (Pos->Name == "REPEAT")
				{
					RehydrateWordData DataEntry;
					DataEntry.Address = WordEntry.BranchDest;
					DataEntry.Word.Type = WordType::Invalid;
					DataEntry.Name = "THEN";
					DataEntry.FakeWord = true;

					BranchData.emplace_back(DataEntry);
				}

				Pos->Name = "REPEAT";

				RehydrateWordData DataEntry;
				DataEntry.Address = Pos->BranchDest;
				DataEntry.Word.Type = WordType::Invalid;
				DataEntry.Name = "BEGIN";
				DataEntry.FakeWord = true;

				BranchData.emplace_back(DataEntry);
			}
			else
			{
				WordEntry.Name = "IF";
				if (Pos->Name != "BRANCH")
				{
					RehydrateWordData DataEntry;
					DataEntry.Address = WordEntry.BranchDest;
					DataEntry.Word.Type = WordType::Invalid;
					DataEntry.Name = "THEN";
					DataEntry.FakeWord = true;

					BranchData.emplace_back(DataEntry);

				}
			}
		}
		else if (WordEntry.Name == "BRANCH" && WordEntry.BranchDest == WordEntry.Address)
		{
			// BEGIN AGAIN infinate loop
			WordEntry.Name = "AGAIN";

			RehydrateWordData DataEntry;
			DataEntry.Address = WordEntry.BranchDest;
			DataEntry.Word.Type = WordType::Invalid;
			DataEntry.Name = "BEGIN";
			DataEntry.FakeWord = true;

			BranchData.emplace_back(DataEntry);
		}
		else if (WordEntry.Name == "BRANCH" && WordEntry.BranchDest > WordEntry.Address)
		{
			WordEntry.Name = "ELSE";

			RehydrateWordData DataEntry;
			DataEntry.Address = WordEntry.BranchDest;
			DataEntry.Word.Type = WordType::Invalid;
			DataEntry.Name = "THEN";
			DataEntry.FakeWord = true;

			BranchData.emplace_back(DataEntry);
		}
		else if (WordEntry.Name == "0BRANCH" && WordEntry.BranchDest < WordEntry.Address)
		{
			WordEntry.Name = "UNTIL";

			RehydrateWordData DataEntry;
			DataEntry.Address = WordEntry.BranchDest;
			DataEntry.Word.Type = WordType::Invalid;
			DataEntry.Name = "BEGIN";
			DataEntry.FakeWord = true;

			BranchData.emplace_back(DataEntry);
		}
		else if (WordEntry.Name == "BRANCH" && WordEntry.BranchDest < WordEntry.Address)
		{
			WordEntry.Name = "AGAIN";

			RehydrateWordData DataEntry;
			DataEntry.Address = WordEntry.BranchDest;
			DataEntry.Word.Type = WordType::Invalid;
			DataEntry.Name = "BEGIN";
			DataEntry.FakeWord = true;

			BranchData.emplace_back(DataEntry);
		}
	}


	for (auto BrachEntry : BranchData)
	{
		RehydratedData.emplace_back(BrachEntry);
	}

	std::sort(RehydratedData.begin(), RehydratedData.end(), RehydrateWordSort);

	Output << ": " << Entry.Name;

	if (HasFlag(Entry.Flags, WordFlags::Immediate))
		Output << " IMMEDIATE";

	Output << "\n";

	std::string Line = " ";
	bool ValidLine = false;
	int Indent = 1;

	for (auto& WordEntry : RehydratedData)
	{
		if (Indent == 0)
			Indent = 1;

		if (Line.length() == 0 || Line.length() > 80)
		{
			if (Line.length() != 0)
				Output << Line << "\n";

			Line = "";
			for (int x = 0; x < Indent; x++)
				Line += " ";

			ValidLine = false;
		}

		if (HasFlag(WordEntry.Word.Flags, WordFlags::Compile))
		{
			Line += WordEntry.Name + " ";

			auto Match = Find(WordEntry.Lit[0]);

			Line += Match.Name + " ";
			ValidLine = true;
		}
		else if (HasFlag(WordEntry.Word.Flags, WordFlags::Lit) && !HasFlag(WordEntry.Word.Flags, WordFlags::Branch))
		{
			if (WordEntry.Word.Name == "2LIT")
			{
				int32_t Value = (WordEntry.Lit[1] << 16) + WordEntry.Lit[0];

				if (ValidInstanceRecord(Value))
					Line += std::format("${:06X}. ( {} ) ", Value, GetInstanceRecordValue(Value));
				else
					Line += std::format("{}. ", Value);
			}
			else
			{
				auto Match = Find(WordEntry.Lit[0] - 2);

				if (!Match.IsValid())
					Match = Find(WordEntry.Lit[0]);

				if (WordEntry.Lit[0] > 0x100 && Match.IsValid())
				{
					Line += "' " + Match.Name + std::format(" ( ${:04X} ) ", WordEntry.Lit[0]);
				}
				else
				{
					int16_t Value = WordEntry.Lit[0];
					if (Value >= -128 && Value < 128)
						Line += std::format("{} ", Value);
					else
						Line += std::format("${:04X} ", WordEntry.Lit[0]);
				}

			}
			ValidLine = true;
		}
		else
		{
			if (WordEntry.Name.find("DOES>") != -1)
			{
				Line += WordEntry.Name + " ";

				Output << Line << "\n";
				Line = "";
				Indent++;

				ValidLine = true;
			}
			else if (HasFlag(WordEntry.Word.Flags, WordFlags::String))
			{
				Line += WordEntry.Name + " ";

				Line += WordEntry.StringLit + "\" ";

				ValidLine = true;
			}
			else if (HasFlag(WordEntry.Word.Flags, WordFlags::Immediate))
			{
				Line += "[COMPILE] " + WordEntry.Name + " ";

				ValidLine = true;
			}
			else
			{
				if (WordEntry.Name == "BEGIN" || WordEntry.Name == "DO" || WordEntry.Name == "IF")
				{
					Line += WordEntry.Name + " ";

					Output << Line << "\n";
					Line = "";
					Indent++;
				}
				else if (WordEntry.Name == "ELSE")
				{
					if (ValidLine)
					{
						Output << Line << "\n";
					}

					Line = "";

					for (int x = 0; x < Indent - 1; x++)
						Line += " ";

					Line += WordEntry.Name + " ";
					Output << Line << "\n";
					Line = "";
				}
				else if (WordEntry.Name == "WHILE")
				{
					Line += WordEntry.Name + " ";

					Output << Line << "\n";
					Line = "";
				}
				else if (WordEntry.Name == "AGAIN" || WordEntry.Name == "REPEAT" || WordEntry.Name == "UNTIL" || WordEntry.Name == "THEN" || WordEntry.Name.find("LOOP") != -1)
				{
					if (ValidLine)
					{
						Output << Line << "\n";
					}

					Line = "";
					Indent--;

					for (int x = 0; x < Indent; x++)
						Line += " ";

					Line += WordEntry.Name + " ";
					Output << Line << "\n";
					Line = "";

				}
				else
				{
					if (WordEntry.Word.Name == "BRANCH" || WordEntry.Word.Name == "0BRANCH")
					{
						std::cout << std::format(" WARN: Unresolved branch in {}\n", Entry.Name);
					}

					Line += WordEntry.Name + " ";
					ValidLine = true;
				}
			}
		}


	}

	Output << Line << "\n";

	if (CodeStart != 0xFFFF)
	{
		Address = Entry.DataAddress + CodeStart;

		Output << std::format(" Address @{:04X}\n", Address);

		while (Address < Entry.DataAddress + Entry.DataLength)
		{
			auto Opcode = X86Disasm::ReadOpcode(GetPtr(Address), 16);

			DumpOpcode(Output, Address, Opcode, false, "");

			Address += Opcode.Length;
		}

	}
}
