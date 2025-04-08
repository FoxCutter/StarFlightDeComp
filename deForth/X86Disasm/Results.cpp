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

namespace X86Disasm
{
	const char* RegNames[] =
	{
		"AL", "CL", "DL", "BL", "SPL", "BPL", "SIL", "DIL",
		"R8B", "R9B", "R10B", "R11B", "R12B", "R13B", "R14B", "R15B",

		"AH", "CH", "DH", "BH",

		"AX", "CX", "DX", "BX", "SP", "BP", "SI", "DI",
		"R8W", "R9W", "R10W", "R11W", "R12W", "R13W", "R14W", "R15W",

		"EAX", "ECX", "EDX", "EBX", "ESP", "EBP", "ESI", "EDI",
		"R8D", "R9D", "R10D", "R11D", "R12D", "R13D", "R14D", "R15D",

		"RAX", "RCX", "RDX", "RBX", "RSP", "RBP", "RSI", "RDI",
		"R8", "R9", "R10", "R11", "R12", "R13", "R14", "R15",

		"BND0", "BND1", "BND2", "BND3",

		"ES", "CS", "SS", "DS", "FS", "GS",

		"ST0", "ST1", "ST2", "ST3", "ST4", "ST5", "ST6", "ST7",

		"MM0", "MM1", "MM2", "MM3", "MM4", "MM5", "MM6", "MM7",

		"XMM0", "XMM1", "XMM2", "XMM3", "XMM4", "XMM5", "XMM6", "XMM7",
		"XMM8", "XMM9", "XMM10", "XMM11", "XMM12", "XMM13", "XMM14", "XMM15",
		"XMM16", "XMM17", "XMM18", "XMM19", "XMM20", "XMM21", "XMM22", "XMM23",
		"XMM24", "XMM25", "XMM26", "XMM27", "XMM28", "XMM29", "XMM30", "XMM31",

		"YMM0", "YMM1", "YMM2", "YMM3", "YMM4", "YMM5", "YMM6", "YMM7",
		"YMM8", "YMM9", "YMM10", "YMM11", "YMM12", "YMM13", "YMM14", "YMM15",
		"YMM16", "YMM17", "YMM18", "YMM19", "YMM20", "YMM21", "YMM22", "YMM23",
		"YMM24", "YMM25", "YMM26", "YMM27", "YMM28", "YMM29", "YMM30", "YMM31",

		"ZMM0", "ZMM1", "ZMM2", "ZMM3", "ZMM4", "ZMM5", "ZMM6", "ZMM7",
		"ZMM8", "ZMM9", "ZMM10", "ZMM11", "ZMM12", "ZMM13", "ZMM14", "ZMM15",
		"ZMM16", "ZMM17", "ZMM18", "ZMM19", "ZMM20", "ZMM21", "ZMM22", "ZMM23",
		"ZMM24", "ZMM25", "ZMM26", "ZMM27", "ZMM28", "ZMM29", "ZMM30", "ZMM31",

		"TMM0", "TMM1", "TMM2", "TMM3", "TMM4", "TMM5", "TMM6", "TMM7",

		"CR0", "CR1", "CR2", "CR3", "CR4", "CR5", "CR6", "CR7",
		"CR8",

		"DR0", "DR1", "DR2", "DR3", "DR4", "DR5", "DR6", "DR7",

		"k0", "k1", "k2", "k3", "k4", "k5", "k6", "k7",

		"RIP",
		"None",
	};

	enum class RegClass : uint8_t
	{
		None,
		GenReg_8L,
		GenReg_8H,
		GenReg_16,
		GenReg_32,
		GenReg_64,
		ST,
		MMX,
		XMM,
		YMM,
		ZMM,
		TMM,
		BND,
		MASK,
		CR,
		DR,
		Segment,
		RIP,
	};


