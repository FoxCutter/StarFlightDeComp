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

namespace X86Disasm
{
    enum class OpcodeSpace : uint8_t
    {
        Legacy = 0,
        VEX = 1,
        EVEX = 2,
        XOP = 3,
    };

    enum class RefiningPrefix : uint8_t
    {
        None = 0,
        OSZ = 1,		// 0x66
        REP = 2,		// 0xF3
        REPNE = 3,		// 0xF2
    };

    enum class ParamType : uint8_t
    {
        None,

        One,                // Implicit One
        Memory,             // Memory Access (with possible displacment)
        MOffset,            // Memory Offset
        Immidate,           // Immidate value
        RelativeBranch,     // Relative Branch

        GenReg,             // General Register
        ControlReg,         // Control Register
        DebugReg,           // Debug Register
        BoundReg,           // Bound register
        SegmentReg,         // Segment 

        X87Reg,             // x87 Reg
        MMXReg,             // MMX Reg
        VexReg,             // Vex Register
        XMMReg,             // XMM Reg
        YMMReg,             // YMM Reg
        ZMMReg,             // ZMM Reg
        MaskReg,            // Mask register
        TileReg,            // Tile Register

        AL = 0x20,
        CL,
        DL,
        BL,
        AH,
        CH,
        DH,
        BH,

        oAX = 0x30,                // AX/EAX/RAX depending on Opsize
        oCX,
        oDX,
        oBX,
        oSP,
        oBP,
        oSI,
        oDI,


        AX = 0x40,
        CX,
        DX,
            BX,
            SP,
            BP,
            SI,
            DI,

        EAX = 0x50,
        ECX,
        EDX,
            EBX,
            ESP,
            EBP,
            ESI,
            EDI,

        RAX = 0x60,
        RCX,
        RDX,
            RBX,
            RSP,
            RBP,
            RSI,
            RDI,

        ST0 = 0x70,

        ES = 0x80,
        CS,
        SS,
        DS,
        FS,
        GS,

    };

    enum class ParamEncoding : uint8_t
    {
        None,

        Immidate,       // Immidate data
        Implicit,       // Part of the opcode itself
        EncImmidate,    // (IMM8[7:4])

        REG,            // (MODRM.reg, REX.R MODRM.reg)
        RM,             // (MODRM.rm, REX.B, MODRM.rm)
        MODRM,          // (MODRM.mod modrm.reg)

        VEX,            // (VEX.vvvv)

        EVEX_Reg,       // (EVEX.R' REX.R MODRM.reg)
        EVEX_RM,        // (REX.X   REX.B MODRM.rm)
        EVEX,           // (EVEX.V' VEX.vvvv)
        Mask,           // (EVEX.aaa)
    };

    enum class ParamSize : uint8_t
    {
        None,

        Byte,                       // 8-bit Int
        Word,                       // 16-bit Int
        DoubleWord,                 // 32-bit int
        QuadWord,                   // 64-bit int

        DoubleQuadWord,             // 128-Bit
        QuadQuadWord,               // 256-Bit
        OctoQuadWord,               // 512-Bit

        Single,                     // 32-Bit Float
        Double,                     // 64-Bit Float
        LongDouble,                 // 80-Bit float

        ScalarSingle,               // 4 Bytes
        ScalarDouble,               // 8 Bytes
        PackedSingle,               // 16 Bytes
        PackedDouble,               // 16 Bytes

        MemData,                    // Address of a data structure in memory

        AdSize,                     // 16/32/64 based on Address Size 
        Opsize,                     // 16/32/64 based on Op Size 
        Opsize32,                   // 16/32/32 based on Op Size 
        Opsize64,                   // 32/32/64 based on Op Size 
        BroadcastSize,              // Memory size is based on the broadcast flag
        VexSize,                    // 128/256/512 depending on the VEX Size

        FarPointer,                 // Pointer, depends on OpSize + Word seg
    };

    enum class ParamFlags : uint8_t
    {
        None        = 0b00000000,

        INDEX_XMM   = 0b00000001,   // Index with XMM Register
        INDEX_YMM   = 0b00000010,   // Index with YMM Register
        INDEX_ZMM   = 0b00000100,   // Index with ZMM Register
        ZEROMASK    = 0b00001000,   // Zeroing Mask
        BROADCAST   = 0b00010000,   // Broadcast
        ROUNDING    = 0b00100000,   // Embeded Rounding Control
        SAE         = 0b01000000,   // SAE Support
        RexBImmune  = 0b10000000,   // Immune to RexB
    };

