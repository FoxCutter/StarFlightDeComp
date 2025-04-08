/*
 *  Copyright 2022 Fox Cutter
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 */

#include "X86Disasm.h"
#include "Internal.h"

#include <cstdlib>
#include <memory>


template<typename ... Args>
std::string string_format(const std::string& format, Args ... args)
{
	int size_s = std::snprintf(nullptr, 0, format.c_str(), args ...) + 1; // Extra space for '\0'
	if (size_s <= 0)
	{
		return std::string();
	}
	auto size = static_cast<size_t>(size_s);
	auto buf = std::make_unique<char[]>(size);

	std::snprintf(buf.get(), size, format.c_str(), args ...);

	return std::string(buf.get(), buf.get() + size - 1); // We don't want the '\0' inside
}

namespace X86Disasm
{
	std::string OpcodeByteString(const ResultData& Results, bool Wrap)
	{
		std::string ResultString = "";

		for (int x = 0; x < Results.Length; x++)
		{
			ResultString += string_format("%02X ", Results.Data[x]);
			if (Wrap && x == 5 && x < Results.Length - 1)
			{
				ResultString += "\n";
			}
		}

		return ResultString;
	}

	const char* GetPrefixName(uint8_t Prefix);

	std::string VEXString(const ResultData& Results)
	{
		std::string ResultString;

		if (Results.Rex.W)
			ResultString += "W";
		if (Results.Rex.R)
			ResultString += "R";
		if (Results.Rex.X)
			ResultString += "X";
		if (Results.Rex.B)
			ResultString += "B";
		if (Results.R2 && Results.OpcodeSpace == OpcodeSpace::EVEX)
			ResultString += "R'";

		if (!ResultString.empty())
			ResultString += ".";

		if (Results.Map == 1)
			ResultString += "0F.";
		else if (Results.Map == 2)
			ResultString += "0F38.";
		else if (Results.Map == 3)
			ResultString += "0F3A.";
		else if (Results.Map == 4)
			ResultString += "0F0F.";
		else
			ResultString += string_format("MAP%X.", Results.Map);

		if (Results.Prefix == RefiningPrefix::None)
			ResultString += "NP.";
		else if (Results.Prefix == RefiningPrefix::OSZ)
			ResultString += "66.";
		else if (Results.Prefix == RefiningPrefix::REP)
			ResultString += "F3.";
		else if (Results.Prefix == RefiningPrefix::REPNE)
			ResultString += "F2.";

		if (Results.VexSize != 0)
			ResultString += string_format("%u.", Results.VexSize);

		if (Results.OpcodeSpace == OpcodeSpace::EVEX)
		{
			if (Results.Zeroing)
				ResultString += "Z.";
			if (Results.BCRC)
				ResultString += "B.";
		}

		ResultString += string_format("V%x", Results.VexReg, Results.OpcodeSpace == OpcodeSpace::EVEX ? 5 : 4);

		if (Results.OpcodeSpace == OpcodeSpace::EVEX)
			ResultString += string_format(".K%x", Results.MaskReg);


		return ResultString;
	}

	bool IsRefiningPrefix(uint8_t Value);

	bool IsPrintablePrefix(uint8_t Value)
	{
		if (Value == PrefixOpcodes::Map01 || Value == PrefixOpcodes::Map02 || Value == PrefixOpcodes::Map03 || Value == PrefixOpcodes::Map04)
			return false;

		return true;
	}