	RegClass GetParamRegClass(const ResultData& Results, const ParamData& Param)
	{
		switch (Param.Type)
		{
			case ParamType::GenReg:
			{
				int Size = GetParamSize(Param.Size, Results);

				if (Size == 1)
					return RegClass::GenReg_8L;
				else if (Size == 2)
					return RegClass::GenReg_16;
				else if (Size == 4)
					return RegClass::GenReg_32;
				else //if (Size == 8)
					return RegClass::GenReg_64;
			}

			case ParamType::ControlReg:
				return RegClass::CR;

			case ParamType::DebugReg:
				return RegClass::DR;

			case ParamType::BoundReg:
				return RegClass::BND;

			case ParamType::SegmentReg:
				return RegClass::Segment;

			case ParamType::ST0:
			case ParamType::X87Reg:
				return RegClass::ST;

			case ParamType::MMXReg:
				return RegClass::MMX;

			case ParamType::XMMReg:
				return RegClass::XMM;

			case ParamType::YMMReg:
				return RegClass::YMM;

			case ParamType::ZMMReg:
				return RegClass::ZMM;

			case ParamType::MaskReg:
				return RegClass::MASK;

			case ParamType::TileReg:
				return RegClass::TMM;
		}

		if ((Param.Type & (ParamType)0xF0) == ParamType::AX)
			return RegClass::GenReg_16;

		else if ((Param.Type & (ParamType)0xF0) == ParamType::EAX)
			return RegClass::GenReg_32;

		else if ((Param.Type & (ParamType)0xF0) == ParamType::RAX)
			return RegClass::GenReg_64;

		else if ((Param.Type & (ParamType)0xF0) == ParamType::ES)
			return RegClass::Segment;

		else if ((Param.Type & (ParamType)0xF0) == ParamType::AL)
			return RegClass::GenReg_8L;

		else if ((Param.Type & (ParamType)0xF0) == ParamType::oAX)
		{
			if (Results.OpSize == 16)
				return RegClass::GenReg_16;
			else if (Results.OpSize == 32)
				return RegClass::GenReg_32;
			else if (Results.OpSize == 64)
				return RegClass::GenReg_64;
		}

		return RegClass::None;
	}

	Register GetRegister(const ResultData& Results, const ParamData& Param, int Reg)
	{
		switch (Param.Type)
		{
			case ParamType::GenReg:
			{
				int Size = GetParamSize(Param.Size, Results);

				if (Size == 1)
				{
					if (Results.Rex.Rex == 0 && Reg >= 4)
					{
						// Unless we have a rex prefix, we need to Covert 4-7 from the 
						// Low 8-bit registers, to the High 8-bit registers
						return Register::AH + (Register)(Reg - 4);
					}

					return Register::AL + (Register)(Reg);
				}

				else if (Size == 2)
					return Register::AX + (Register)(Reg);
				else if (Size == 4)
					return Register::EAX + (Register)(Reg);
				else //if (Size == 8)
					return Register::RAX + (Register)(Reg);
			}
			case ParamType::ControlReg:
				return Register::CR0 + (Register)(Reg);

			case ParamType::DebugReg:
				return Register::DR0 + (Register)(Reg);

			case ParamType::BoundReg:
				return Register::BND0 + (Register)(Reg);

			case ParamType::SegmentReg:
				return Register::ES + (Register)(Reg);

			case ParamType::ST0:
				return Register::ST0;

			case ParamType::X87Reg:
				return Register::ST0 + (Register)(Reg);

			case ParamType::MMXReg:
				return Register::MM0 + (Register)(Reg);

			case ParamType::XMMReg:
				return Register::XMM0 + (Register)(Reg);

			case ParamType::YMMReg:
				return Register::YMM0 + (Register)(Reg);

			case ParamType::ZMMReg:
				return Register::ZMM0 + (Register)(Reg);

			case ParamType::MaskReg:
				return Register::k0 + (Register)(Reg);

			case ParamType::TileReg:
				return Register::TMM0 + (Register)(Reg);
			}
	
		if ((Param.Type & (ParamType)0xF0) == ParamType::AX)
			return Register::AX + (Register)(Reg);

		else if ((Param.Type & (ParamType)0xF0) == ParamType::EAX)
			return Register::EAX + (Register)(Reg);

		else if ((Param.Type & (ParamType)0xF0) == ParamType::RAX)
			return Register::RAX + (Register)(Reg);

		else if ((Param.Type & (ParamType)0xF0) == ParamType::ES)
			return Register::ES + (Register)(Reg);

		else if ((Param.Type & (ParamType)0xF0) == ParamType::AL)
			return Register::AL + (Register)(Reg);

		else if ((Param.Type & (ParamType)0xF0) == ParamType::oAX)
		{
			if (Results.OpSize == 16)
				return Register::AX + (Register)(Reg);
			else if (Results.OpSize == 32)
				return Register::EAX + (Register)(Reg);
			else if (Results.OpSize == 64)
				return Register::RAX + (Register)(Reg);
		}

		return Register::None;
	}

