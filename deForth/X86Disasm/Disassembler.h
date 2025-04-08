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

#include "ResultData.h"

namespace X86Disasm
{
	// Dissassambles the opcode pointed to by the buffer, using the passed in mode
	//   Buffer: A pointer readable memory, can be readonly
	//	 Mode: The CPU mode that the opcode is meant to be executing under. Can be:
	//		16: 16-Bit Real Mode/16-Bit Protected mode
	//		32: 32-Bit Protected mode, Compatibility Mode
	//		64:	64-Bit Mode
	
	// In all cases, all opcodes that are valid for the specified mode will be disassambled.
	// This includes VEX/AVX Encoded opcodes, AMD specific opcodes (including XOP and 3dNow) 
	// VIA opcodes, and Xenon opcodes
	// 
	// To dissasamble the next opcode, add ResultData.Length to the buffer to get the next address
	ResultData ReadOpcode(void* Buffer, int Mode = 32);
}