	std::string FullPrefixString(const ResultData& Results, bool NewLine)
	{
		std::string ResultString;

		for (int x = 0; x < Results.PrefixCount; x++)
		{
			std::string PrefixString = "";

			uint8_t Value = Results.Data[x];
			if (IsSegmentPrefix(Value))
			{
				if ((Value == PrefixOpcodes::CS || Value == PrefixOpcodes::DS) &&
					HasFlag(Results.OpcodeData->Flags, OpcodeFlags::Conditional))
				{
					if (Value == PrefixOpcodes::CS)
						PrefixString += "BRANCH_NOT_HINT";
					else
						PrefixString += "BRANCH_HINT";
				}
				else
				{
					PrefixString += GetPrefixName(Value);
				}
			}
			else if (Results.Mode == 64 && Value >= 0x40 && Value <= 0x4F)
			{
				PrefixString += "REX";

				if (Value != 0x40)
					PrefixString += ".";

				if (Results.Rex.W)
					PrefixString += "W";

				if (Results.Rex.R)
					PrefixString += "R";

				if (Results.Rex.X)
					PrefixString += "X";

				if (Results.Rex.B)
					PrefixString += "B";
			}
			else if (IsRefiningPrefix(Value))
			{
				if (!HasFlag(Results.OpcodeData->Flags, OpcodeFlags::ExtendedPrefix))
				{
					if (Value == PrefixOpcodes::REPNE)
					{
						if (HasFlag(Results.OpcodeData->Flags, OpcodeFlags::BNDPrefix))
							PrefixString += "BND";

						else
							PrefixString += GetPrefixName(Value);
					}
					else if (Value == PrefixOpcodes::REP || Value == PrefixOpcodes::OpcodeSize)
					{
						PrefixString += GetPrefixName(Value);
					}
				}
			}
			else if (Value == PrefixOpcodes::VEX2 || Value == PrefixOpcodes::VEX3 || Value == PrefixOpcodes::XOP)
			{
				PrefixString += GetPrefixName(Value);
				PrefixString += ".";
				PrefixString += VEXString(Results);

				x++;
				if (Value != PrefixOpcodes::VEX2)
					x++;
			}
			else if (Value == PrefixOpcodes::EVEX)
			{
				PrefixString += GetPrefixName(Value);
				PrefixString += ".";
				PrefixString += VEXString(Results);
				x += 3;
			}
			else if (IsPrintablePrefix(Value))
			{
				PrefixString += GetPrefixName(Value);
			}


			if (!PrefixString.empty())
			{
				if (!ResultString.empty())
					ResultString += NewLine ? "\n" : " ";

				ResultString += PrefixString;
			}
		}

		return ResultString;
	}