	Register GetRegisterName(const ResultData& Results, const ParamData& Param, int Reg)
	{
		switch (GetParamRegClass(Results, Param))
		{
			case RegClass::GenReg_8H:
				return Register::AH + (Register)(Reg);

			case RegClass::GenReg_8L:
			{
				if (Results.Rex.Rex == 0 && Reg >= 4)
				{
					// Unless we have a rex prefix, we need to Covert 4-7 from the 
					// Low 8-bit registers, to the High 8-bit registers
					return Register::AH + (Register)(Reg - 4);
				}

				return Register::AL + (Register)(Reg);
			}

			case RegClass::GenReg_16:
				return Register::AX + (Register)(Reg);

			case RegClass::GenReg_32:
				return Register::EAX + (Register)(Reg);

			case RegClass::GenReg_64:
				return Register::RAX + (Register)(Reg);

			case RegClass::Segment:
				return Register::ES + (Register)(Reg);

			case RegClass::BND:
				return Register::BND0 + (Register)(Reg);

			case RegClass::ST:
				return Register::ST0 + (Register)(Reg);

			case RegClass::MMX:
				return Register::MM0 + (Register)(Reg);

			case RegClass::XMM:
				return Register::XMM0 + (Register)(Reg);

			case RegClass::YMM:
				return Register::YMM0 + (Register)(Reg);

			case RegClass::ZMM:
				return Register::ZMM0 + (Register)(Reg);

			case RegClass::TMM:
				return Register::TMM0 + (Register)(Reg);

			case RegClass::CR:
				return Register::CR0 + (Register)(Reg);

			case RegClass::DR:
				return Register::DR0 + (Register)(Reg);

			case RegClass::MASK:
				return Register::k0 + (Register)(Reg);

			case RegClass::RIP:
				return Register::RIP;
		}

		return Register::None;
	}

	Register ModRM16Bit[8][2]
	{
		{Register::BX, Register::SI},	// BX + SI
		{Register::BX, Register::DI},	// BX + DI
		{Register::BP, Register::SI},	// BP + SI
		{Register::BP, Register::DI},	// BP + DI
		{Register::SI, Register::None},	// SI
		{Register::DI, Register::None},	// DI
		{Register::BP, Register::None},	// BP
		{Register::BX, Register::None},	// BX
	};

