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
	enum class DissasambleState
	{
		// Reading any prefix bytes
		Prefix,

		// VEX/EVEX/XOP Encoding
		Encoded,

		// Reading the opcode value
		Opcode,

		// Read the ModRM, SIB and Displacment
		ModRM,

		// Read any immediate values
		Immediate,

		Done = 0xFF,
	};

	const OpcodeData* FindOpcode(uint32_t Opcode)
	{
		OpcodeData Value;
		Value.Code = Opcode;

		auto Match = std::lower_bound(OpcodeListBegin, OpcodeListEnd, Value, [](const OpcodeData& rhs, const OpcodeData& lhs)
			{
				return rhs.Code < lhs.Code;
			});

		return Match;
	}

	std::pair<const OpcodeData*, const OpcodeData*> FindRange(uint32_t OpcodeStart, uint32_t OpcodeEnd, int Mode)
	{
		OpcodeData Value;
		Value.Code = OpcodeStart;

		auto Start = FindOpcode(OpcodeStart);
		auto End = FindOpcode(OpcodeEnd);

		if (Mode == 64)
		{
			while (Start != End)
			{
				// If we're 64 bit, Remove any Not64 entries from the start of the range
				if (!HasFlag(Start->Flags, OpcodeFlags::Not64))
					break;

				Start++;
			};
		}
		else
		{
			while (Start != End)
			{
				// If we're not 64 bit, Remove any Mode64 entries from the end of the range
				if (!HasFlag((End - 1)->Flags, OpcodeFlags::Mode64))
					break;

				End--;
			};
		}

		return std::make_pair(Start, End);
	}

	uint8_t ReadNext(uint8_t* Buffer, ResultData& Result)
	{
		uint8_t Data = Buffer[Result.Length];
		Result.Data[Result.Length] = Data;
		Result.Length++;

		return Data;
	}
	
	DissasambleState ReadPrefix(uint8_t* Buffer, ResultData& Result)
	{
		uint8_t Data = Buffer[Result.Length];

		if (Result.Mode == 64 && Data >= 0x40 && Data <= 0x4F)
		{
			ReadNext(Buffer, Result);

			Result.PrefixCount++;
			Result.Rex.Prefix = Data;

			if (Result.Rex.W)
				Result.OpSize = 64;
		}
		else if (Data == PrefixOpcodes::AddressSize)
		{
			ReadNext(Buffer, Result);

			Result.PrefixCount++;

			if (Result.Mode == 64)
				Result.AddressSize = 32;
			else
				Result.AddressSize = Result.Mode == 16 ? 32 : 16;
		}
		else if (Data == PrefixOpcodes::OpcodeSize)
		{
			ReadNext(Buffer, Result);
			Result.PrefixCount++;

			// Save the prefix for later lookup
			Result.Prefix = RefiningPrefix::OSZ;

			if (Result.Mode == 64)
				Result.OpSize = 16;
			else
				Result.OpSize = Result.Mode == 16 ? 32 : 16;
		}
		else if (Data == PrefixOpcodes::REP || Data == PrefixOpcodes::REPNE)
		{
			ReadNext(Buffer, Result);

			Result.PrefixCount++;

			// Save the prefix for later lookup
			Result.Prefix = (Data == PrefixOpcodes::REP) ? RefiningPrefix::REP : RefiningPrefix::REPNE;
		}
		else if (IsSegmentPrefix(Data))
		{
			// Segment overrides, save it and stay in the same state
			ReadNext(Buffer, Result);

			Result.SegmentOverride = Data;
			Result.PrefixCount++;
		}
		else if (Data == PrefixOpcodes::VEX2 || Data == PrefixOpcodes::VEX3 || Data == PrefixOpcodes::EVEX)
		{
			if (Result.Mode == 64)
			{
				// In 32-Bit mode the VEX/EVEX prefixs are also valid Opcodes, but are only prefixes in 64-bit mode
				ReadNext(Buffer, Result);
				Result.PrefixCount++;

				Result.OpcodeSpace = (Data == PrefixOpcodes::EVEX) ? OpcodeSpace::EVEX : OpcodeSpace::VEX;
				return DissasambleState::Encoded;
			}
			else
			{
				uint8_t Data2 = Buffer[Result.Length + 1];

				// Peak at the next byte to see that Mod = 0b11
				if ((Data2 & 0b11000000) == 0b11000000)
				{
					// We have an encoded opcode
					ReadNext(Buffer, Result);
					Result.PrefixCount++;

					Result.OpcodeSpace = (Data == PrefixOpcodes::EVEX) ? OpcodeSpace::EVEX : OpcodeSpace::VEX;
					return DissasambleState::Encoded;
				}
				else
				{
					// Just a normal opcode
					return DissasambleState::Opcode;
				}
			}
		}
		else if (Data == PrefixOpcodes::XOP)
		{
			uint8_t Data2 = Buffer[Result.Length + 1];

			// For XOP the map has to be 8 or above to be valid (only 8, 9 & A are used)
			if ((Data2 & 0b00011111) >= 0x8)
			{
				// We have an encoded opcode
				ReadNext(Buffer, Result);
				Result.PrefixCount++;

				Result.OpcodeSpace = OpcodeSpace::XOP;
				return DissasambleState::Encoded;
			}
			else
			{
				// Just a normal opcode
				return DissasambleState::Opcode;
			}
		}
		else if (Data == PrefixOpcodes::Map01)
		{	
			ReadNext(Buffer, Result);
			Result.PrefixCount++;
			Result.Map = 1;

			Data = Buffer[Result.Length];

			if (Data == PrefixOpcodes::Map02)
			{
				Result.Map = 2;
				ReadNext(Buffer, Result);
				Result.PrefixCount++;
			}
			else if (Data == PrefixOpcodes::Map03)
			{
				Result.Map = 3;
				ReadNext(Buffer, Result);
				Result.PrefixCount++;
			}
			else if (Data == PrefixOpcodes::Map04)
			{
				Result.Map = 4;
				ReadNext(Buffer, Result);
				Result.PrefixCount++;

				// 3DNow! is different as the inital encoding of 0x0f 0x0f is followed by the MOD/RM byte, any displacement data, then the opcode
				return DissasambleState::ModRM;
			}
		}
		else
		{
			return DissasambleState::Opcode;
		}

		// Stay in the prefix state
		return DissasambleState::Prefix;
	}

	DissasambleState ReadEncoding(uint8_t* Buffer, ResultData& Result)
	{
		uint8_t Prefix = Buffer[Result.Length - 1];
		uint8_t Data = ReadNext(Buffer, Result);
		Result.PrefixCount++;

		if (Prefix == PrefixOpcodes::VEX2)
		{
			Result.Rex.Prefix = 0x00;
			Result.Rex.R = (Data & 0x80) == 0;							// R
			Result.VexReg = (Data & 0b01111000) >> 3;					// vvv
			Result.VexSize = (Data & 0b00000100) == 0 ? 128 : 256;		// L
			Result.Prefix = (RefiningPrefix) (Data & 0b00000011);		// pp
			Result.Map = 1;

			Result.VexReg = (~Result.VexReg) & 0x0F;

		}
		else if (Prefix == PrefixOpcodes::VEX3 || Prefix == PrefixOpcodes::XOP)
		{
			Result.Rex.R = (Data & 0x80) == 0;							// R
			Result.Rex.X = (Data & 0x40) == 0;							// X
			Result.Rex.B = (Data & 0x20) == 0;							// B
			Result.Map = (Data & 0x1F);									// m-mmmm

			Data = ReadNext(Buffer, Result);
			Result.PrefixCount++;

			Result.Rex.W = (Data & 0x80) != 0;							// W
			Result.VexReg = (Data & 0b01111000) >> 3;					// VVVV
			Result.VexSize = (Data & 0b00000100) == 0 ? 128 : 256;		// L
			Result.Prefix = (RefiningPrefix) (Data & 0b00000011);		// pp

			Result.VexReg = (~Result.VexReg) & 0x0F;					// Invert the VexReg
		}
		else if (Prefix == PrefixOpcodes::EVEX)
		{
			Result.Rex.R = (Data & 0x80) == 0;							// R
			Result.Rex.X = (Data & 0x40) == 0;							// X
			Result.Rex.B = (Data & 0x20) == 0;							// B
			Result.R2 = (Data & 0x10) == 0;								// R'
			Result.Map = (Data & 0x0F);									// mmmm

			Data = ReadNext(Buffer, Result);
			Result.PrefixCount++;

			Result.Rex.W = (Data & 0x80) != 0;							// W
			Result.VexReg = (Data & 0b01111000) >> 3;					// VVVV
			Result.Prefix = (RefiningPrefix) (Data & 0b00000011);		// pp

			Data = ReadNext(Buffer, Result);
			Result.PrefixCount++;

			Result.Zeroing = (Data & 0x80) != 0;						// z
			Result.BCRC = (Data & 0x10) != 0;							// b
			Result.VexReg |= (Data & 0b00001000) << 1;					// V'
			Result.MaskReg = (Data & 0b000000111);						// aaa

			Data = (Data & 0b01100000) >> 5;							// L'L
			if (Data == 0)
				Result.VexSize = 128;
			else if (Data == 1)
				Result.VexSize = 256;
			else if (Data == 2)
				Result.VexSize = 512;

			Result.Rounding = (RoundingType)Data;

			Result.VexReg = (~Result.VexReg) & 0x1F;					// Invert the VexReg
		}

		return DissasambleState::Opcode;
	}

	DissasambleState ReadOpcode(uint8_t* Buffer, ResultData& Result)
	{
		CodeData CodeGen;

		CodeGen.Code = 0;

		Result.Opcode = ReadNext(Buffer, Result);

		CodeGen.Space = (uint32_t) Result.OpcodeSpace;
		CodeGen.Map = Result.Map;
		CodeGen.Opcode = Result.Opcode;


		// Get the basic opcode range
		auto OpcodeRange = FindRange(CodeGen.Code, CodeGen.Code | 0xFFFF, Result.Mode);

		// If it's empty no match, just move on to the ModRM
		if (OpcodeRange.first == OpcodeRange.second)
		{
			Result.OpcodeData = OpcodeBad;
			return DissasambleState::ModRM;
		}

		if (Result.Mode == 64)
		{
			if (HasFlag(OpcodeRange.first->Flags, OpcodeFlags::Force64))
				Result.OpSize = 64;

			if (HasFlag(OpcodeRange.first->Flags, OpcodeFlags::Default64))
			{
				if (Result.OpSize == 32)
					Result.OpSize = 64;
			}
		}

		// Read in the parts of the ModRM that effect the opcode
		if (!HasFlag(OpcodeRange.first->Flags, OpcodeFlags::NoModRM))
		{
			uint8_t Next = Buffer[Result.Length];

			CodeGen.Secondary = 0;
			uint8_t SecondaryBase = 0;

			// Set the mod if it's not MemToReg
			if ((Next & 0xC0) == 0xC0)
				CodeGen.Secondary = 0xC0;

			auto RangeResult = FindRange(CodeGen.Code, CodeGen.Code | 0x3FFF, Result.Mode);
			if (RangeResult.first != RangeResult.second)
				OpcodeRange = RangeResult;

			else
				CodeGen.Secondary = 0;

			SecondaryBase = CodeGen.Secondary;

			if (HasFlag(OpcodeRange.first->Flags, OpcodeFlags::FixedReg))
			{
				CodeGen.Secondary |= Next & 0b00111000;

				auto RangeResult = FindRange(CodeGen.Code, CodeGen.Code | 0x07FF, Result.Mode);
				if (RangeResult.first != RangeResult.second)
					OpcodeRange = RangeResult;

				else
					CodeGen.Secondary = SecondaryBase;

				SecondaryBase = CodeGen.Secondary;
			}

			if (HasFlag(OpcodeRange.first->Flags, OpcodeFlags::FixedRM))
			{
				CodeGen.Secondary |= Next & 0b00000111;

				auto RangeResult = FindRange(CodeGen.Code, CodeGen.Code | 0x00FF, Result.Mode);
				if (RangeResult.first != RangeResult.second)
					OpcodeRange = RangeResult;

				else
					CodeGen.Secondary = SecondaryBase;
			}

			Result.SecondaryOpcode = CodeGen.Secondary;
		}

		if (Result.OpcodeSpace == OpcodeSpace::Legacy)
		{
			if (HasFlag(OpcodeRange.first->Flags, OpcodeFlags::ExtendedPrefix))
			{
				CodeGen.Prefix = (uint32_t) Result.Prefix;
				auto RangeResult = FindRange(CodeGen.Code, CodeGen.Code | 0x3F, Result.Mode);
				if (RangeResult.first != RangeResult.second)
					OpcodeRange = RangeResult;

				else
					CodeGen.Prefix = 0;
			}

			if (HasFlag(OpcodeRange.first->Flags, OpcodeFlags::ExtendedSize))
			{
				if (HasFlag(OpcodeRange.first->Flags, OpcodeFlags::AddressSize))
				{
					Result.OpSize = Result.AddressSize;
				}

				if (Result.OpSize == 16)
					CodeGen.OpSize = 1;
				else if (Result.OpSize == 32)
					CodeGen.OpSize = 2;
				else if (Result.OpSize == 64)
					CodeGen.OpSize = 3;

				auto RangeResult = FindRange(CodeGen.Code, CodeGen.Code | 0x0F, Result.Mode);
				if (RangeResult.first != RangeResult.second)
					OpcodeRange = RangeResult;

				else
					CodeGen.OpSize = 0;
			}
		}
		else
		{		
			CodeGen.Prefix = (uint32_t) Result.Prefix;
		
			// W Flag
			if (!HasFlag(OpcodeRange.first->Flags, OpcodeFlags::WIG))
			{
				if (Result.Rex.W)
					CodeGen.OpSize = 3;
			}

			auto RangeResult = FindRange(CodeGen.Code, CodeGen.Code | 0x0F, Result.Mode);
			if (RangeResult.first != RangeResult.second)
			{
				OpcodeRange = RangeResult;
			}
			else
			{
				CodeGen.OpSize = 0;
			}

			// Rounding/SAE 
			if (Result.BCRC)
			{
				// If the BCRC is set, but not for broadcast, then VexSize is always 512
				if (!HasFlag(OpcodeRange.first->Flags, OpcodeFlags::BroadcastAllowed) &&
					!HasFlag(OpcodeRange.first->Flags, OpcodeFlags::LIG))
				{
					Result.VexSize = 512;
				}
			}

			// Vex Size
			if (!HasFlag(OpcodeRange.first->Flags, OpcodeFlags::LIG))
			{
				if (Result.VexSize == 128)
					CodeGen.VexSize = 1;

				else if (Result.VexSize == 256)
					CodeGen.VexSize = 2;

				else if (Result.VexSize == 512)
					CodeGen.VexSize = 3;
			}

			RangeResult = FindRange(CodeGen.Code, CodeGen.Code | 0x03, Result.Mode);
			if (RangeResult.first != RangeResult.second)
			{
				OpcodeRange = RangeResult;
			}
			else
			{
				Result.OpcodeData = OpcodeBad;
				return DissasambleState::ModRM;
			}
		}

		// Make sure we found something
		if (OpcodeRange.first == OpcodeRange.second || OpcodeRange.first->Code != CodeGen.Code)
		{
			Result.OpcodeData = OpcodeBad;
			return DissasambleState::ModRM;
		}

		// Handle the case of 0x90 with Rex.B, which is actually XCHG oAX, or8
		if (Result.OpcodeSpace == OpcodeSpace::Legacy && CodeGen.Opcode == 0x90 && Result.Rex.B == 1)
			OpcodeRange.first++;

		if (OpcodeRange.second - OpcodeRange.first != 1)
		{
			Result.OpcodeData = OpcodeBad;
			return DissasambleState::ModRM;
		}

		Result.OpcodeData = OpcodeRange.first;

		if (HasFlag(OpcodeRange.first->Flags, OpcodeFlags::SecondaryOpcode))
		{
			Result.SecondaryOpcode = ReadNext(Buffer, Result);
		}

		if (HasFlag(OpcodeRange.first->Flags, OpcodeFlags::NoModRM) || HasFlag(OpcodeRange.first->Flags, OpcodeFlags::SecondaryOpcode))
		{
			return DissasambleState::Immediate;
		}

		// If this is 3DNow! the opcode comes last, so were done
		if(Result.Map == 4)
			return DissasambleState::Done;

		return DissasambleState::ModRM;
	}

	DissasambleState ReadModRM(uint8_t* Buffer, ResultData& Result)
	{
		Result.ModRM.Value = ReadNext(Buffer, Result);

		int DisplacmentSize = 0;

		if (Result.AddressSize == 16)
		{
			if (Result.ModRM.Mod == 00 && Result.ModRM.RM == 0b00000110)
				DisplacmentSize = 2;
			else if (Result.ModRM.Mod == 01)
				DisplacmentSize = 1;
			else if (Result.ModRM.Mod == 02)
				DisplacmentSize = 2;
		}
		else
		{
			if (Result.ModRM.Mod != 3 && Result.ModRM.RM == 0b00000100)
			{
				Result.SIB.Value = ReadNext(Buffer, Result);

				if (Result.ModRM.Mod == 0 && Result.SIB.Base == 5)
					DisplacmentSize = 4;
			}
			if (DisplacmentSize == 0)
			{
				if (Result.ModRM.Mod == 00 && Result.ModRM.RM == 0b00000101)
					DisplacmentSize = 4;
				else if (Result.ModRM.Mod == 01)
					DisplacmentSize = 1;
				else if (Result.ModRM.Mod == 02)
					DisplacmentSize = 4;
			}
		}

		if (DisplacmentSize != 0)
		{
			if (DisplacmentSize == 1)
			{
				Result.Displacment = *((int8_t*)&Buffer[Result.Length]);

				if (Result.OpcodeSpace == OpcodeSpace::EVEX)
				{
					// Tulp calculations
					if (HasFlag(Result.OpcodeData->Flags, OpcodeFlags::BroadcastAllowed) && Result.BCRC)
					{
						if (Result.OpcodeData->BroadcastSize != 0)
							Result.Displacment *= Result.OpcodeData->BroadcastSize;
					}
					else if (Result.OpcodeData->Disp8 != 0)
						Result.Displacment *= Result.OpcodeData->Disp8;
				}
			}

			else if (DisplacmentSize == 2)
				Result.Displacment = *((int16_t*)&Buffer[Result.Length]);

			else if (DisplacmentSize == 4)
				Result.Displacment = *((int32_t*)&Buffer[Result.Length]);
			
			while (DisplacmentSize != 0)
			{
				ReadNext(Buffer, Result);
				DisplacmentSize--;
			}
		}

		return DissasambleState::Immediate;
	}

	DissasambleState ReadImmediate(uint8_t* Buffer, ResultData& Result)
	{
		int ImmidateSize = 0;

		if (Result.OpcodeData == OpcodeBad)
		{
			// Opcode map 3 and A both have an extra immediate byte, so if we don't know anything else about the opcode, we at least know that
			if (Result.Map == 0x03 || Result.Map == 0x0A)
				ImmidateSize = 1;
		}

		int ImmidatePos = 0;
		for (int x = 0; x < 5; x++)
		{
			const ParamData& Param = ParamList[Result.OpcodeData->Params[x]];

			if (Param.Encoding == ParamEncoding::Immidate && Param.Type == ParamType::MOffset)
			{
				if (Result.Mode == 64)
				{
					ImmidateSize = 8;

					Result.Displacment = *((int64_t*)&Buffer[Result.Length]);

					while (ImmidateSize != 0)
					{
						ReadNext(Buffer, Result);
						ImmidateSize--;
					}
				}
				else
				{
					ImmidateSize = Result.AddressSize / 8;
				}
			}
			else if (Param.Encoding == ParamEncoding::Immidate)
			{				
				ImmidateSize = GetParamSize(Param.Size, Result);
			}
			else if (Param.Encoding == ParamEncoding::EncImmidate)
			{
				ImmidateSize = 1;
			}
			
			if (ImmidateSize != 0)
			{
				// Split the segment out of a immidate far pointer, this is only used for
				// CALL FAR and JMP CALL, which arn't useable in mode64, so the address
				// part will never be bigger then 32-bit, so it's safe to split it like this.				

				if (Param.Size == ParamSize::FarPointer)
				{
					ImmidateSize -= 2;
					Result.Immidate.Word[1] = *((int16_t*)&Buffer[Result.Length + ImmidateSize]);
				}
				
				if (ImmidateSize == 1)
					Result.Immidate.Word[ImmidatePos] = *((int8_t*)&Buffer[Result.Length]);

				else if (ImmidateSize == 2)
					Result.Immidate.Word[ImmidatePos] = *((int16_t*)&Buffer[Result.Length]);

				else if (ImmidateSize == 4)
					Result.Immidate.Word[ImmidatePos] = *((int32_t*)&Buffer[Result.Length]);
				
				else if (ImmidateSize == 8)
					Result.Immidate.Full = *((int64_t*)&Buffer[Result.Length]);

				if (Param.Size == ParamSize::FarPointer)
					ImmidateSize += 2;

				while (ImmidateSize != 0)
				{
					ReadNext(Buffer, Result);
					ImmidateSize--;
				}

				ImmidatePos++;
			}

		}

		// If we're in 3DNow! We need to read the opcode last, otherwise we're done.
		if (Result.Map == 4)
			return DissasambleState::Opcode;

		return DissasambleState::Done;
	}

	ResultData ReadOpcode(void* RawBuffer, int Mode)
	{
		DissasambleState State = DissasambleState::Prefix;
		uint8_t* Buffer = reinterpret_cast<uint8_t*>(RawBuffer);

		ResultData Result;
		Result.Clear();

		Result.Mode = Mode;

		if (Mode == 64)
			Result.OpSize = 32;

		else
			Result.OpSize = Mode;

		Result.AddressSize = Mode;


		do
		{
			switch (State)
			{
				case DissasambleState::Prefix:
					State = ReadPrefix(Buffer, Result);
					break;

				case DissasambleState::Encoded:
					State = ReadEncoding(Buffer, Result);
					break;

				case DissasambleState::Opcode:
					State = ReadOpcode(Buffer, Result);
					break;

				case DissasambleState::ModRM:
					State = ReadModRM(Buffer, Result);
					break;

				case DissasambleState::Immediate:
					State = ReadImmediate(Buffer, Result);
					break;

				default:
					State = DissasambleState::Done;
			}

		} while (State != DissasambleState::Done);



		return Result;
	}
}