	std::string ParamString(const ResultData& Results, int ParamIndex, std::string& Notes, intptr_t Address)
	{
		std::string ResultString;

		auto ParamInfo = GetParam(Results, ParamIndex);

		if (ParamInfo.ValueType == X86Disasm::ValueType::Address)
		{
			bool First = true;

			ResultString = X86Disasm::ParamSizeString(ParamInfo);
			if (ResultString.size() != 0)
				ResultString += " ";

			ResultString += "[";
			ResultString += X86Disasm::SegmentOverrideString(Results);

			if (ParamInfo.Base != X86Disasm::Register::None)
			{
				ResultString += X86Disasm::GetRegisterName(ParamInfo.Base);
				First = false;
			}

			if (ParamInfo.Index != X86Disasm::Register::None)
			{
				if (!First)
					ResultString += "+";

				ResultString += X86Disasm::GetRegisterName(ParamInfo.Index);

				if (ParamInfo.Scale != 1)
					ResultString += string_format("*%u", ParamInfo.Scale);
			}

			// always print out the value if nothing else has been printed
			if (ParamInfo.Value != 0 || First)
			{
				// If Base and Index were blank, always print the displacment, even if 0
				if (!First)
				{
					// We want to alwasy have the sign for this, as it's applying a displacment 
					if (ParamInfo.Value >= -64 && ParamInfo.Value <= 64)
						ResultString += string_format("%+d", ParamInfo.Value);
					else if (Results.Mode == 16)
						ResultString += string_format("%04Xh", ParamInfo.Value);
					else if (Results.Mode == 32)
						ResultString += string_format("%08Xh", ParamInfo.Value);
					else if (Results.Mode == 64)
						ResultString += string_format("%016Xh", ParamInfo.Value);
					
				}
				else
				{
					// Just a straight address 
					if (ParamInfo.DataSize == 2)
						ResultString += string_format("%04Xh", (uint16_t)ParamInfo.Value);
					else if (ParamInfo.DataSize == 4)
						ResultString += string_format("%08Xh", (uint32_t)ParamInfo.Value);
					else if (ParamInfo.DataSize == 8)
						ResultString += string_format("%016Xh", (uint64_t)ParamInfo.Value);
				}
			}

			ResultString += "]";
		}
		else if (ParamInfo.ValueType == ValueType::FarPointer)
		{
			uint16_t Seg = (uint16_t)(ParamInfo.Value >> 32);
			if (Results.OpSize == 32)
			{
				ResultString = string_format("%04X:%08X", Seg, (uint32_t)ParamInfo.Value);
			}
			else
			{
				ResultString = string_format("%04X:%04X", Seg, (uint16_t)ParamInfo.Value);

				if (!Notes.empty())
					Notes += " ";

				// Convert 16-Bit Seg:Offset to a flat address
				Notes = string_format("(%Xh)", ((uint32_t)Seg << 4) + (uint16_t)ParamInfo.Value);

			}
		}
		else if (ParamInfo.ValueType == ValueType::Relative)
		{
			if (!Notes.empty())
				Notes += " ";

			Notes += string_format("(%+d)", (uint64_t)ParamInfo.Value);

			if (Results.Mode == 16)
				ResultString += string_format("%04Xh", (Address + Results.Length + ParamInfo.Value));
			else if (Results.Mode == 32)
				ResultString += string_format("%08Xh", (Address + Results.Length + ParamInfo.Value));
			else if (Results.Mode == 64)
				ResultString += string_format("%016Xh", (Address + Results.Length + ParamInfo.Value));
		}
		else if (ParamInfo.ValueType == ValueType::RipRealative)
		{
			if (!Notes.empty())
				Notes += " ";

			Notes += string_format("(RIP%+d)", ParamInfo.Value);

			ResultString = string_format("%016Xh", (Address + Results.Length + ParamInfo.Value));
		}
		else if (ParamInfo.ValueType == ValueType::Immidate)
		{
			// I just like INT being hex is all. 
			if (ParamInfo.Value >= -64 && ParamInfo.Value <= 64 && Results.OpcodeData->Name != "INT")
				ResultString = string_format("%u", ParamInfo.Value);
			else if (ParamInfo.DataSize == 1)
				ResultString = string_format("%02Xh", (uint8_t)ParamInfo.Value);
			else if (ParamInfo.DataSize == 2)
				ResultString = string_format("%04Xh", (uint16_t)ParamInfo.Value);
			else if (ParamInfo.DataSize == 4)
				ResultString = string_format("%08Xh", (uint32_t)ParamInfo.Value);
			else if (ParamInfo.DataSize == 8)
				ResultString = string_format("%016Xh", (uint64_t)ParamInfo.Value);
		}
		else if (ParamInfo.Base >= Register::k0 && ParamInfo.Base <= Register::k7)
		{
			auto Param = ParamList[Results.OpcodeData->Params[ParamIndex]];

			ResultString = " {";
			ResultString += GetRegisterName(ParamInfo.Base);
			ResultString += "}";
			if (HasFlag(Param.Flags, ParamFlags::ZEROMASK) && Results.Zeroing)
				ResultString += "{z}";
		}
		else
		{
			ResultString += GetRegisterName(ParamInfo.Base);
		}

		return ResultString;
	}