	ParamInformation GetModRMParam(const ResultData& Results, const ParamData& Param)
	{
		ParamInformation ParamResult;
		ParamResult.Base = Register::None;
		ParamResult.Index = Register::None;
		ParamResult.Scale = 1;
		ParamResult.ValueType = ValueType::Address;
		ParamResult.Value = Results.Displacment;
		ParamResult.DataSize = GetParamSize(Param.Size, Results);

		if (Results.AddressSize == 16)
		{
			// Mod=0 RM=b110 is just displacment
			if (Results.ModRM.Mod == 00 && Results.ModRM.RM == 0b00000110)
			{
				ParamResult.Base = Register::None;
				ParamResult.Index = Register::None;
			}
			else
			{
				ParamResult.Base = ModRM16Bit[Results.ModRM.RM][0];
				ParamResult.Index = ModRM16Bit[Results.ModRM.RM][1];
			}
		}
		else
		{
			// SIB
			if (Results.ModRM.Mod != 3 && Results.ModRM.RM == 0b100)
			{
				ParamResult.Scale = 1 << Results.SIB.Scale;
				int Reg = Results.SIB.Base;
				if (Results.Mode == 64)
					Reg |= Results.Rex.B << 3;

				// Base b0101/b1101 and Mod == 0 Dosn't use the base register
				if (!(Results.ModRM.Mod == 0 && Results.SIB.Base == 0b101))
				{
					ParamResult.Base = GetRegister(Results, Param, Reg);
				}

				Reg = Results.SIB.Index;
				if (Results.Mode == 64)
					Reg |= Results.Rex.X << 3;

				// Index b0100 is no Index
				if (Reg != 0b0100)
				{
					if (HasFlag(Param.Flags, ParamFlags::INDEX_XMM))
					{
						// The 5th bit (V') is the high bit of the register when doing VSIB
						Reg |= Results.VexReg & 0x10;
						ParamResult.Index = Register::XMM0 + (Register)(Reg);
					}
					else if (HasFlag(Param.Flags, ParamFlags::INDEX_YMM))
					{
						Reg |= Results.VexReg & 0x10;
						ParamResult.Index = Register::YMM0 + (Register)(Reg);
					}
					else if (HasFlag(Param.Flags, ParamFlags::INDEX_ZMM))
					{
						Reg |= Results.VexReg & 0x10;
						ParamResult.Index = Register::ZMM0 + (Register)(Reg);
					}
					else
					{
						ParamResult.Index = GetRegister(Results, Param, Reg);
					}
				}
			}
			else
			{
				int Reg = Results.ModRM.RM;
				if (Results.Mode == 64)
					Reg |= Results.Rex.B << 3;

				// Mod 0, RM 101 (Rex.B is ignored) is Displacment only (or Rip displacment on 64-bit)
				if (Results.ModRM.Mod == 0 && Results.ModRM.RM == 0b101)
				{
					if (Results.Mode == 64)
					{
						ParamResult.Base = Register::RIP;
						ParamResult.ValueType = ValueType::RipRealative;
					}
					else
					{
						ParamResult.Base = Register::None;
					}
				}
				else
				{
					ParamResult.Base = GetRegister(Results, Param, Reg);
				}
			}
		}
		
		return ParamResult;
	}