    enum class OpcodeFlags : uint32_t
    {
        None                = 0b00000000,

        NoModRM             = 0b00000001,   // Opcode dosn't have an ModRM byte
        FixedMod            = 0b00000010,   // Almost always Mod 11
        FixedReg            = 0b00000100,   // Usually for extended opcode tables
        FixedRM             = 0b00001000,   // Either a secondary byte, or to only support SIB
        SecondaryOpcode     = 0b00001110,   // Uses the full ModRM for the opcode encoding
        WIG                 = 0b00010000,   // Rex.W is ignored in VEX
        LIG                 = 0b00100000,   // {L}L is ignored in VEX
        BCRC                = 0b01000000,   // The BCRC flag is set for Rounding/SAE

        AddressSize         = 0b00000001'00000000,  // If the Address size effects the size of the opcode
        Default64           = 0b00000010'00000000,  // Defaults to 64-bit opsize in 64-bit mode, Rex.W is ignored
        Force64             = 0b00000100'00000000,  // 64-bit Opsize in 64-bit mode
        Mode64              = 0b00001000'00000000,  // 64-Bit mode only
        Not64               = 0b00010000'00000000,  // Opcode can not be used in 64-bit mode
        BroadcastAllowed    = 0b00100000'00000000,  // Can broadcast
        Conditional         = 0b01000000'00000000,  // Opcode is a Conditinal jump           
        Branch              = 0b10000000'00000000,  // Opcode is a Branch

        ExtendedPrefix      = 0b00000001'00000000'00000000,    // The opcode can be refined by a refining prefix
        ExtendedSize        = 0b00000010'00000000'00000000,    // The opcode can be refined by the OpSize
        BNDPrefix           = 0b00000100'00000000'00000000,    // The opcode can have a BND Prefix

        VIAOnly             = 0b00100000'00000000'00000000'00000000,    // Only on VIA Processors
        AMDOnly             = 0b01000000'00000000'00000000'00000000,    // Only on AMD Processors (XOP, 3DNow and a few other opcodes)           
    };


    enum class RoundingType
    {
        Nearest = 0,
        Down = 1,
        Up = 2,
        Zero = 3,
    };

    struct ParamData
    {
        ParamType		Type;
        ParamEncoding	Encoding;
        ParamSize		Size;
        ParamFlags		Flags;
    };
   
    // 00SS'MMMM-oooooooo-ssssssss-PPzz'LL0n
    //   SS = Space
    //		0 = Legacy
    //		1 = VEX
    //		2 = EVEX
    //		3 = XOP
    //   MMMM = Opcode Map : 0 = None 01 = 0x0f 02 = 0x0f38 03 = 0x0f3A, 04 = 3DNow, n = MAPn
    //   oooooooo = Opcode
    //   ssssssss = Secondary Byte
    //   PP = SIMD Prefix: 0 = None, 1 = 0x66, 2 = 0xF3 REP, 3 = 0xF2  REPNE
    //   zz = Size 0=any 1=16, 2=32, 3=64/WFLAG = 1
    //   LL = VEX/EVEX Size: 0 = any 1 = 128, 2 = 256, 3 = 512

    union CodeData
    {
        uint32_t Code;
        struct
        {
            uint32_t Reserved : 2;
            uint32_t VexSize : 2;
            uint32_t OpSize : 2;
            uint32_t Prefix : 2;

            uint32_t Secondary : 8;
            uint32_t Opcode : 8;

            uint32_t Map : 4;	// Opcode Map
            uint32_t Space : 2;	// Opcode Space (Legacy,VEX,EVEX,XOP)
            uint32_t Reserved2 : 2;	
        };
    };

    struct OpcodeData
    {
        uint32_t    Code;
        const char* Name;
        OpcodeFlags Flags;
        uint8_t     Disp8;
        uint8_t     BroadcastSize;
        uint8_t     Params[5];
    };


    extern const ParamData ParamList[];
    extern const OpcodeData OpcodeList[];
    extern const OpcodeData* const OpcodeListBegin;
    extern const OpcodeData* const OpcodeListEnd;
    extern const OpcodeData* const OpcodeBad;
}