	std::string ParamStringRounding(const ResultData& Results)
	{
		std::string ResultString;

		std::string Extra = "";

		for (int x = 0; x < 5; x++)
		{
			if (Results.OpcodeData->Params[x] == 0)
				break;

			auto Param = ParamList[Results.OpcodeData->Params[x]];

			if (Results.BCRC)
			{
				if (HasFlag(Param.Flags, ParamFlags::SAE))
					Extra += "{sae}";

				if (HasFlag(Param.Flags, ParamFlags::ROUNDING))
				{
					switch (Results.Rounding)
					{
					case RoundingType::Nearest:
						Extra += "{rn-sae}";
						break;
					case RoundingType::Down:
						Extra += "{rd-sae}";
						break;
					case RoundingType::Up:
						Extra += "{ru-sae}";
						break;
					case RoundingType::Zero:
						Extra += "{rz-sae}";
						break;
					}
				}

				if (HasFlag(Param.Flags, ParamFlags::BROADCAST))
				{
					if (Results.OpcodeData->BroadcastSize == 0)
						Extra += " {1toNAN}";
					else
						Extra += string_format(" {1to%u}", Results.OpcodeData->Disp8 / Results.OpcodeData->BroadcastSize);
				}
			}
		}

		if (!Extra.empty())
			ResultString += " " + Extra;

		return ResultString;
	}

	std::string FullParamString(const ResultData& Results, std::string& Notes, intptr_t Address)
	{
		std::string ResultString;

		for (int x = 0; x < 5; x++)
		{
			if (Results.OpcodeData->Params[x] == 0)
				break;

			auto Param = ParamList[Results.OpcodeData->Params[x]];

			if (x != 0 && Param.Type != ParamType::MaskReg)
				ResultString += ", ";

			ResultString += ParamString(Results, x, Notes, Address);
		}
		std::string Rounding = ParamStringRounding(Results);

		if (!Rounding.empty())
			ResultString += " " + Rounding;

		return ResultString;
	}

	std::string OpcodeString(const ResultData& Results, intptr_t Address)
	{
		std::string ResultString;

		std::string Notes;

		ResultString += Results.OpcodeData->Name;
		ResultString += "\t";
		ResultString += FullParamString(Results, Notes, Address);

		if (!Notes.empty())
		{
			ResultString += "\t; ";
			ResultString += Notes;
		}


		return ResultString;
	}

	bool IsEncodingPrefix(uint8_t Value)
	{
		if (Value == PrefixOpcodes::VEX2 || Value == PrefixOpcodes::VEX3 || Value == PrefixOpcodes::EVEX || Value == PrefixOpcodes::XOP)
			return true;

		return false;
	}

	std::string OpcodeTabbedString(const ResultData& Results, intptr_t Address, const std::string& Remark)
	{
		std::string ResultString;
	
		if (Results.Mode == 16)
			ResultString += string_format("%04Xh", Address);
		else if (Results.Mode == 32)
			ResultString += string_format("%08Xh", Address);
		else if (Results.Mode == 64)
			ResultString += string_format("%016Xh", Address);

		int Count = 0;
		int Pos = 0;

		bool Full = true;
		bool SplitPrefix = false;

		if (SplitPrefix)
		{
			if (Results.PrefixCount != 0)
			{
				for (Pos = 0; Pos < Results.PrefixCount; Pos++)
				{
					ResultString += string_format("%02X ", Results.Data[Pos]);
					Count++;

					if (Count == 5)
					{
						ResultString += "\n\t";
						Count = 0;
					}

					if (IsEncodingPrefix(Results.Data[Pos]) && !Full)
						break;
				}

				std::string Prefix = "";
				if(Full)
					Prefix = FullPrefixString(Results, false);
				else 
					Prefix = PrefixString(Results, false);

				if (!Prefix.empty())
				{
					ResultString += "\t";
					ResultString += Prefix;
					ResultString += "\n\t";
					Count = 0;
				}
			}
		}


		// Print the rest of the bytes
		ResultString += OpcodeByteString(Results, true);
		ResultString += "\t";


		ResultString += OpcodeMemonicString(Results, !SplitPrefix) + " ";
		std::string Extra = "";

		ResultString += FullParamString(Results, Extra, Address);

		if (!Extra.empty() || !Remark.empty())
		{
			ResultString += "\t; ";
			if (!Extra.empty())
			{
				ResultString += Extra;
				ResultString += " ";
			}

			ResultString += Remark;
		}

		return ResultString;
	}
}