	ParamInformation GetParam(const ResultData& Results, unsigned int ParamIndex)
	{
		ParamInformation ParamResult;
		ParamResult.Base = Register::None;
		ParamResult.Index = Register::None;
		ParamResult.Scale = 1;
		ParamResult.ValueType = ValueType::None;
		ParamResult.Value = 0;
		ParamResult.DataSize = 0;

		if (ParamIndex >= 5 || Results.OpcodeData->Params[ParamIndex] == 0)
			return ParamResult;

		const ParamData& Param = ParamList[Results.OpcodeData->Params[ParamIndex]];
		ParamResult.DataSize = GetParamSize(Param.Size, Results);

		int Reg = 0;

		switch (Param.Encoding)
		{
			case ParamEncoding::Implicit:
				if (Param.Type == ParamType::One)
				{
					ParamResult.Value = 1;
					ParamResult.ValueType = ValueType::Immidate;
				}
				else
				{
					Reg = (uint8_t)(Param.Type & (ParamType)0x0F);
					if (Results.Mode == 64 && !HasFlag(Param.Flags, ParamFlags::RexBImmune))
						Reg |= Results.Rex.R << 3;
				}
				break;

			case ParamEncoding::Immidate:
				if (Param.Type == ParamType::MOffset)
				{
					ParamResult.ValueType = ValueType::Address;

					if (Results.Mode == 64)
						ParamResult.Value = Results.Displacment;
					else
						ParamResult.Value = Results.Immidate.Word[0];
				}
				else if (Param.Size == ParamSize::FarPointer)
				{
					ParamResult.Value = ((int64_t)Results.Immidate.Word[1] << 32) | Results.Immidate.Word[0];
					ParamResult.ValueType = ValueType::FarPointer;

				}
				else if (Param.Type == ParamType::RelativeBranch)
				{
					ParamResult.Value = Results.Immidate.Word[0];
					ParamResult.ValueType = ValueType::Relative;
				}
				else
				{
					ParamResult.Value = Results.Immidate.Word[0];
					ParamResult.ValueType = ValueType::Immidate;

					for (unsigned int x = 0; x < ParamIndex; x++)
					{
						// If there is another immidate before this one, return the second value
						if (ParamList[Results.OpcodeData->Params[x]].Encoding == ParamEncoding::Immidate)
							ParamResult.Value = Results.Immidate.Word[1];
					}
				}
				break;

			case ParamEncoding::MODRM:
				// If it's ModRM and the Mod == 11 (Reg to Reg) , treat it as an RM
				if (Results.ModRM.Mod != 11)
					return GetModRMParam(Results, Param);

				Reg = Results.ModRM.RM;
				if (Results.Mode == 64 && !HasFlag(Param.Flags, ParamFlags::RexBImmune))
					Reg |= Results.Rex.R << 3;

				break;
		
			case ParamEncoding::RM:
				Reg = Results.ModRM.RM;
				if (Results.Mode == 64 && !HasFlag(Param.Flags, ParamFlags::RexBImmune))
					Reg |= Results.Rex.R << 3;

				break;

			case ParamEncoding::EVEX_RM:
				Reg = Results.ModRM.RM;
				Reg |= Results.Rex.B << 3;
				//if(Param.Type == ParamType::XMMReg || Param.Type == ParamType::YMMReg || Param.Type == ParamType::ZMMReg)
					Reg |= Results.Rex.X << 4;
				//else
				//	Reg |= Results.Rex.B4 << 4;

				break;

			case ParamEncoding::REG:
				Reg = Results.ModRM.Reg;
				if (Results.Mode == 64 && !HasFlag(Param.Flags, ParamFlags::RexBImmune))
					Reg |= Results.Rex.R << 3;

				break;

			case ParamEncoding::EVEX_Reg:
				Reg = Results.ModRM.Reg;
				Reg |= Results.Rex.B << 3;
				Reg |= Results.R2 << 4;
				break;

			case ParamEncoding::VEX:
				Reg = Results.VexReg & 0x0F;
				break;

			case ParamEncoding::EVEX:
				Reg = Results.VexReg & 0x1F;
				break;

			case ParamEncoding::EncImmidate:
				Reg = (Results.Data[Results.Length - 1] & 0xF0) >> 4;
				break;

			case ParamEncoding::Mask:
				Reg = Results.MaskReg;
				break;
		}

		ParamResult.Base = GetRegister(Results, Param, Reg);

		return ParamResult;
	}

	std::string ParamSizeString(ParamInformation& ParamInfo)
	{
		switch (ParamInfo.DataSize)
		{
			case 1:
				return "BYTE PTR";
			case 2:
				return "WORD PTR";
			case 4:
				return "DWORD PTR";
			case 8:
				return "QWORD PTR";
			case 10:
				return "TBYTE PTR";
			case 16:
				return "XMMWORD PTR";
			case 32:
				return "YMMWORD PTR";
			case 64:
				return "ZMMWORD PTR";
		}

		return "";
	}

