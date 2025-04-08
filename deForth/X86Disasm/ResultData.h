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
#include <stdint.h>

#include "X86OpcodeData.h"

namespace X86Disasm
{
	struct ResultData
	{
		const OpcodeData* OpcodeData;

		uint8_t Mode;					
		uint8_t OpSize;					
		uint8_t AddressSize;

		uint8_t Length;					// Length of the Opcode
		uint8_t PrefixCount;			// Total number of Prefix Bytes

		uint8_t SegmentOverride;		// Segment override (if provided) (Can also be Jcc hints)
		OpcodeSpace OpcodeSpace;		// Opcode space 
		RefiningPrefix Prefix;			// Refining Prefix 
		uint8_t Map;					// Opcode Map
		uint8_t Opcode;
		uint8_t SecondaryOpcode;

		union
		{
			struct
			{
				uint8_t B : 1;		// Extension of the ModR/M r/m field, SIB base field, or Opcode reg field
				uint8_t X : 1;		// Extension of the SIB index field
				uint8_t R : 1;		// Extension of the ModR/M reg field
				uint8_t W : 1;		// 64 Bit Operand Size

				uint8_t Rex : 4;	// 0b0100
			};
			uint8_t Prefix;
		} Rex;

		union
		{
			struct
			{
				uint8_t RM : 3;
				uint8_t Reg : 3;
				uint8_t Mod : 2;
			};
			uint8_t Value;
		} ModRM;

		union
		{
			struct
			{
				uint8_t Base : 3;
				uint8_t Index : 3;
				uint8_t Scale : 2;
			};
			uint8_t Value;
		} SIB;

		// VEX data
		uint16_t VexSize;
		uint8_t VexReg;

		// EVEX Data
		uint8_t MaskReg;
		uint8_t R2;					// Combine with EVEX.R and ModR/M.reg
		bool Zeroing;				// Zeroing/Merging
		bool BCRC;					// Broadcast/Rounding/SAE Context
		RoundingType Rounding;		// Rounding Type

		uint64_t Displacment;		// Displacment (or MOffset)

		union
		{
			uint64_t Full;
			int32_t Word[2];
		} Immidate;

		uint8_t Data[32] = { 0 };

		void Clear()
		{
			OpcodeData = OpcodeBad;

			Mode = 0;
			OpSize = 0;
			AddressSize = 0;

			Length = 0;
			PrefixCount = 0;
			OpcodeSpace = OpcodeSpace::Legacy;
			Prefix = RefiningPrefix::None;
			Map = 0;
			SegmentOverride = 0;
			Map = 0;
			Opcode = 0;
			SecondaryOpcode = 0;

			VexSize = 0;
			VexReg = 0;
			MaskReg = 0;

			Rex.Prefix = 0;
			ModRM.Value = 0;
			SIB.Value = 0;

			R2 = 0;
			Zeroing = false;
			BCRC = false;
			Rounding = RoundingType::Nearest;

			Displacment = 0;
			Immidate.Full = 0;

			Data[0] = 0;
		}
	};
}