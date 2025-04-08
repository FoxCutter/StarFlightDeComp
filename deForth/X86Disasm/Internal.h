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

#pragma once
#include "X86Disasm.h"

#include "..\Enum.h"

namespace X86Disasm
{
	enum PrefixOpcodes : uint8_t
	{
		Map01 = 0x0F,	// 0x0F
		Map02 = 0x38,	// 0x0F 0x38
		Map03 = 0x3A,	// 0x0F 0x3A
		Map04 = 0x0F,	// 0x0F 0x0F

		CS = 0x2E,
		DS = 0x3E,
		ES = 0x26,
		SS = 0x36,
		FS = 0x64,
		GS = 0x65,

		REP = 0xF3,
		REPNE = 0xF2,

		VEX3 = 0xC4,
		VEX2 = 0xC5,
		EVEX = 0x62,
		XOP = 0x8F,

		LOCK = 0xF0,

		OpcodeSize = 0x66,
		AddressSize = 0x67,
	};

	inline bool IsSegmentPrefix(uint8_t Prefix)
	{
		if (Prefix == PrefixOpcodes::CS || Prefix == PrefixOpcodes::DS ||
			Prefix == PrefixOpcodes::ES || Prefix == PrefixOpcodes::SS ||
			Prefix == PrefixOpcodes::FS || Prefix == PrefixOpcodes::GS)
			return true;

		return false;
	}

	inline int GetParamSize(ParamSize Size, const ResultData& Results)
	{
		switch (Size)
		{
		case ParamSize::Byte:
			return 1;
		case ParamSize::Word:
			return 2;
		case ParamSize::DoubleWord:
			return 4;
		case ParamSize::QuadWord:
			return 8;

		case ParamSize::DoubleQuadWord:
			return 16;
		case ParamSize::QuadQuadWord:
			return 32;
		case ParamSize::OctoQuadWord:
			return 64;

		case ParamSize::Single:
			return 4;
		case ParamSize::Double:
			return 8;
		case ParamSize::LongDouble:
			return 10;

		case ParamSize::ScalarSingle:
			return 4;
		case ParamSize::ScalarDouble:
			return 8;
		case ParamSize::PackedSingle:
		case ParamSize::PackedDouble:
			return 16;

		case ParamSize::AdSize:
			return Results.AddressSize / 8;

		case ParamSize::Opsize:
			return Results.OpSize / 8;

		case ParamSize::Opsize32:
			return (Results.OpSize == 64 ? 32 : Results.OpSize) / 8;

		case ParamSize::Opsize64:
			return (Results.OpSize == 16 ? 32 : Results.OpSize) / 8;

		case ParamSize::BroadcastSize:
			if (Results.BCRC)
				return Results.OpcodeData->BroadcastSize;
			else
				return Results.OpcodeData->Disp8;

		case ParamSize::VexSize:
			return Results.VexSize / 8;

		case ParamSize::FarPointer:
			return 2 + (Results.OpSize / 8);
		}

		return 0;
	}
}