	const char* GetRegisterName(Register Reg)
	{
		if (Reg > Register::None || Reg < (Register)0)
			Reg = Register::None;

		return RegNames[(int)Reg];
	}
	
	const char* GetPrefixName(uint8_t Prefix)
	{
		if (Prefix >= 0x40 && Prefix <= 0x4F)
			return "REX";

		switch (Prefix)
		{
			case PrefixOpcodes::AddressSize:
				return "ADSIZE";

			case PrefixOpcodes::OpcodeSize:
				return "OPSIZE";

			case PrefixOpcodes::VEX2:
				return "VEX2";

			case PrefixOpcodes::VEX3:
				return "VEX";

			case PrefixOpcodes::EVEX:
				return "EVEX";

			case PrefixOpcodes::XOP:
				return "XOP";

			case PrefixOpcodes::LOCK:
				return "LOCK";

			case PrefixOpcodes::REP:
				return "REP";

			case PrefixOpcodes::REPNE:
				return "REPNE";

			case PrefixOpcodes::CS:
				return "CS";

			case PrefixOpcodes::DS:
				return "DS";

			case PrefixOpcodes::ES:
				return "ES";

			case PrefixOpcodes::SS:
				return "SS";

			case PrefixOpcodes::FS:
				return "FS";

			case PrefixOpcodes::GS:
				return "GS";
		}

		return "";
	}

	bool IsRefiningPrefix(uint8_t Value)
	{
		if (Value == PrefixOpcodes::OpcodeSize || Value == PrefixOpcodes::REP || Value == PrefixOpcodes::REPNE)
			return true;

		return false;
	}

	std::string PrefixString(const ResultData& Results, bool NewLine)
	{
		std::string ResultString;

		for (int x = 0; x < Results.PrefixCount; x++)
		{
			std::string PrefixString = "";

			uint8_t Value = Results.Data[x];
			if (IsSegmentPrefix(Value))
			{
				// Only Display the override here if it isn't accessing memory
				bool PrintPrefix = true;
				for (int x = 0; x < 5; x++)
				{
					if ((ParamList[Results.OpcodeData->Params[x]].Encoding == ParamEncoding::MODRM && Results.ModRM.Mod != 3) ||
						ParamList[Results.OpcodeData->Params[x]].Type == ParamType::MOffset)
						PrintPrefix = false;
				}

				if (PrintPrefix)
				{
					PrefixString += GetPrefixName(Value);
				}
			}
			else if (IsRefiningPrefix(Value) && !HasFlag(Results.OpcodeData->Flags, OpcodeFlags::ExtendedPrefix))
			{
				if (Value == PrefixOpcodes::REPNE)
				{
					if (HasFlag(Results.OpcodeData->Flags, OpcodeFlags::BNDPrefix))
						PrefixString += "BND";

					else
						PrefixString += GetPrefixName(Value);
				}
				else if (Value == PrefixOpcodes::REP)
				{
					PrefixString += GetPrefixName(Value);
				}
			}
			else if (Value == PrefixOpcodes::VEX2 || Value == PrefixOpcodes::XOP)
			{
				x++;
			}
			else if (Value == PrefixOpcodes::VEX3)
			{
				x += 2;
			}
			else if (Value == PrefixOpcodes::EVEX)
			{
				x += 3;
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

	std::string OpcodeMemonicString(const ResultData& Results, bool Prefix)
	{
		std::string ResultString = "";

		if (Prefix)
		{
			PrefixString(Results);

			if (!ResultString.empty())
			{
				ResultString += " ";
			}
		}

		ResultString += Results.OpcodeData->Name;

		return ResultString;
	}

	std::string SegmentOverrideString(const ResultData& Results)
	{
		std::string Ret = "";
		if (Results.SegmentOverride)
		{
			Ret = GetPrefixName(Results.SegmentOverride);
			Ret += ":";
		}

		return Ret;
	}
}



