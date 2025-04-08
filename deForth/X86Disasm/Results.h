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
#include <string>

#include "ResultData.h"

namespace X86Disasm
{
	enum class Register
	{
		AL, CL, DL,	BL,	SPL, BPL, SIL, DIL,
		R8B, R9B, R10B, R11B, R12B, R13B, R14B, R15B,
		
		AH, CH, DH, BH,

		AX, CX, DX, BX, SP, BP, SI, DI,
        R8W, R9W, R10W, R11W, R12W, R13W, R14W, R15W,

		EAX, ECX, EDX, EBX, ESP, EBP, ESI, EDI,
		R8D, R9D, R10D, R11D, R12D, R13D, R14D, R15D,

		RAX, RCX, RDX, RBX, RSP, RBP, RSI, RDI,
		R8, R9, R10, R11, R12, R13, R14, R15,

		BND0, BND1,	BND2, BND3,

		ES, CS, SS, DS, FS, GS,

		ST0, ST1, ST2, ST3, ST4, ST5, ST6, ST7,

		MM0, MM1, MM2, MM3, MM4, MM5, MM6, MM7,

		XMM0, XMM1, XMM2, XMM3, XMM4, XMM5, XMM6, XMM7,
		XMM8, XMM9, XMM10, XMM11, XMM12, XMM13, XMM14, XMM15,
		XMM16, XMM17, XMM18, XMM19, XMM20, XMM21, XMM22, XMM23,
		XMM24, XMM25, XMM26, XMM27, XMM28, XMM29, XMM30, XMM31,

		YMM0, YMM1, YMM2, YMM3, YMM4, YMM5, YMM6, YMM7, 
		YMM8, YMM9, YMM10, YMM11, YMM12, YMM13, YMM14, YMM15, 
		YMM16, YMM17, YMM18, YMM19, YMM20, YMM21, YMM22, YMM23, 
		YMM24, YMM25, YMM26, YMM27, YMM28, YMM29, YMM30, YMM31,

		ZMM0, ZMM1, ZMM2, ZMM3, ZMM4, ZMM5, ZMM6, ZMM7, 
		ZMM8, ZMM9, ZMM10, ZMM11, ZMM12, ZMM13, ZMM14, ZMM15, 
		ZMM16, ZMM17, ZMM18, ZMM19, ZMM20, ZMM21, ZMM22, ZMM23, 
		ZMM24, ZMM25, ZMM26, ZMM27, ZMM28, ZMM29, ZMM30, ZMM31,

		TMM0, TMM1, TMM2, TMM3, TMM4, TMM5, TMM6, TMM7,

		CR0, CR1, CR2, CR3, CR4, CR5, CR6, CR7,
		CR8,

		DR0, DR1, DR2, DR3, DR4, DR5, DR6, DR7,

		k0, k1, k2, k3, k4, k5, k6, k7,

		RIP,
		None,
	};

	enum class ValueType
	{
		None,
		Address,
		Immidate,
		Relative,
		RipRealative,
		FarPointer,
	};

	struct ParamInformation
	{
		Register Base;		// Base Register
		Register Index;		// Index Register
		int Scale;				// Scale

		ValueType ValueType;	// The type of the value
		int64_t Value;			// Address, value, reltive or far pointer (The segment for the far pointer is always in the high DWORD)
		int DataSize;			// Size of the value in bytes 
	};


	// Returns the name of the prefix opcode 
	//  Prefix: The opcode value of the prefix
	const char* GetPrefixName(uint8_t Prefix);

	// Returss the name of a register
	//  Reg: The register name to get 
	const char* GetRegisterName(Register Reg);
	
	// Returns the bytes of the opcode, space seperated 
	//	Results: The data returned from ReadOpcode.
	//  Wrap: Wrap after five bytes 
	std::string OpcodeByteString(const ResultData& Results, bool Wrap = false);

	// Returns just the opcode prefixes for this opcode data
	//  NewLine: Add new lines between each prefix
	std::string PrefixString(const ResultData& Results, bool NewLine = false);

	// Returns the opcode Memonic, with possible prefix
	//	Results: The data returned from ReadOpcode.
	//  Prefix: Include the prefix with the Memonic (via PrefixString)
	std::string OpcodeMemonicString(const ResultData& Results, bool Prefix = true);

	// Returns the Segment Override string for the current opcode, if one is used
	//	Results: The data returned from ReadOpcode.
	std::string SegmentOverrideString(const ResultData& Results);

	// Returns the detailed information about a param
	//	Results: The data returned from ReadOpcode.
	//  Index: Which Param (0-4) to retrive the data for
	ParamInformation GetParam(const ResultData& Results, unsigned int ParamIndex);

	// Returns the Param size string for the given paramater ( "SIZE PRT" )
	//  ParamInfo: The param to get the string for 
	std::string ParamSizeString(ParamInformation& ParamInfo);

	// Returns the string value of a specific param
	//	Results: The data returned from ReadOpcode.
	//  ParamIndex: The param to get the value for
	//  Notes: Where to put Notes related to the params 
	//  Address: The base address for the params, used for displacment
	std::string ParamString(const ResultData& Results, int ParamIndex, std::string& Notes, intptr_t Address);

	// Returns the rounding flags for the opcode
	//	Results: The data returned from ReadOpcode.
	std::string ParamStringRounding(const ResultData& Results);

	// Returns just the opcodes string (Name\tparam, parma...)
	//	Results: The data returned from ReadOpcode.
	//  Address: The address to use for this opcode, this will also be used as the rIP pointer for relative displacements
	std::string OpcodeString(const ResultData& Results, intptr_t Address = 0);

	// Returns the string value of the params, (param, parma...)
	//	Results: The data returned from ReadOpcode.
	//  Notes: Where to put Notes related to the params 
	//  Address: The address to use for this opcode, this will also be used as the rIP pointer for relative displacements
	std::string FullParamString(const ResultData& Results, std::string& Notes, intptr_t Address = 0);

	// Creates a full tab based output string. Address\tBytes\tName\tParams\t; Remark
	//	Results: The data returned from ReadOpcode.
	//  Address: The address to use for this opcode, this will also be used as the rIP pointer for relative displacements
	//  Remark: An optional remark that can be added to the resulting output (in addition to any remark data that might be generated)
	std::string OpcodeTabbedString(const ResultData& Results, intptr_t Address = 0, const std::string& Remark = "");
}
