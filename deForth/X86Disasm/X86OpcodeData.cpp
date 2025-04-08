#include "X86OpcodeData.h"
#include "..\Enum.h"
#include <algorithm>

namespace X86Disasm {

const ParamData ParamList[] = {
  { ParamType::None,	ParamEncoding::None,	ParamSize::None,	ParamFlags::None }, // 0
  { ParamType::One,	ParamEncoding::Implicit,	ParamSize::Byte,	ParamFlags::None }, // 1
  { ParamType::Memory,	ParamEncoding::MODRM,	ParamSize::Byte,	ParamFlags::None }, // 2
  { ParamType::Memory,	ParamEncoding::MODRM,	ParamSize::Word,	ParamFlags::None }, // 3
  { ParamType::Memory,	ParamEncoding::MODRM,	ParamSize::MemData,	ParamFlags::None }, // 4
  { ParamType::Memory,	ParamEncoding::MODRM,	ParamSize::Opsize,	ParamFlags::None }, // 5
  { ParamType::Memory,	ParamEncoding::MODRM,	ParamSize::FarPointer,	ParamFlags::None }, // 6
  { ParamType::MOffset,	ParamEncoding::Immidate,	ParamSize::Byte,	ParamFlags::None }, // 7
  { ParamType::MOffset,	ParamEncoding::Immidate,	ParamSize::Opsize,	ParamFlags::None }, // 8
  { ParamType::Immidate,	ParamEncoding::Immidate,	ParamSize::Byte,	ParamFlags::None }, // 9
  { ParamType::Immidate,	ParamEncoding::Immidate,	ParamSize::Word,	ParamFlags::None }, // 10
  { ParamType::Immidate,	ParamEncoding::Immidate,	ParamSize::Opsize,	ParamFlags::None }, // 11
  { ParamType::Immidate,	ParamEncoding::Immidate,	ParamSize::Opsize32,	ParamFlags::None }, // 12
  { ParamType::Immidate,	ParamEncoding::Immidate,	ParamSize::FarPointer,	ParamFlags::None }, // 13
  { ParamType::RelativeBranch,	ParamEncoding::Immidate,	ParamSize::Byte,	ParamFlags::None }, // 14
  { ParamType::RelativeBranch,	ParamEncoding::Immidate,	ParamSize::Opsize32,	ParamFlags::None }, // 15
  { ParamType::GenReg,	ParamEncoding::REG,	ParamSize::Byte,	ParamFlags::None }, // 16
  { ParamType::GenReg,	ParamEncoding::RM,	ParamSize::Byte,	ParamFlags::None }, // 17
  { ParamType::GenReg,	ParamEncoding::RM,	ParamSize::Word,	ParamFlags::None }, // 18
  { ParamType::GenReg,	ParamEncoding::RM,	ParamSize::DoubleWord,	ParamFlags::None }, // 19
  { ParamType::GenReg,	ParamEncoding::RM,	ParamSize::QuadWord,	ParamFlags::None }, // 20
  { ParamType::GenReg,	ParamEncoding::REG,	ParamSize::Opsize,	ParamFlags::None }, // 21
  { ParamType::GenReg,	ParamEncoding::RM,	ParamSize::Opsize,	ParamFlags::None }, // 22
  { ParamType::GenReg,	ParamEncoding::REG,	ParamSize::Opsize32,	ParamFlags::None }, // 23
  { ParamType::ControlReg,	ParamEncoding::REG,	ParamSize::DoubleWord,	ParamFlags::None }, // 24
  { ParamType::DebugReg,	ParamEncoding::REG,	ParamSize::DoubleWord,	ParamFlags::None }, // 25
  { ParamType::SegmentReg,	ParamEncoding::REG,	ParamSize::Word,	ParamFlags::None }, // 26
  { ParamType::oAX,	ParamEncoding::Implicit,	ParamSize::Opsize,	ParamFlags::None }, // 27
  { ParamType::oAX,	ParamEncoding::Implicit,	ParamSize::Opsize,	ParamFlags::RexBImmune }, // 28
  { ParamType::oAX,	ParamEncoding::Implicit,	ParamSize::Opsize32,	ParamFlags::None }, // 29
  { ParamType::oCX,	ParamEncoding::Implicit,	ParamSize::Opsize,	ParamFlags::None }, // 30
  { ParamType::oDX,	ParamEncoding::Implicit,	ParamSize::Opsize,	ParamFlags::None }, // 31
  { ParamType::oBX,	ParamEncoding::Implicit,	ParamSize::Opsize,	ParamFlags::None }, // 32
  { ParamType::oSP,	ParamEncoding::Implicit,	ParamSize::Opsize,	ParamFlags::None }, // 33
  { ParamType::oBP,	ParamEncoding::Implicit,	ParamSize::Opsize,	ParamFlags::None }, // 34
  { ParamType::oSI,	ParamEncoding::Implicit,	ParamSize::Opsize,	ParamFlags::None }, // 35
  { ParamType::oDI,	ParamEncoding::Implicit,	ParamSize::Opsize,	ParamFlags::None }, // 36
  { ParamType::AL,	ParamEncoding::Implicit,	ParamSize::Byte,	ParamFlags::None }, // 37
  { ParamType::CL,	ParamEncoding::Implicit,	ParamSize::Byte,	ParamFlags::None }, // 38
  { ParamType::DL,	ParamEncoding::Implicit,	ParamSize::Byte,	ParamFlags::None }, // 39
  { ParamType::BL,	ParamEncoding::Implicit,	ParamSize::Byte,	ParamFlags::None }, // 40
  { ParamType::AH,	ParamEncoding::Implicit,	ParamSize::Byte,	ParamFlags::None }, // 41
  { ParamType::CH,	ParamEncoding::Implicit,	ParamSize::Byte,	ParamFlags::None }, // 42
  { ParamType::DH,	ParamEncoding::Implicit,	ParamSize::Byte,	ParamFlags::None }, // 43
  { ParamType::BH,	ParamEncoding::Implicit,	ParamSize::Byte,	ParamFlags::None }, // 44
  { ParamType::DX,	ParamEncoding::Implicit,	ParamSize::Word,	ParamFlags::None }, // 45
  { ParamType::CS,	ParamEncoding::Implicit,	ParamSize::Word,	ParamFlags::None }, // 46
  { ParamType::DS,	ParamEncoding::Implicit,	ParamSize::Word,	ParamFlags::None }, // 47
  { ParamType::SS,	ParamEncoding::Implicit,	ParamSize::Word,	ParamFlags::None }, // 48
  { ParamType::ES,	ParamEncoding::Implicit,	ParamSize::Word,	ParamFlags::None }, // 49
  { ParamType::GS,	ParamEncoding::Implicit,	ParamSize::Word,	ParamFlags::None }, // 50
  { ParamType::FS,	ParamEncoding::Implicit,	ParamSize::Word,	ParamFlags::None }, // 51
};


const OpcodeData OpcodeList[] = {
  { 0x0'0'00'00'0'0, "ADD", OpcodeFlags::None, 0, 0, { 2, 16, 0, 0, 0, }  }, // ADD Memory-Byte, GenReg-Byte, 
  { 0x0'0'00'C0'0'0, "ADD", OpcodeFlags::FixedMod, 0, 0, { 17, 16, 0, 0, 0, }  }, // ADD GenReg-Byte, GenReg-Byte, 
  { 0x0'0'01'00'0'0, "ADD", OpcodeFlags::None, 0, 0, { 5, 21, 0, 0, 0, }  }, // ADD Memory-Opsize, GenReg-Opsize, 
  { 0x0'0'01'C0'0'0, "ADD", OpcodeFlags::FixedMod, 0, 0, { 22, 21, 0, 0, 0, }  }, // ADD GenReg-Opsize, GenReg-Opsize, 
  { 0x0'0'02'00'0'0, "ADD", OpcodeFlags::None, 0, 0, { 16, 2, 0, 0, 0, }  }, // ADD GenReg-Byte, Memory-Byte, 
  { 0x0'0'02'C0'0'0, "ADD", OpcodeFlags::FixedMod, 0, 0, { 16, 17, 0, 0, 0, }  }, // ADD GenReg-Byte, GenReg-Byte, 
  { 0x0'0'03'00'0'0, "ADD", OpcodeFlags::None, 0, 0, { 21, 5, 0, 0, 0, }  }, // ADD GenReg-Opsize, Memory-Opsize, 
  { 0x0'0'03'C0'0'0, "ADD", OpcodeFlags::FixedMod, 0, 0, { 21, 22, 0, 0, 0, }  }, // ADD GenReg-Opsize, GenReg-Opsize, 
  { 0x0'0'04'00'0'0, "ADD", OpcodeFlags::NoModRM, 0, 0, { 37, 9, 0, 0, 0, }  }, // ADD AL-Byte, Immidate-Byte, 
  { 0x0'0'05'00'0'0, "ADD", OpcodeFlags::NoModRM, 0, 0, { 27, 12, 0, 0, 0, }  }, // ADD oAX-Opsize, Immidate-Opsize32, 
  { 0x0'0'06'00'0'0, "PUSH", OpcodeFlags::NoModRM | OpcodeFlags::Not64, 0, 0, { 49, 0, 0, 0, 0, }  }, // PUSH ES-Word, 
  { 0x0'0'07'00'0'0, "POP", OpcodeFlags::NoModRM | OpcodeFlags::Not64, 0, 0, { 49, 0, 0, 0, 0, }  }, // POP ES-Word, 
  { 0x0'0'08'00'0'0, "OR", OpcodeFlags::None, 0, 0, { 2, 16, 0, 0, 0, }  }, // OR Memory-Byte, GenReg-Byte, 
  { 0x0'0'08'C0'0'0, "OR", OpcodeFlags::FixedMod, 0, 0, { 17, 16, 0, 0, 0, }  }, // OR GenReg-Byte, GenReg-Byte, 
  { 0x0'0'09'00'0'0, "OR", OpcodeFlags::None, 0, 0, { 5, 21, 0, 0, 0, }  }, // OR Memory-Opsize, GenReg-Opsize, 
  { 0x0'0'09'C0'0'0, "OR", OpcodeFlags::FixedMod, 0, 0, { 22, 21, 0, 0, 0, }  }, // OR GenReg-Opsize, GenReg-Opsize, 
  { 0x0'0'0A'00'0'0, "OR", OpcodeFlags::None, 0, 0, { 16, 2, 0, 0, 0, }  }, // OR GenReg-Byte, Memory-Byte, 
  { 0x0'0'0A'C0'0'0, "OR", OpcodeFlags::FixedMod, 0, 0, { 16, 17, 0, 0, 0, }  }, // OR GenReg-Byte, GenReg-Byte, 
  { 0x0'0'0B'00'0'0, "OR", OpcodeFlags::None, 0, 0, { 21, 5, 0, 0, 0, }  }, // OR GenReg-Opsize, Memory-Opsize, 
  { 0x0'0'0B'C0'0'0, "OR", OpcodeFlags::FixedMod, 0, 0, { 21, 22, 0, 0, 0, }  }, // OR GenReg-Opsize, GenReg-Opsize, 
  { 0x0'0'0C'00'0'0, "OR", OpcodeFlags::NoModRM, 0, 0, { 37, 9, 0, 0, 0, }  }, // OR AL-Byte, Immidate-Byte, 
  { 0x0'0'0D'00'0'0, "OR", OpcodeFlags::NoModRM, 0, 0, { 27, 12, 0, 0, 0, }  }, // OR oAX-Opsize, Immidate-Opsize32, 
  { 0x0'0'0E'00'0'0, "PUSH", OpcodeFlags::NoModRM | OpcodeFlags::Not64, 0, 0, { 46, 0, 0, 0, 0, }  }, // PUSH CS-Word, 
  { 0x0'0'10'00'0'0, "ADC", OpcodeFlags::None, 0, 0, { 2, 16, 0, 0, 0, }  }, // ADC Memory-Byte, GenReg-Byte, 
  { 0x0'0'10'C0'0'0, "ADC", OpcodeFlags::FixedMod, 0, 0, { 17, 16, 0, 0, 0, }  }, // ADC GenReg-Byte, GenReg-Byte, 
  { 0x0'0'11'00'0'0, "ADC", OpcodeFlags::None, 0, 0, { 5, 21, 0, 0, 0, }  }, // ADC Memory-Opsize, GenReg-Opsize, 
  { 0x0'0'11'C0'0'0, "ADC", OpcodeFlags::FixedMod, 0, 0, { 22, 21, 0, 0, 0, }  }, // ADC GenReg-Opsize, GenReg-Opsize, 
  { 0x0'0'12'00'0'0, "ADC", OpcodeFlags::None, 0, 0, { 16, 2, 0, 0, 0, }  }, // ADC GenReg-Byte, Memory-Byte, 
  { 0x0'0'12'C0'0'0, "ADC", OpcodeFlags::FixedMod, 0, 0, { 16, 17, 0, 0, 0, }  }, // ADC GenReg-Byte, GenReg-Byte, 
  { 0x0'0'13'00'0'0, "ADC", OpcodeFlags::None, 0, 0, { 21, 5, 0, 0, 0, }  }, // ADC GenReg-Opsize, Memory-Opsize, 
  { 0x0'0'13'C0'0'0, "ADC", OpcodeFlags::FixedMod, 0, 0, { 21, 22, 0, 0, 0, }  }, // ADC GenReg-Opsize, GenReg-Opsize, 
  { 0x0'0'14'00'0'0, "ADC", OpcodeFlags::NoModRM, 0, 0, { 37, 9, 0, 0, 0, }  }, // ADC AL-Byte, Immidate-Byte, 
  { 0x0'0'15'00'0'0, "ADC", OpcodeFlags::NoModRM, 0, 0, { 27, 12, 0, 0, 0, }  }, // ADC oAX-Opsize, Immidate-Opsize32, 
  { 0x0'0'16'00'0'0, "PUSH", OpcodeFlags::NoModRM | OpcodeFlags::Not64, 0, 0, { 48, 0, 0, 0, 0, }  }, // PUSH SS-Word, 
  { 0x0'0'17'00'0'0, "POP", OpcodeFlags::NoModRM | OpcodeFlags::Not64, 0, 0, { 48, 0, 0, 0, 0, }  }, // POP SS-Word, 
  { 0x0'0'18'00'0'0, "SBB", OpcodeFlags::None, 0, 0, { 2, 16, 0, 0, 0, }  }, // SBB Memory-Byte, GenReg-Byte, 
  { 0x0'0'18'C0'0'0, "SBB", OpcodeFlags::FixedMod, 0, 0, { 17, 16, 0, 0, 0, }  }, // SBB GenReg-Byte, GenReg-Byte, 
  { 0x0'0'19'00'0'0, "SBB", OpcodeFlags::None, 0, 0, { 5, 21, 0, 0, 0, }  }, // SBB Memory-Opsize, GenReg-Opsize, 
  { 0x0'0'19'C0'0'0, "SBB", OpcodeFlags::FixedMod, 0, 0, { 22, 21, 0, 0, 0, }  }, // SBB GenReg-Opsize, GenReg-Opsize, 
  { 0x0'0'1A'00'0'0, "SBB", OpcodeFlags::None, 0, 0, { 16, 2, 0, 0, 0, }  }, // SBB GenReg-Byte, Memory-Byte, 
  { 0x0'0'1A'C0'0'0, "SBB", OpcodeFlags::FixedMod, 0, 0, { 16, 17, 0, 0, 0, }  }, // SBB GenReg-Byte, GenReg-Byte, 
  { 0x0'0'1B'00'0'0, "SBB", OpcodeFlags::None, 0, 0, { 21, 5, 0, 0, 0, }  }, // SBB GenReg-Opsize, Memory-Opsize, 
  { 0x0'0'1B'C0'0'0, "SBB", OpcodeFlags::FixedMod, 0, 0, { 21, 22, 0, 0, 0, }  }, // SBB GenReg-Opsize, GenReg-Opsize, 
  { 0x0'0'1C'00'0'0, "SBB", OpcodeFlags::NoModRM, 0, 0, { 37, 9, 0, 0, 0, }  }, // SBB AL-Byte, Immidate-Byte, 
  { 0x0'0'1D'00'0'0, "SBB", OpcodeFlags::NoModRM, 0, 0, { 27, 12, 0, 0, 0, }  }, // SBB oAX-Opsize, Immidate-Opsize32, 
  { 0x0'0'1E'00'0'0, "PUSH", OpcodeFlags::NoModRM | OpcodeFlags::Not64, 0, 0, { 47, 0, 0, 0, 0, }  }, // PUSH DS-Word, 
  { 0x0'0'1F'00'0'0, "POP", OpcodeFlags::NoModRM | OpcodeFlags::Not64, 0, 0, { 47, 0, 0, 0, 0, }  }, // POP DS-Word, 
  { 0x0'0'20'00'0'0, "AND", OpcodeFlags::None, 0, 0, { 2, 16, 0, 0, 0, }  }, // AND Memory-Byte, GenReg-Byte, 
  { 0x0'0'20'C0'0'0, "AND", OpcodeFlags::FixedMod, 0, 0, { 17, 16, 0, 0, 0, }  }, // AND GenReg-Byte, GenReg-Byte, 
  { 0x0'0'21'00'0'0, "AND", OpcodeFlags::None, 0, 0, { 5, 21, 0, 0, 0, }  }, // AND Memory-Opsize, GenReg-Opsize, 
  { 0x0'0'21'C0'0'0, "AND", OpcodeFlags::FixedMod, 0, 0, { 22, 21, 0, 0, 0, }  }, // AND GenReg-Opsize, GenReg-Opsize, 
  { 0x0'0'22'00'0'0, "AND", OpcodeFlags::None, 0, 0, { 16, 2, 0, 0, 0, }  }, // AND GenReg-Byte, Memory-Byte, 
  { 0x0'0'22'C0'0'0, "AND", OpcodeFlags::FixedMod, 0, 0, { 16, 17, 0, 0, 0, }  }, // AND GenReg-Byte, GenReg-Byte, 
  { 0x0'0'23'00'0'0, "AND", OpcodeFlags::None, 0, 0, { 21, 5, 0, 0, 0, }  }, // AND GenReg-Opsize, Memory-Opsize, 
  { 0x0'0'23'C0'0'0, "AND", OpcodeFlags::FixedMod, 0, 0, { 21, 22, 0, 0, 0, }  }, // AND GenReg-Opsize, GenReg-Opsize, 
  { 0x0'0'24'00'0'0, "AND", OpcodeFlags::NoModRM, 0, 0, { 37, 9, 0, 0, 0, }  }, // AND AL-Byte, Immidate-Byte, 
  { 0x0'0'25'00'0'0, "AND", OpcodeFlags::NoModRM, 0, 0, { 27, 12, 0, 0, 0, }  }, // AND oAX-Opsize, Immidate-Opsize32, 
  { 0x0'0'27'00'0'0, "DAA", OpcodeFlags::NoModRM | OpcodeFlags::Not64, 0, 0, { 0, 0, 0, 0, 0, }  }, // DAA 
  { 0x0'0'28'00'0'0, "SUB", OpcodeFlags::None, 0, 0, { 2, 16, 0, 0, 0, }  }, // SUB Memory-Byte, GenReg-Byte, 
  { 0x0'0'28'C0'0'0, "SUB", OpcodeFlags::FixedMod, 0, 0, { 17, 16, 0, 0, 0, }  }, // SUB GenReg-Byte, GenReg-Byte, 
  { 0x0'0'29'00'0'0, "SUB", OpcodeFlags::None, 0, 0, { 5, 21, 0, 0, 0, }  }, // SUB Memory-Opsize, GenReg-Opsize, 
  { 0x0'0'29'C0'0'0, "SUB", OpcodeFlags::FixedMod, 0, 0, { 22, 21, 0, 0, 0, }  }, // SUB GenReg-Opsize, GenReg-Opsize, 
  { 0x0'0'2A'00'0'0, "SUB", OpcodeFlags::None, 0, 0, { 16, 2, 0, 0, 0, }  }, // SUB GenReg-Byte, Memory-Byte, 
  { 0x0'0'2A'C0'0'0, "SUB", OpcodeFlags::FixedMod, 0, 0, { 16, 17, 0, 0, 0, }  }, // SUB GenReg-Byte, GenReg-Byte, 
  { 0x0'0'2B'00'0'0, "SUB", OpcodeFlags::None, 0, 0, { 21, 5, 0, 0, 0, }  }, // SUB GenReg-Opsize, Memory-Opsize, 
  { 0x0'0'2B'C0'0'0, "SUB", OpcodeFlags::FixedMod, 0, 0, { 21, 22, 0, 0, 0, }  }, // SUB GenReg-Opsize, GenReg-Opsize, 
  { 0x0'0'2C'00'0'0, "SUB", OpcodeFlags::NoModRM, 0, 0, { 37, 9, 0, 0, 0, }  }, // SUB AL-Byte, Immidate-Byte, 
  { 0x0'0'2D'00'0'0, "SUB", OpcodeFlags::NoModRM, 0, 0, { 27, 12, 0, 0, 0, }  }, // SUB oAX-Opsize, Immidate-Opsize32, 
  { 0x0'0'2F'00'0'0, "DAS", OpcodeFlags::NoModRM | OpcodeFlags::Not64, 0, 0, { 0, 0, 0, 0, 0, }  }, // DAS 
  { 0x0'0'30'00'0'0, "XOR", OpcodeFlags::None, 0, 0, { 2, 16, 0, 0, 0, }  }, // XOR Memory-Byte, GenReg-Byte, 
  { 0x0'0'30'C0'0'0, "XOR", OpcodeFlags::FixedMod, 0, 0, { 17, 16, 0, 0, 0, }  }, // XOR GenReg-Byte, GenReg-Byte, 
  { 0x0'0'31'00'0'0, "XOR", OpcodeFlags::None, 0, 0, { 5, 21, 0, 0, 0, }  }, // XOR Memory-Opsize, GenReg-Opsize, 
  { 0x0'0'31'C0'0'0, "XOR", OpcodeFlags::FixedMod, 0, 0, { 22, 21, 0, 0, 0, }  }, // XOR GenReg-Opsize, GenReg-Opsize, 
  { 0x0'0'32'00'0'0, "XOR", OpcodeFlags::None, 0, 0, { 16, 2, 0, 0, 0, }  }, // XOR GenReg-Byte, Memory-Byte, 
  { 0x0'0'32'C0'0'0, "XOR", OpcodeFlags::FixedMod, 0, 0, { 16, 17, 0, 0, 0, }  }, // XOR GenReg-Byte, GenReg-Byte, 
  { 0x0'0'33'00'0'0, "XOR", OpcodeFlags::None, 0, 0, { 21, 5, 0, 0, 0, }  }, // XOR GenReg-Opsize, Memory-Opsize, 
  { 0x0'0'33'C0'0'0, "XOR", OpcodeFlags::FixedMod, 0, 0, { 21, 22, 0, 0, 0, }  }, // XOR GenReg-Opsize, GenReg-Opsize, 
  { 0x0'0'34'00'0'0, "XOR", OpcodeFlags::NoModRM, 0, 0, { 37, 9, 0, 0, 0, }  }, // XOR AL-Byte, Immidate-Byte, 
  { 0x0'0'35'00'0'0, "XOR", OpcodeFlags::NoModRM, 0, 0, { 27, 12, 0, 0, 0, }  }, // XOR oAX-Opsize, Immidate-Opsize32, 
  { 0x0'0'37'00'0'0, "AAA", OpcodeFlags::NoModRM | OpcodeFlags::Not64, 0, 0, { 0, 0, 0, 0, 0, }  }, // AAA 
  { 0x0'0'38'00'0'0, "CMP", OpcodeFlags::None, 0, 0, { 2, 16, 0, 0, 0, }  }, // CMP Memory-Byte, GenReg-Byte, 
  { 0x0'0'38'C0'0'0, "CMP", OpcodeFlags::FixedMod, 0, 0, { 17, 16, 0, 0, 0, }  }, // CMP GenReg-Byte, GenReg-Byte, 
  { 0x0'0'39'00'0'0, "CMP", OpcodeFlags::None, 0, 0, { 5, 21, 0, 0, 0, }  }, // CMP Memory-Opsize, GenReg-Opsize, 
  { 0x0'0'39'C0'0'0, "CMP", OpcodeFlags::FixedMod, 0, 0, { 22, 21, 0, 0, 0, }  }, // CMP GenReg-Opsize, GenReg-Opsize, 
  { 0x0'0'3A'00'0'0, "CMP", OpcodeFlags::None, 0, 0, { 16, 2, 0, 0, 0, }  }, // CMP GenReg-Byte, Memory-Byte, 
  { 0x0'0'3A'C0'0'0, "CMP", OpcodeFlags::FixedMod, 0, 0, { 16, 17, 0, 0, 0, }  }, // CMP GenReg-Byte, GenReg-Byte, 
  { 0x0'0'3B'00'0'0, "CMP", OpcodeFlags::None, 0, 0, { 21, 5, 0, 0, 0, }  }, // CMP GenReg-Opsize, Memory-Opsize, 
  { 0x0'0'3B'C0'0'0, "CMP", OpcodeFlags::FixedMod, 0, 0, { 21, 22, 0, 0, 0, }  }, // CMP GenReg-Opsize, GenReg-Opsize, 
  { 0x0'0'3C'00'0'0, "CMP", OpcodeFlags::NoModRM, 0, 0, { 37, 9, 0, 0, 0, }  }, // CMP AL-Byte, Immidate-Byte, 
  { 0x0'0'3D'00'0'0, "CMP", OpcodeFlags::NoModRM, 0, 0, { 27, 12, 0, 0, 0, }  }, // CMP oAX-Opsize, Immidate-Opsize32, 
  { 0x0'0'3F'00'0'0, "AAS", OpcodeFlags::NoModRM | OpcodeFlags::Not64, 0, 0, { 0, 0, 0, 0, 0, }  }, // AAS 
  { 0x0'0'40'00'0'0, "INC", OpcodeFlags::NoModRM | OpcodeFlags::Not64, 0, 0, { 27, 0, 0, 0, 0, }  }, // INC oAX-Opsize, 
  { 0x0'0'41'00'0'0, "INC", OpcodeFlags::NoModRM | OpcodeFlags::Not64, 0, 0, { 30, 0, 0, 0, 0, }  }, // INC oCX-Opsize, 
  { 0x0'0'42'00'0'0, "INC", OpcodeFlags::NoModRM | OpcodeFlags::Not64, 0, 0, { 31, 0, 0, 0, 0, }  }, // INC oDX-Opsize, 
  { 0x0'0'43'00'0'0, "INC", OpcodeFlags::NoModRM | OpcodeFlags::Not64, 0, 0, { 32, 0, 0, 0, 0, }  }, // INC oBX-Opsize, 
  { 0x0'0'44'00'0'0, "INC", OpcodeFlags::NoModRM | OpcodeFlags::Not64, 0, 0, { 33, 0, 0, 0, 0, }  }, // INC oSP-Opsize, 
  { 0x0'0'45'00'0'0, "INC", OpcodeFlags::NoModRM | OpcodeFlags::Not64, 0, 0, { 34, 0, 0, 0, 0, }  }, // INC oBP-Opsize, 
  { 0x0'0'46'00'0'0, "INC", OpcodeFlags::NoModRM | OpcodeFlags::Not64, 0, 0, { 35, 0, 0, 0, 0, }  }, // INC oSI-Opsize, 
  { 0x0'0'47'00'0'0, "INC", OpcodeFlags::NoModRM | OpcodeFlags::Not64, 0, 0, { 36, 0, 0, 0, 0, }  }, // INC oDI-Opsize, 
  { 0x0'0'48'00'0'0, "DEC", OpcodeFlags::NoModRM | OpcodeFlags::Not64, 0, 0, { 27, 0, 0, 0, 0, }  }, // DEC oAX-Opsize, 
  { 0x0'0'49'00'0'0, "DEC", OpcodeFlags::NoModRM | OpcodeFlags::Not64, 0, 0, { 30, 0, 0, 0, 0, }  }, // DEC oCX-Opsize, 
  { 0x0'0'4A'00'0'0, "DEC", OpcodeFlags::NoModRM | OpcodeFlags::Not64, 0, 0, { 31, 0, 0, 0, 0, }  }, // DEC oDX-Opsize, 
  { 0x0'0'4B'00'0'0, "DEC", OpcodeFlags::NoModRM | OpcodeFlags::Not64, 0, 0, { 32, 0, 0, 0, 0, }  }, // DEC oBX-Opsize, 
  { 0x0'0'4C'00'0'0, "DEC", OpcodeFlags::NoModRM | OpcodeFlags::Not64, 0, 0, { 33, 0, 0, 0, 0, }  }, // DEC oSP-Opsize, 
  { 0x0'0'4D'00'0'0, "DEC", OpcodeFlags::NoModRM | OpcodeFlags::Not64, 0, 0, { 34, 0, 0, 0, 0, }  }, // DEC oBP-Opsize, 
  { 0x0'0'4E'00'0'0, "DEC", OpcodeFlags::NoModRM | OpcodeFlags::Not64, 0, 0, { 35, 0, 0, 0, 0, }  }, // DEC oSI-Opsize, 
  { 0x0'0'4F'00'0'0, "DEC", OpcodeFlags::NoModRM | OpcodeFlags::Not64, 0, 0, { 36, 0, 0, 0, 0, }  }, // DEC oDI-Opsize, 
  { 0x0'0'50'00'0'0, "PUSH", OpcodeFlags::NoModRM | OpcodeFlags::Default64, 0, 0, { 27, 0, 0, 0, 0, }  }, // PUSH oAX-Opsize, 
  { 0x0'0'51'00'0'0, "PUSH", OpcodeFlags::NoModRM | OpcodeFlags::Default64, 0, 0, { 30, 0, 0, 0, 0, }  }, // PUSH oCX-Opsize, 
  { 0x0'0'52'00'0'0, "PUSH", OpcodeFlags::NoModRM | OpcodeFlags::Default64, 0, 0, { 31, 0, 0, 0, 0, }  }, // PUSH oDX-Opsize, 
  { 0x0'0'53'00'0'0, "PUSH", OpcodeFlags::NoModRM | OpcodeFlags::Default64, 0, 0, { 32, 0, 0, 0, 0, }  }, // PUSH oBX-Opsize, 
  { 0x0'0'54'00'0'0, "PUSH", OpcodeFlags::NoModRM | OpcodeFlags::Default64, 0, 0, { 33, 0, 0, 0, 0, }  }, // PUSH oSP-Opsize, 
  { 0x0'0'55'00'0'0, "PUSH", OpcodeFlags::NoModRM | OpcodeFlags::Default64, 0, 0, { 34, 0, 0, 0, 0, }  }, // PUSH oBP-Opsize, 
  { 0x0'0'56'00'0'0, "PUSH", OpcodeFlags::NoModRM | OpcodeFlags::Default64, 0, 0, { 35, 0, 0, 0, 0, }  }, // PUSH oSI-Opsize, 
  { 0x0'0'57'00'0'0, "PUSH", OpcodeFlags::NoModRM | OpcodeFlags::Default64, 0, 0, { 36, 0, 0, 0, 0, }  }, // PUSH oDI-Opsize, 
  { 0x0'0'58'00'0'0, "POP", OpcodeFlags::NoModRM | OpcodeFlags::Default64, 0, 0, { 27, 0, 0, 0, 0, }  }, // POP oAX-Opsize, 
  { 0x0'0'59'00'0'0, "POP", OpcodeFlags::NoModRM | OpcodeFlags::Default64, 0, 0, { 30, 0, 0, 0, 0, }  }, // POP oCX-Opsize, 
  { 0x0'0'5A'00'0'0, "POP", OpcodeFlags::NoModRM | OpcodeFlags::Default64, 0, 0, { 31, 0, 0, 0, 0, }  }, // POP oDX-Opsize, 
  { 0x0'0'5B'00'0'0, "POP", OpcodeFlags::NoModRM | OpcodeFlags::Default64, 0, 0, { 32, 0, 0, 0, 0, }  }, // POP oBX-Opsize, 
  { 0x0'0'5C'00'0'0, "POP", OpcodeFlags::NoModRM | OpcodeFlags::Default64, 0, 0, { 33, 0, 0, 0, 0, }  }, // POP oSP-Opsize, 
  { 0x0'0'5D'00'0'0, "POP", OpcodeFlags::NoModRM | OpcodeFlags::Default64, 0, 0, { 34, 0, 0, 0, 0, }  }, // POP oBP-Opsize, 
  { 0x0'0'5E'00'0'0, "POP", OpcodeFlags::NoModRM | OpcodeFlags::Default64, 0, 0, { 35, 0, 0, 0, 0, }  }, // POP oSI-Opsize, 
  { 0x0'0'5F'00'0'0, "POP", OpcodeFlags::NoModRM | OpcodeFlags::Default64, 0, 0, { 36, 0, 0, 0, 0, }  }, // POP oDI-Opsize, 
  { 0x0'0'70'00'0'0, "JO", OpcodeFlags::NoModRM | OpcodeFlags::Force64 | OpcodeFlags::Conditional | OpcodeFlags::BNDPrefix, 0, 0, { 14, 0, 0, 0, 0, }  }, // JO RelativeBranch-Byte, 
  { 0x0'0'71'00'0'0, "JNO", OpcodeFlags::NoModRM | OpcodeFlags::Force64 | OpcodeFlags::Conditional | OpcodeFlags::BNDPrefix, 0, 0, { 14, 0, 0, 0, 0, }  }, // JNO RelativeBranch-Byte, 
  { 0x0'0'72'00'0'0, "JB", OpcodeFlags::NoModRM | OpcodeFlags::Force64 | OpcodeFlags::Conditional | OpcodeFlags::BNDPrefix, 0, 0, { 14, 0, 0, 0, 0, }  }, // JB RelativeBranch-Byte, 
  { 0x0'0'73'00'0'0, "JNB", OpcodeFlags::NoModRM | OpcodeFlags::Force64 | OpcodeFlags::Conditional | OpcodeFlags::BNDPrefix, 0, 0, { 14, 0, 0, 0, 0, }  }, // JNB RelativeBranch-Byte, 
  { 0x0'0'74'00'0'0, "JZ", OpcodeFlags::NoModRM | OpcodeFlags::Force64 | OpcodeFlags::Conditional | OpcodeFlags::BNDPrefix, 0, 0, { 14, 0, 0, 0, 0, }  }, // JZ RelativeBranch-Byte, 
  { 0x0'0'75'00'0'0, "JNZ", OpcodeFlags::NoModRM | OpcodeFlags::Force64 | OpcodeFlags::Conditional | OpcodeFlags::BNDPrefix, 0, 0, { 14, 0, 0, 0, 0, }  }, // JNZ RelativeBranch-Byte, 
  { 0x0'0'76'00'0'0, "JBE", OpcodeFlags::NoModRM | OpcodeFlags::Force64 | OpcodeFlags::Conditional | OpcodeFlags::BNDPrefix, 0, 0, { 14, 0, 0, 0, 0, }  }, // JBE RelativeBranch-Byte, 
  { 0x0'0'77'00'0'0, "JNBE", OpcodeFlags::NoModRM | OpcodeFlags::Force64 | OpcodeFlags::Conditional | OpcodeFlags::BNDPrefix, 0, 0, { 14, 0, 0, 0, 0, }  }, // JNBE RelativeBranch-Byte, 
  { 0x0'0'78'00'0'0, "JS", OpcodeFlags::NoModRM | OpcodeFlags::Force64 | OpcodeFlags::Conditional | OpcodeFlags::BNDPrefix, 0, 0, { 14, 0, 0, 0, 0, }  }, // JS RelativeBranch-Byte, 
  { 0x0'0'79'00'0'0, "JNS", OpcodeFlags::NoModRM | OpcodeFlags::Force64 | OpcodeFlags::Conditional | OpcodeFlags::BNDPrefix, 0, 0, { 14, 0, 0, 0, 0, }  }, // JNS RelativeBranch-Byte, 
  { 0x0'0'7A'00'0'0, "JP", OpcodeFlags::NoModRM | OpcodeFlags::Force64 | OpcodeFlags::Conditional | OpcodeFlags::BNDPrefix, 0, 0, { 14, 0, 0, 0, 0, }  }, // JP RelativeBranch-Byte, 
  { 0x0'0'7B'00'0'0, "JNP", OpcodeFlags::NoModRM | OpcodeFlags::Force64 | OpcodeFlags::Conditional | OpcodeFlags::BNDPrefix, 0, 0, { 14, 0, 0, 0, 0, }  }, // JNP RelativeBranch-Byte, 
  { 0x0'0'7C'00'0'0, "JL", OpcodeFlags::NoModRM | OpcodeFlags::Force64 | OpcodeFlags::Conditional | OpcodeFlags::BNDPrefix, 0, 0, { 14, 0, 0, 0, 0, }  }, // JL RelativeBranch-Byte, 
  { 0x0'0'7D'00'0'0, "JNL", OpcodeFlags::NoModRM | OpcodeFlags::Force64 | OpcodeFlags::Conditional | OpcodeFlags::BNDPrefix, 0, 0, { 14, 0, 0, 0, 0, }  }, // JNL RelativeBranch-Byte, 
  { 0x0'0'7E'00'0'0, "JLE", OpcodeFlags::NoModRM | OpcodeFlags::Force64 | OpcodeFlags::Conditional | OpcodeFlags::BNDPrefix, 0, 0, { 14, 0, 0, 0, 0, }  }, // JLE RelativeBranch-Byte, 
  { 0x0'0'7F'00'0'0, "JNLE", OpcodeFlags::NoModRM | OpcodeFlags::Force64 | OpcodeFlags::Conditional | OpcodeFlags::BNDPrefix, 0, 0, { 14, 0, 0, 0, 0, }  }, // JNLE RelativeBranch-Byte, 
  { 0x0'0'80'00'0'0, "ADD", OpcodeFlags::FixedReg, 0, 0, { 2, 9, 0, 0, 0, }  }, // ADD Memory-Byte, Immidate-Byte, 
  { 0x0'0'80'08'0'0, "OR", OpcodeFlags::FixedReg, 0, 0, { 2, 9, 0, 0, 0, }  }, // OR Memory-Byte, Immidate-Byte, 
  { 0x0'0'80'10'0'0, "ADC", OpcodeFlags::FixedReg, 0, 0, { 2, 9, 0, 0, 0, }  }, // ADC Memory-Byte, Immidate-Byte, 
  { 0x0'0'80'18'0'0, "SBB", OpcodeFlags::FixedReg, 0, 0, { 2, 9, 0, 0, 0, }  }, // SBB Memory-Byte, Immidate-Byte, 
  { 0x0'0'80'20'0'0, "AND", OpcodeFlags::FixedReg, 0, 0, { 2, 9, 0, 0, 0, }  }, // AND Memory-Byte, Immidate-Byte, 
  { 0x0'0'80'28'0'0, "SUB", OpcodeFlags::FixedReg, 0, 0, { 2, 9, 0, 0, 0, }  }, // SUB Memory-Byte, Immidate-Byte, 
  { 0x0'0'80'30'0'0, "XOR", OpcodeFlags::FixedReg, 0, 0, { 2, 9, 0, 0, 0, }  }, // XOR Memory-Byte, Immidate-Byte, 
  { 0x0'0'80'38'0'0, "CMP", OpcodeFlags::FixedReg, 0, 0, { 2, 9, 0, 0, 0, }  }, // CMP Memory-Byte, Immidate-Byte, 
  { 0x0'0'80'C0'0'0, "ADD", OpcodeFlags::FixedMod | OpcodeFlags::FixedReg, 0, 0, { 17, 9, 0, 0, 0, }  }, // ADD GenReg-Byte, Immidate-Byte, 
  { 0x0'0'80'C8'0'0, "OR", OpcodeFlags::FixedMod | OpcodeFlags::FixedReg, 0, 0, { 17, 9, 0, 0, 0, }  }, // OR GenReg-Byte, Immidate-Byte, 
  { 0x0'0'80'D0'0'0, "ADC", OpcodeFlags::FixedMod | OpcodeFlags::FixedReg, 0, 0, { 17, 9, 0, 0, 0, }  }, // ADC GenReg-Byte, Immidate-Byte, 
  { 0x0'0'80'D8'0'0, "SBB", OpcodeFlags::FixedMod | OpcodeFlags::FixedReg, 0, 0, { 17, 9, 0, 0, 0, }  }, // SBB GenReg-Byte, Immidate-Byte, 
  { 0x0'0'80'E0'0'0, "AND", OpcodeFlags::FixedMod | OpcodeFlags::FixedReg, 0, 0, { 17, 9, 0, 0, 0, }  }, // AND GenReg-Byte, Immidate-Byte, 
  { 0x0'0'80'E8'0'0, "SUB", OpcodeFlags::FixedMod | OpcodeFlags::FixedReg, 0, 0, { 17, 9, 0, 0, 0, }  }, // SUB GenReg-Byte, Immidate-Byte, 
  { 0x0'0'80'F0'0'0, "XOR", OpcodeFlags::FixedMod | OpcodeFlags::FixedReg, 0, 0, { 17, 9, 0, 0, 0, }  }, // XOR GenReg-Byte, Immidate-Byte, 
  { 0x0'0'80'F8'0'0, "CMP", OpcodeFlags::FixedMod | OpcodeFlags::FixedReg, 0, 0, { 17, 9, 0, 0, 0, }  }, // CMP GenReg-Byte, Immidate-Byte, 
  { 0x0'0'81'00'0'0, "ADD", OpcodeFlags::FixedReg, 0, 0, { 5, 12, 0, 0, 0, }  }, // ADD Memory-Opsize, Immidate-Opsize32, 
  { 0x0'0'81'08'0'0, "OR", OpcodeFlags::FixedReg, 0, 0, { 5, 12, 0, 0, 0, }  }, // OR Memory-Opsize, Immidate-Opsize32, 
  { 0x0'0'81'10'0'0, "ADC", OpcodeFlags::FixedReg, 0, 0, { 5, 12, 0, 0, 0, }  }, // ADC Memory-Opsize, Immidate-Opsize32, 
  { 0x0'0'81'18'0'0, "SBB", OpcodeFlags::FixedReg, 0, 0, { 5, 12, 0, 0, 0, }  }, // SBB Memory-Opsize, Immidate-Opsize32, 
  { 0x0'0'81'20'0'0, "AND", OpcodeFlags::FixedReg, 0, 0, { 5, 12, 0, 0, 0, }  }, // AND Memory-Opsize, Immidate-Opsize32, 
  { 0x0'0'81'28'0'0, "SUB", OpcodeFlags::FixedReg, 0, 0, { 5, 12, 0, 0, 0, }  }, // SUB Memory-Opsize, Immidate-Opsize32, 
  { 0x0'0'81'30'0'0, "XOR", OpcodeFlags::FixedReg, 0, 0, { 5, 12, 0, 0, 0, }  }, // XOR Memory-Opsize, Immidate-Opsize32, 
  { 0x0'0'81'38'0'0, "CMP", OpcodeFlags::FixedReg, 0, 0, { 5, 12, 0, 0, 0, }  }, // CMP Memory-Opsize, Immidate-Opsize32, 
  { 0x0'0'81'C0'0'0, "ADD", OpcodeFlags::FixedMod | OpcodeFlags::FixedReg, 0, 0, { 22, 12, 0, 0, 0, }  }, // ADD GenReg-Opsize, Immidate-Opsize32, 
  { 0x0'0'81'C8'0'0, "OR", OpcodeFlags::FixedMod | OpcodeFlags::FixedReg, 0, 0, { 22, 12, 0, 0, 0, }  }, // OR GenReg-Opsize, Immidate-Opsize32, 
  { 0x0'0'81'D0'0'0, "ADC", OpcodeFlags::FixedMod | OpcodeFlags::FixedReg, 0, 0, { 22, 12, 0, 0, 0, }  }, // ADC GenReg-Opsize, Immidate-Opsize32, 
  { 0x0'0'81'D8'0'0, "SBB", OpcodeFlags::FixedMod | OpcodeFlags::FixedReg, 0, 0, { 22, 12, 0, 0, 0, }  }, // SBB GenReg-Opsize, Immidate-Opsize32, 
  { 0x0'0'81'E0'0'0, "AND", OpcodeFlags::FixedMod | OpcodeFlags::FixedReg, 0, 0, { 22, 12, 0, 0, 0, }  }, // AND GenReg-Opsize, Immidate-Opsize32, 
  { 0x0'0'81'E8'0'0, "SUB", OpcodeFlags::FixedMod | OpcodeFlags::FixedReg, 0, 0, { 22, 12, 0, 0, 0, }  }, // SUB GenReg-Opsize, Immidate-Opsize32, 
  { 0x0'0'81'F0'0'0, "XOR", OpcodeFlags::FixedMod | OpcodeFlags::FixedReg, 0, 0, { 22, 12, 0, 0, 0, }  }, // XOR GenReg-Opsize, Immidate-Opsize32, 
  { 0x0'0'81'F8'0'0, "CMP", OpcodeFlags::FixedMod | OpcodeFlags::FixedReg, 0, 0, { 22, 12, 0, 0, 0, }  }, // CMP GenReg-Opsize, Immidate-Opsize32, 
  { 0x0'0'82'00'0'0, "ADD", OpcodeFlags::FixedReg | OpcodeFlags::Not64, 0, 0, { 2, 9, 0, 0, 0, }  }, // ADD Memory-Byte, Immidate-Byte, 
  { 0x0'0'82'08'0'0, "OR", OpcodeFlags::FixedReg | OpcodeFlags::Not64, 0, 0, { 2, 9, 0, 0, 0, }  }, // OR Memory-Byte, Immidate-Byte, 
  { 0x0'0'82'10'0'0, "ADC", OpcodeFlags::FixedReg | OpcodeFlags::Not64, 0, 0, { 2, 9, 0, 0, 0, }  }, // ADC Memory-Byte, Immidate-Byte, 
  { 0x0'0'82'18'0'0, "SBB", OpcodeFlags::FixedReg | OpcodeFlags::Not64, 0, 0, { 2, 9, 0, 0, 0, }  }, // SBB Memory-Byte, Immidate-Byte, 
  { 0x0'0'82'20'0'0, "AND", OpcodeFlags::FixedReg | OpcodeFlags::Not64, 0, 0, { 2, 9, 0, 0, 0, }  }, // AND Memory-Byte, Immidate-Byte, 
  { 0x0'0'82'28'0'0, "SUB", OpcodeFlags::FixedReg | OpcodeFlags::Not64, 0, 0, { 2, 9, 0, 0, 0, }  }, // SUB Memory-Byte, Immidate-Byte, 
  { 0x0'0'82'30'0'0, "XOR", OpcodeFlags::FixedReg | OpcodeFlags::Not64, 0, 0, { 2, 9, 0, 0, 0, }  }, // XOR Memory-Byte, Immidate-Byte, 
  { 0x0'0'82'38'0'0, "CMP", OpcodeFlags::FixedReg | OpcodeFlags::Not64, 0, 0, { 2, 9, 0, 0, 0, }  }, // CMP Memory-Byte, Immidate-Byte, 
  { 0x0'0'82'C0'0'0, "ADD", OpcodeFlags::FixedMod | OpcodeFlags::FixedReg | OpcodeFlags::Not64, 0, 0, { 17, 9, 0, 0, 0, }  }, // ADD GenReg-Byte, Immidate-Byte, 
  { 0x0'0'82'C8'0'0, "OR", OpcodeFlags::FixedMod | OpcodeFlags::FixedReg | OpcodeFlags::Not64, 0, 0, { 17, 9, 0, 0, 0, }  }, // OR GenReg-Byte, Immidate-Byte, 
  { 0x0'0'82'D0'0'0, "ADC", OpcodeFlags::FixedMod | OpcodeFlags::FixedReg | OpcodeFlags::Not64, 0, 0, { 17, 9, 0, 0, 0, }  }, // ADC GenReg-Byte, Immidate-Byte, 
  { 0x0'0'82'D8'0'0, "SBB", OpcodeFlags::FixedMod | OpcodeFlags::FixedReg | OpcodeFlags::Not64, 0, 0, { 17, 9, 0, 0, 0, }  }, // SBB GenReg-Byte, Immidate-Byte, 
  { 0x0'0'82'E0'0'0, "AND", OpcodeFlags::FixedMod | OpcodeFlags::FixedReg | OpcodeFlags::Not64, 0, 0, { 17, 9, 0, 0, 0, }  }, // AND GenReg-Byte, Immidate-Byte, 
  { 0x0'0'82'E8'0'0, "SUB", OpcodeFlags::FixedMod | OpcodeFlags::FixedReg | OpcodeFlags::Not64, 0, 0, { 17, 9, 0, 0, 0, }  }, // SUB GenReg-Byte, Immidate-Byte, 
  { 0x0'0'82'F0'0'0, "XOR", OpcodeFlags::FixedMod | OpcodeFlags::FixedReg | OpcodeFlags::Not64, 0, 0, { 17, 9, 0, 0, 0, }  }, // XOR GenReg-Byte, Immidate-Byte, 
  { 0x0'0'82'F8'0'0, "CMP", OpcodeFlags::FixedMod | OpcodeFlags::FixedReg | OpcodeFlags::Not64, 0, 0, { 17, 9, 0, 0, 0, }  }, // CMP GenReg-Byte, Immidate-Byte, 
  { 0x0'0'83'00'0'0, "ADD", OpcodeFlags::FixedReg, 0, 0, { 5, 9, 0, 0, 0, }  }, // ADD Memory-Opsize, Immidate-Byte, 
  { 0x0'0'83'08'0'0, "OR", OpcodeFlags::FixedReg, 0, 0, { 5, 9, 0, 0, 0, }  }, // OR Memory-Opsize, Immidate-Byte, 
  { 0x0'0'83'10'0'0, "ADC", OpcodeFlags::FixedReg, 0, 0, { 5, 9, 0, 0, 0, }  }, // ADC Memory-Opsize, Immidate-Byte, 
  { 0x0'0'83'18'0'0, "SBB", OpcodeFlags::FixedReg, 0, 0, { 5, 9, 0, 0, 0, }  }, // SBB Memory-Opsize, Immidate-Byte, 
  { 0x0'0'83'20'0'0, "AND", OpcodeFlags::FixedReg, 0, 0, { 5, 9, 0, 0, 0, }  }, // AND Memory-Opsize, Immidate-Byte, 
  { 0x0'0'83'28'0'0, "SUB", OpcodeFlags::FixedReg, 0, 0, { 5, 9, 0, 0, 0, }  }, // SUB Memory-Opsize, Immidate-Byte, 
  { 0x0'0'83'30'0'0, "XOR", OpcodeFlags::FixedReg, 0, 0, { 5, 9, 0, 0, 0, }  }, // XOR Memory-Opsize, Immidate-Byte, 
  { 0x0'0'83'38'0'0, "CMP", OpcodeFlags::FixedReg, 0, 0, { 5, 9, 0, 0, 0, }  }, // CMP Memory-Opsize, Immidate-Byte, 
  { 0x0'0'83'C0'0'0, "ADD", OpcodeFlags::FixedMod | OpcodeFlags::FixedReg, 0, 0, { 22, 9, 0, 0, 0, }  }, // ADD GenReg-Opsize, Immidate-Byte, 
  { 0x0'0'83'C8'0'0, "OR", OpcodeFlags::FixedMod | OpcodeFlags::FixedReg, 0, 0, { 22, 9, 0, 0, 0, }  }, // OR GenReg-Opsize, Immidate-Byte, 
  { 0x0'0'83'D0'0'0, "ADC", OpcodeFlags::FixedMod | OpcodeFlags::FixedReg, 0, 0, { 22, 9, 0, 0, 0, }  }, // ADC GenReg-Opsize, Immidate-Byte, 
  { 0x0'0'83'D8'0'0, "SBB", OpcodeFlags::FixedMod | OpcodeFlags::FixedReg, 0, 0, { 22, 9, 0, 0, 0, }  }, // SBB GenReg-Opsize, Immidate-Byte, 
  { 0x0'0'83'E0'0'0, "AND", OpcodeFlags::FixedMod | OpcodeFlags::FixedReg, 0, 0, { 22, 9, 0, 0, 0, }  }, // AND GenReg-Opsize, Immidate-Byte, 
  { 0x0'0'83'E8'0'0, "SUB", OpcodeFlags::FixedMod | OpcodeFlags::FixedReg, 0, 0, { 22, 9, 0, 0, 0, }  }, // SUB GenReg-Opsize, Immidate-Byte, 
  { 0x0'0'83'F0'0'0, "XOR", OpcodeFlags::FixedMod | OpcodeFlags::FixedReg, 0, 0, { 22, 9, 0, 0, 0, }  }, // XOR GenReg-Opsize, Immidate-Byte, 
  { 0x0'0'83'F8'0'0, "CMP", OpcodeFlags::FixedMod | OpcodeFlags::FixedReg, 0, 0, { 22, 9, 0, 0, 0, }  }, // CMP GenReg-Opsize, Immidate-Byte, 
  { 0x0'0'84'00'0'0, "TEST", OpcodeFlags::None, 0, 0, { 2, 16, 0, 0, 0, }  }, // TEST Memory-Byte, GenReg-Byte, 
  { 0x0'0'84'C0'0'0, "TEST", OpcodeFlags::FixedMod, 0, 0, { 17, 16, 0, 0, 0, }  }, // TEST GenReg-Byte, GenReg-Byte, 
  { 0x0'0'85'00'0'0, "TEST", OpcodeFlags::None, 0, 0, { 5, 21, 0, 0, 0, }  }, // TEST Memory-Opsize, GenReg-Opsize, 
  { 0x0'0'85'C0'0'0, "TEST", OpcodeFlags::FixedMod, 0, 0, { 22, 21, 0, 0, 0, }  }, // TEST GenReg-Opsize, GenReg-Opsize, 
  { 0x0'0'86'00'0'0, "XCHG", OpcodeFlags::None, 0, 0, { 2, 16, 0, 0, 0, }  }, // XCHG Memory-Byte, GenReg-Byte, 
  { 0x0'0'86'C0'0'0, "XCHG", OpcodeFlags::FixedMod, 0, 0, { 17, 16, 0, 0, 0, }  }, // XCHG GenReg-Byte, GenReg-Byte, 
  { 0x0'0'87'00'0'0, "XCHG", OpcodeFlags::None, 0, 0, { 5, 21, 0, 0, 0, }  }, // XCHG Memory-Opsize, GenReg-Opsize, 
  { 0x0'0'87'C0'0'0, "XCHG", OpcodeFlags::FixedMod, 0, 0, { 22, 21, 0, 0, 0, }  }, // XCHG GenReg-Opsize, GenReg-Opsize, 
  { 0x0'0'88'00'0'0, "MOV", OpcodeFlags::None, 0, 0, { 2, 16, 0, 0, 0, }  }, // MOV Memory-Byte, GenReg-Byte, 
  { 0x0'0'88'C0'0'0, "MOV", OpcodeFlags::FixedMod, 0, 0, { 17, 16, 0, 0, 0, }  }, // MOV GenReg-Byte, GenReg-Byte, 
  { 0x0'0'89'00'0'0, "MOV", OpcodeFlags::None, 0, 0, { 5, 21, 0, 0, 0, }  }, // MOV Memory-Opsize, GenReg-Opsize, 
  { 0x0'0'89'C0'0'0, "MOV", OpcodeFlags::FixedMod, 0, 0, { 22, 21, 0, 0, 0, }  }, // MOV GenReg-Opsize, GenReg-Opsize, 
  { 0x0'0'8A'00'0'0, "MOV", OpcodeFlags::None, 0, 0, { 16, 2, 0, 0, 0, }  }, // MOV GenReg-Byte, Memory-Byte, 
  { 0x0'0'8A'C0'0'0, "MOV", OpcodeFlags::FixedMod, 0, 0, { 16, 17, 0, 0, 0, }  }, // MOV GenReg-Byte, GenReg-Byte, 
  { 0x0'0'8B'00'0'0, "MOV", OpcodeFlags::None, 0, 0, { 21, 5, 0, 0, 0, }  }, // MOV GenReg-Opsize, Memory-Opsize, 
  { 0x0'0'8B'C0'0'0, "MOV", OpcodeFlags::FixedMod, 0, 0, { 21, 22, 0, 0, 0, }  }, // MOV GenReg-Opsize, GenReg-Opsize, 
  { 0x0'0'8C'00'0'0, "MOV", OpcodeFlags::None, 0, 0, { 3, 26, 0, 0, 0, }  }, // MOV Memory-Word, SegmentReg-Word, 
  { 0x0'0'8C'C0'0'0, "MOV", OpcodeFlags::FixedMod, 0, 0, { 22, 26, 0, 0, 0, }  }, // MOV GenReg-Opsize, SegmentReg-Word, 
  { 0x0'0'8D'00'0'0, "LEA", OpcodeFlags::None, 0, 0, { 21, 4, 0, 0, 0, }  }, // LEA GenReg-Opsize, Memory-MemData, 
  { 0x0'0'8E'00'0'0, "MOV", OpcodeFlags::None, 0, 0, { 26, 3, 0, 0, 0, }  }, // MOV SegmentReg-Word, Memory-Word, 
  { 0x0'0'8E'C0'0'0, "MOV", OpcodeFlags::FixedMod, 0, 0, { 26, 18, 0, 0, 0, }  }, // MOV SegmentReg-Word, GenReg-Word, 
  { 0x0'0'8F'00'0'0, "POP", OpcodeFlags::FixedReg | OpcodeFlags::Default64, 0, 0, { 5, 0, 0, 0, 0, }  }, // POP Memory-Opsize, 
  { 0x0'0'8F'C0'0'0, "POP", OpcodeFlags::FixedMod | OpcodeFlags::FixedReg | OpcodeFlags::Default64, 0, 0, { 22, 0, 0, 0, 0, }  }, // POP GenReg-Opsize, 
  { 0x0'0'90'00'0'0, "NOP", OpcodeFlags::NoModRM | OpcodeFlags::ExtendedPrefix, 0, 0, { 0, 0, 0, 0, 0, }  }, // NOP 
  { 0x0'0'90'00'0'0, "XCHG", OpcodeFlags::NoModRM | OpcodeFlags::ExtendedPrefix | OpcodeFlags::Mode64, 0, 0, { 27, 28, 0, 0, 0, }  }, // XCHG oAX-Opsize, oAX-Opsize, 
  { 0x0'0'90'00'8'0, "NOP", OpcodeFlags::NoModRM | OpcodeFlags::ExtendedPrefix, 0, 0, { 0, 0, 0, 0, 0, }  }, // NOP 
  { 0x0'0'91'00'0'0, "XCHG", OpcodeFlags::NoModRM, 0, 0, { 30, 28, 0, 0, 0, }  }, // XCHG oCX-Opsize, oAX-Opsize, 
  { 0x0'0'92'00'0'0, "XCHG", OpcodeFlags::NoModRM, 0, 0, { 31, 28, 0, 0, 0, }  }, // XCHG oDX-Opsize, oAX-Opsize, 
  { 0x0'0'93'00'0'0, "XCHG", OpcodeFlags::NoModRM, 0, 0, { 32, 28, 0, 0, 0, }  }, // XCHG oBX-Opsize, oAX-Opsize, 
  { 0x0'0'94'00'0'0, "XCHG", OpcodeFlags::NoModRM, 0, 0, { 33, 28, 0, 0, 0, }  }, // XCHG oSP-Opsize, oAX-Opsize, 
  { 0x0'0'95'00'0'0, "XCHG", OpcodeFlags::NoModRM, 0, 0, { 34, 28, 0, 0, 0, }  }, // XCHG oBP-Opsize, oAX-Opsize, 
  { 0x0'0'96'00'0'0, "XCHG", OpcodeFlags::NoModRM, 0, 0, { 35, 28, 0, 0, 0, }  }, // XCHG oSI-Opsize, oAX-Opsize, 
  { 0x0'0'97'00'0'0, "XCHG", OpcodeFlags::NoModRM, 0, 0, { 36, 28, 0, 0, 0, }  }, // XCHG oDI-Opsize, oAX-Opsize, 
  { 0x0'0'98'00'1'0, "CBW", OpcodeFlags::NoModRM | OpcodeFlags::ExtendedSize, 0, 0, { 0, 0, 0, 0, 0, }  }, // CBW 
  { 0x0'0'99'00'1'0, "CWD", OpcodeFlags::NoModRM | OpcodeFlags::ExtendedSize, 0, 0, { 0, 0, 0, 0, 0, }  }, // CWD 
  { 0x0'0'9A'00'0'0, "CALL FAR", OpcodeFlags::NoModRM | OpcodeFlags::Branch | OpcodeFlags::Not64, 0, 0, { 13, 0, 0, 0, 0, }  }, // CALL FAR Immidate-FarPointer, 
  { 0x0'0'9C'00'1'0, "PUSHF", OpcodeFlags::NoModRM | OpcodeFlags::ExtendedSize, 0, 0, { 0, 0, 0, 0, 0, }  }, // PUSHF 
  { 0x0'0'9D'00'1'0, "POPF", OpcodeFlags::NoModRM | OpcodeFlags::ExtendedSize, 0, 0, { 0, 0, 0, 0, 0, }  }, // POPF 
  { 0x0'0'9E'00'0'0, "SAHF", OpcodeFlags::NoModRM, 0, 0, { 0, 0, 0, 0, 0, }  }, // SAHF 
  { 0x0'0'9F'00'0'0, "LAHF", OpcodeFlags::NoModRM, 0, 0, { 0, 0, 0, 0, 0, }  }, // LAHF 
  { 0x0'0'A0'00'0'0, "MOV", OpcodeFlags::NoModRM, 0, 0, { 37, 7, 0, 0, 0, }  }, // MOV AL-Byte, MOffset-Byte, 
  { 0x0'0'A1'00'0'0, "MOV", OpcodeFlags::NoModRM, 0, 0, { 27, 8, 0, 0, 0, }  }, // MOV oAX-Opsize, MOffset-Opsize, 
  { 0x0'0'A2'00'0'0, "MOV", OpcodeFlags::NoModRM, 0, 0, { 7, 37, 0, 0, 0, }  }, // MOV MOffset-Byte, AL-Byte, 
  { 0x0'0'A3'00'0'0, "MOV", OpcodeFlags::NoModRM, 0, 0, { 8, 27, 0, 0, 0, }  }, // MOV MOffset-Opsize, oAX-Opsize, 
  { 0x0'0'A4'00'0'0, "MOVSB", OpcodeFlags::NoModRM, 0, 0, { 0, 0, 0, 0, 0, }  }, // MOVSB 
  { 0x0'0'A5'00'1'0, "MOVSW", OpcodeFlags::NoModRM | OpcodeFlags::ExtendedSize, 0, 0, { 0, 0, 0, 0, 0, }  }, // MOVSW 
  { 0x0'0'A6'00'0'0, "CMPSB", OpcodeFlags::NoModRM, 0, 0, { 0, 0, 0, 0, 0, }  }, // CMPSB 
  { 0x0'0'A7'00'1'0, "CMPSW", OpcodeFlags::NoModRM | OpcodeFlags::ExtendedSize, 0, 0, { 0, 0, 0, 0, 0, }  }, // CMPSW 
  { 0x0'0'A8'00'0'0, "TEST", OpcodeFlags::NoModRM, 0, 0, { 37, 9, 0, 0, 0, }  }, // TEST AL-Byte, Immidate-Byte, 
  { 0x0'0'A9'00'0'0, "TEST", OpcodeFlags::NoModRM, 0, 0, { 27, 12, 0, 0, 0, }  }, // TEST oAX-Opsize, Immidate-Opsize32, 
  { 0x0'0'AA'00'0'0, "STOSB", OpcodeFlags::NoModRM, 0, 0, { 0, 0, 0, 0, 0, }  }, // STOSB 
  { 0x0'0'AB'00'1'0, "STOSW", OpcodeFlags::NoModRM | OpcodeFlags::ExtendedSize, 0, 0, { 0, 0, 0, 0, 0, }  }, // STOSW 
  { 0x0'0'AC'00'0'0, "LODSB", OpcodeFlags::NoModRM, 0, 0, { 0, 0, 0, 0, 0, }  }, // LODSB 
  { 0x0'0'AD'00'1'0, "LODSW", OpcodeFlags::NoModRM | OpcodeFlags::ExtendedSize, 0, 0, { 0, 0, 0, 0, 0, }  }, // LODSW 
  { 0x0'0'AE'00'0'0, "SCASB", OpcodeFlags::NoModRM, 0, 0, { 0, 0, 0, 0, 0, }  }, // SCASB 
  { 0x0'0'AF'00'1'0, "SCASW", OpcodeFlags::NoModRM | OpcodeFlags::ExtendedSize, 0, 0, { 0, 0, 0, 0, 0, }  }, // SCASW 
  { 0x0'0'B0'00'0'0, "MOV", OpcodeFlags::NoModRM, 0, 0, { 37, 9, 0, 0, 0, }  }, // MOV AL-Byte, Immidate-Byte, 
  { 0x0'0'B1'00'0'0, "MOV", OpcodeFlags::NoModRM, 0, 0, { 38, 9, 0, 0, 0, }  }, // MOV CL-Byte, Immidate-Byte, 
  { 0x0'0'B2'00'0'0, "MOV", OpcodeFlags::NoModRM, 0, 0, { 39, 9, 0, 0, 0, }  }, // MOV DL-Byte, Immidate-Byte, 
  { 0x0'0'B3'00'0'0, "MOV", OpcodeFlags::NoModRM, 0, 0, { 40, 9, 0, 0, 0, }  }, // MOV BL-Byte, Immidate-Byte, 
  { 0x0'0'B4'00'0'0, "MOV", OpcodeFlags::NoModRM, 0, 0, { 41, 9, 0, 0, 0, }  }, // MOV AH-Byte, Immidate-Byte, 
  { 0x0'0'B5'00'0'0, "MOV", OpcodeFlags::NoModRM, 0, 0, { 42, 9, 0, 0, 0, }  }, // MOV CH-Byte, Immidate-Byte, 
  { 0x0'0'B6'00'0'0, "MOV", OpcodeFlags::NoModRM, 0, 0, { 43, 9, 0, 0, 0, }  }, // MOV DH-Byte, Immidate-Byte, 
  { 0x0'0'B7'00'0'0, "MOV", OpcodeFlags::NoModRM, 0, 0, { 44, 9, 0, 0, 0, }  }, // MOV BH-Byte, Immidate-Byte, 
  { 0x0'0'B8'00'0'0, "MOV", OpcodeFlags::NoModRM, 0, 0, { 27, 11, 0, 0, 0, }  }, // MOV oAX-Opsize, Immidate-Opsize, 
  { 0x0'0'B9'00'0'0, "MOV", OpcodeFlags::NoModRM, 0, 0, { 30, 11, 0, 0, 0, }  }, // MOV oCX-Opsize, Immidate-Opsize, 
  { 0x0'0'BA'00'0'0, "MOV", OpcodeFlags::NoModRM, 0, 0, { 31, 11, 0, 0, 0, }  }, // MOV oDX-Opsize, Immidate-Opsize, 
  { 0x0'0'BB'00'0'0, "MOV", OpcodeFlags::NoModRM, 0, 0, { 32, 11, 0, 0, 0, }  }, // MOV oBX-Opsize, Immidate-Opsize, 
  { 0x0'0'BC'00'0'0, "MOV", OpcodeFlags::NoModRM, 0, 0, { 33, 11, 0, 0, 0, }  }, // MOV oSP-Opsize, Immidate-Opsize, 
  { 0x0'0'BD'00'0'0, "MOV", OpcodeFlags::NoModRM, 0, 0, { 34, 11, 0, 0, 0, }  }, // MOV oBP-Opsize, Immidate-Opsize, 
  { 0x0'0'BE'00'0'0, "MOV", OpcodeFlags::NoModRM, 0, 0, { 35, 11, 0, 0, 0, }  }, // MOV oSI-Opsize, Immidate-Opsize, 
  { 0x0'0'BF'00'0'0, "MOV", OpcodeFlags::NoModRM, 0, 0, { 36, 11, 0, 0, 0, }  }, // MOV oDI-Opsize, Immidate-Opsize, 
  { 0x0'0'C2'00'0'0, "RET", OpcodeFlags::NoModRM | OpcodeFlags::Default64 | OpcodeFlags::Branch | OpcodeFlags::BNDPrefix, 0, 0, { 10, 0, 0, 0, 0, }  }, // RET Immidate-Word, 
  { 0x0'0'C3'00'0'0, "RET", OpcodeFlags::NoModRM | OpcodeFlags::Default64 | OpcodeFlags::Branch | OpcodeFlags::BNDPrefix, 0, 0, { 0, 0, 0, 0, 0, }  }, // RET 
  { 0x0'0'C4'00'0'0, "LES", OpcodeFlags::Not64, 0, 0, { 23, 6, 0, 0, 0, }  }, // LES GenReg-Opsize32, Memory-FarPointer, 
  { 0x0'0'C5'00'0'0, "LDS", OpcodeFlags::Not64, 0, 0, { 23, 6, 0, 0, 0, }  }, // LDS GenReg-Opsize32, Memory-FarPointer, 
  { 0x0'0'C6'00'0'0, "MOV", OpcodeFlags::FixedReg, 0, 0, { 2, 9, 0, 0, 0, }  }, // MOV Memory-Byte, Immidate-Byte, 
  { 0x0'0'C6'C0'0'0, "MOV", OpcodeFlags::FixedMod | OpcodeFlags::FixedReg, 0, 0, { 17, 9, 0, 0, 0, }  }, // MOV GenReg-Byte, Immidate-Byte, 
  { 0x0'0'C7'00'0'0, "MOV", OpcodeFlags::FixedReg, 0, 0, { 5, 12, 0, 0, 0, }  }, // MOV Memory-Opsize, Immidate-Opsize32, 
  { 0x0'0'C7'C0'0'0, "MOV", OpcodeFlags::FixedMod | OpcodeFlags::FixedReg, 0, 0, { 22, 12, 0, 0, 0, }  }, // MOV GenReg-Opsize, Immidate-Opsize32, 
  { 0x0'0'CA'00'0'0, "RET FAR", OpcodeFlags::NoModRM | OpcodeFlags::Branch, 0, 0, { 10, 0, 0, 0, 0, }  }, // RET FAR Immidate-Word, 
  { 0x0'0'CB'00'0'0, "RET FAR", OpcodeFlags::NoModRM | OpcodeFlags::Branch, 0, 0, { 0, 0, 0, 0, 0, }  }, // RET FAR 
  { 0x0'0'CC'00'0'0, "INT3", OpcodeFlags::NoModRM, 0, 0, { 0, 0, 0, 0, 0, }  }, // INT3 
  { 0x0'0'CD'00'0'0, "INT", OpcodeFlags::NoModRM, 0, 0, { 9, 0, 0, 0, 0, }  }, // INT Immidate-Byte, 
  { 0x0'0'CE'00'0'0, "INTO", OpcodeFlags::NoModRM | OpcodeFlags::Not64, 0, 0, { 0, 0, 0, 0, 0, }  }, // INTO 
  { 0x0'0'CF'00'1'0, "IRET", OpcodeFlags::NoModRM | OpcodeFlags::Branch | OpcodeFlags::ExtendedSize, 0, 0, { 0, 0, 0, 0, 0, }  }, // IRET 
  { 0x0'0'D0'00'0'0, "ROL", OpcodeFlags::FixedReg, 0, 0, { 2, 1, 0, 0, 0, }  }, // ROL Memory-Byte, One-Byte, 
  { 0x0'0'D0'08'0'0, "ROR", OpcodeFlags::FixedReg, 0, 0, { 2, 1, 0, 0, 0, }  }, // ROR Memory-Byte, One-Byte, 
  { 0x0'0'D0'10'0'0, "RCL", OpcodeFlags::FixedReg, 0, 0, { 2, 1, 0, 0, 0, }  }, // RCL Memory-Byte, One-Byte, 
  { 0x0'0'D0'18'0'0, "RCR", OpcodeFlags::FixedReg, 0, 0, { 2, 1, 0, 0, 0, }  }, // RCR Memory-Byte, One-Byte, 
  { 0x0'0'D0'20'0'0, "SHL", OpcodeFlags::FixedReg, 0, 0, { 2, 1, 0, 0, 0, }  }, // SHL Memory-Byte, One-Byte, 
  { 0x0'0'D0'28'0'0, "SHR", OpcodeFlags::FixedReg, 0, 0, { 2, 1, 0, 0, 0, }  }, // SHR Memory-Byte, One-Byte, 
  { 0x0'0'D0'30'0'0, "SHL", OpcodeFlags::FixedReg, 0, 0, { 2, 1, 0, 0, 0, }  }, // SHL Memory-Byte, One-Byte, 
  { 0x0'0'D0'38'0'0, "SAR", OpcodeFlags::FixedReg, 0, 0, { 2, 1, 0, 0, 0, }  }, // SAR Memory-Byte, One-Byte, 
  { 0x0'0'D0'C0'0'0, "ROL", OpcodeFlags::FixedMod | OpcodeFlags::FixedReg, 0, 0, { 17, 1, 0, 0, 0, }  }, // ROL GenReg-Byte, One-Byte, 
  { 0x0'0'D0'C8'0'0, "ROR", OpcodeFlags::FixedMod | OpcodeFlags::FixedReg, 0, 0, { 17, 1, 0, 0, 0, }  }, // ROR GenReg-Byte, One-Byte, 
  { 0x0'0'D0'D0'0'0, "RCL", OpcodeFlags::FixedMod | OpcodeFlags::FixedReg, 0, 0, { 17, 1, 0, 0, 0, }  }, // RCL GenReg-Byte, One-Byte, 
  { 0x0'0'D0'D8'0'0, "RCR", OpcodeFlags::FixedMod | OpcodeFlags::FixedReg, 0, 0, { 17, 1, 0, 0, 0, }  }, // RCR GenReg-Byte, One-Byte, 
  { 0x0'0'D0'E0'0'0, "SHL", OpcodeFlags::FixedMod | OpcodeFlags::FixedReg, 0, 0, { 17, 1, 0, 0, 0, }  }, // SHL GenReg-Byte, One-Byte, 
  { 0x0'0'D0'E8'0'0, "SHR", OpcodeFlags::FixedMod | OpcodeFlags::FixedReg, 0, 0, { 17, 1, 0, 0, 0, }  }, // SHR GenReg-Byte, One-Byte, 
  { 0x0'0'D0'F0'0'0, "SHL", OpcodeFlags::FixedMod | OpcodeFlags::FixedReg, 0, 0, { 17, 1, 0, 0, 0, }  }, // SHL GenReg-Byte, One-Byte, 
  { 0x0'0'D0'F8'0'0, "SAR", OpcodeFlags::FixedMod | OpcodeFlags::FixedReg, 0, 0, { 17, 1, 0, 0, 0, }  }, // SAR GenReg-Byte, One-Byte, 
  { 0x0'0'D1'00'0'0, "ROL", OpcodeFlags::FixedReg, 0, 0, { 5, 1, 0, 0, 0, }  }, // ROL Memory-Opsize, One-Byte, 
  { 0x0'0'D1'08'0'0, "ROR", OpcodeFlags::FixedReg, 0, 0, { 5, 1, 0, 0, 0, }  }, // ROR Memory-Opsize, One-Byte, 
  { 0x0'0'D1'10'0'0, "RCL", OpcodeFlags::FixedReg, 0, 0, { 5, 1, 0, 0, 0, }  }, // RCL Memory-Opsize, One-Byte, 
  { 0x0'0'D1'18'0'0, "RCR", OpcodeFlags::FixedReg, 0, 0, { 5, 1, 0, 0, 0, }  }, // RCR Memory-Opsize, One-Byte, 
  { 0x0'0'D1'20'0'0, "SHL", OpcodeFlags::FixedReg, 0, 0, { 5, 1, 0, 0, 0, }  }, // SHL Memory-Opsize, One-Byte, 
  { 0x0'0'D1'28'0'0, "SHR", OpcodeFlags::FixedReg, 0, 0, { 5, 1, 0, 0, 0, }  }, // SHR Memory-Opsize, One-Byte, 
  { 0x0'0'D1'30'0'0, "SHL", OpcodeFlags::FixedReg, 0, 0, { 5, 1, 0, 0, 0, }  }, // SHL Memory-Opsize, One-Byte, 
  { 0x0'0'D1'38'0'0, "SAR", OpcodeFlags::FixedReg, 0, 0, { 5, 1, 0, 0, 0, }  }, // SAR Memory-Opsize, One-Byte, 
  { 0x0'0'D1'C0'0'0, "ROL", OpcodeFlags::FixedMod | OpcodeFlags::FixedReg, 0, 0, { 22, 1, 0, 0, 0, }  }, // ROL GenReg-Opsize, One-Byte, 
  { 0x0'0'D1'C8'0'0, "ROR", OpcodeFlags::FixedMod | OpcodeFlags::FixedReg, 0, 0, { 22, 1, 0, 0, 0, }  }, // ROR GenReg-Opsize, One-Byte, 
  { 0x0'0'D1'D0'0'0, "RCL", OpcodeFlags::FixedMod | OpcodeFlags::FixedReg, 0, 0, { 22, 1, 0, 0, 0, }  }, // RCL GenReg-Opsize, One-Byte, 
  { 0x0'0'D1'D8'0'0, "RCR", OpcodeFlags::FixedMod | OpcodeFlags::FixedReg, 0, 0, { 22, 1, 0, 0, 0, }  }, // RCR GenReg-Opsize, One-Byte, 
  { 0x0'0'D1'E0'0'0, "SHL", OpcodeFlags::FixedMod | OpcodeFlags::FixedReg, 0, 0, { 22, 1, 0, 0, 0, }  }, // SHL GenReg-Opsize, One-Byte, 
  { 0x0'0'D1'E8'0'0, "SHR", OpcodeFlags::FixedMod | OpcodeFlags::FixedReg, 0, 0, { 22, 1, 0, 0, 0, }  }, // SHR GenReg-Opsize, One-Byte, 
  { 0x0'0'D1'F0'0'0, "SHL", OpcodeFlags::FixedMod | OpcodeFlags::FixedReg, 0, 0, { 22, 1, 0, 0, 0, }  }, // SHL GenReg-Opsize, One-Byte, 
  { 0x0'0'D1'F8'0'0, "SAR", OpcodeFlags::FixedMod | OpcodeFlags::FixedReg, 0, 0, { 22, 1, 0, 0, 0, }  }, // SAR GenReg-Opsize, One-Byte, 
  { 0x0'0'D2'00'0'0, "ROL", OpcodeFlags::FixedReg, 0, 0, { 2, 38, 0, 0, 0, }  }, // ROL Memory-Byte, CL-Byte, 
  { 0x0'0'D2'08'0'0, "ROR", OpcodeFlags::FixedReg, 0, 0, { 2, 38, 0, 0, 0, }  }, // ROR Memory-Byte, CL-Byte, 
  { 0x0'0'D2'10'0'0, "RCL", OpcodeFlags::FixedReg, 0, 0, { 2, 38, 0, 0, 0, }  }, // RCL Memory-Byte, CL-Byte, 
  { 0x0'0'D2'18'0'0, "RCR", OpcodeFlags::FixedReg, 0, 0, { 2, 38, 0, 0, 0, }  }, // RCR Memory-Byte, CL-Byte, 
  { 0x0'0'D2'20'0'0, "SHL", OpcodeFlags::FixedReg, 0, 0, { 2, 38, 0, 0, 0, }  }, // SHL Memory-Byte, CL-Byte, 
  { 0x0'0'D2'28'0'0, "SHR", OpcodeFlags::FixedReg, 0, 0, { 2, 38, 0, 0, 0, }  }, // SHR Memory-Byte, CL-Byte, 
  { 0x0'0'D2'30'0'0, "SHL", OpcodeFlags::FixedReg, 0, 0, { 2, 38, 0, 0, 0, }  }, // SHL Memory-Byte, CL-Byte, 
  { 0x0'0'D2'38'0'0, "SAR", OpcodeFlags::FixedReg, 0, 0, { 2, 38, 0, 0, 0, }  }, // SAR Memory-Byte, CL-Byte, 
  { 0x0'0'D2'C0'0'0, "ROL", OpcodeFlags::FixedMod | OpcodeFlags::FixedReg, 0, 0, { 17, 38, 0, 0, 0, }  }, // ROL GenReg-Byte, CL-Byte, 
  { 0x0'0'D2'C8'0'0, "ROR", OpcodeFlags::FixedMod | OpcodeFlags::FixedReg, 0, 0, { 17, 38, 0, 0, 0, }  }, // ROR GenReg-Byte, CL-Byte, 
  { 0x0'0'D2'D0'0'0, "RCL", OpcodeFlags::FixedMod | OpcodeFlags::FixedReg, 0, 0, { 17, 38, 0, 0, 0, }  }, // RCL GenReg-Byte, CL-Byte, 
  { 0x0'0'D2'D8'0'0, "RCR", OpcodeFlags::FixedMod | OpcodeFlags::FixedReg, 0, 0, { 17, 38, 0, 0, 0, }  }, // RCR GenReg-Byte, CL-Byte, 
  { 0x0'0'D2'E0'0'0, "SHL", OpcodeFlags::FixedMod | OpcodeFlags::FixedReg, 0, 0, { 17, 38, 0, 0, 0, }  }, // SHL GenReg-Byte, CL-Byte, 
  { 0x0'0'D2'E8'0'0, "SHR", OpcodeFlags::FixedMod | OpcodeFlags::FixedReg, 0, 0, { 17, 38, 0, 0, 0, }  }, // SHR GenReg-Byte, CL-Byte, 
  { 0x0'0'D2'F0'0'0, "SHL", OpcodeFlags::FixedMod | OpcodeFlags::FixedReg, 0, 0, { 17, 38, 0, 0, 0, }  }, // SHL GenReg-Byte, CL-Byte, 
  { 0x0'0'D2'F8'0'0, "SAR", OpcodeFlags::FixedMod | OpcodeFlags::FixedReg, 0, 0, { 17, 38, 0, 0, 0, }  }, // SAR GenReg-Byte, CL-Byte, 
  { 0x0'0'D3'00'0'0, "ROL", OpcodeFlags::FixedReg, 0, 0, { 5, 38, 0, 0, 0, }  }, // ROL Memory-Opsize, CL-Byte, 
  { 0x0'0'D3'08'0'0, "ROR", OpcodeFlags::FixedReg, 0, 0, { 5, 38, 0, 0, 0, }  }, // ROR Memory-Opsize, CL-Byte, 
  { 0x0'0'D3'10'0'0, "RCL", OpcodeFlags::FixedReg, 0, 0, { 5, 38, 0, 0, 0, }  }, // RCL Memory-Opsize, CL-Byte, 
  { 0x0'0'D3'18'0'0, "RCR", OpcodeFlags::FixedReg, 0, 0, { 5, 38, 0, 0, 0, }  }, // RCR Memory-Opsize, CL-Byte, 
  { 0x0'0'D3'20'0'0, "SHL", OpcodeFlags::FixedReg, 0, 0, { 5, 38, 0, 0, 0, }  }, // SHL Memory-Opsize, CL-Byte, 
  { 0x0'0'D3'28'0'0, "SHR", OpcodeFlags::FixedReg, 0, 0, { 5, 38, 0, 0, 0, }  }, // SHR Memory-Opsize, CL-Byte, 
  { 0x0'0'D3'30'0'0, "SHL", OpcodeFlags::FixedReg, 0, 0, { 5, 38, 0, 0, 0, }  }, // SHL Memory-Opsize, CL-Byte, 
  { 0x0'0'D3'38'0'0, "SAR", OpcodeFlags::FixedReg, 0, 0, { 5, 38, 0, 0, 0, }  }, // SAR Memory-Opsize, CL-Byte, 
  { 0x0'0'D3'C0'0'0, "ROL", OpcodeFlags::FixedMod | OpcodeFlags::FixedReg, 0, 0, { 22, 38, 0, 0, 0, }  }, // ROL GenReg-Opsize, CL-Byte, 
  { 0x0'0'D3'C8'0'0, "ROR", OpcodeFlags::FixedMod | OpcodeFlags::FixedReg, 0, 0, { 22, 38, 0, 0, 0, }  }, // ROR GenReg-Opsize, CL-Byte, 
  { 0x0'0'D3'D0'0'0, "RCL", OpcodeFlags::FixedMod | OpcodeFlags::FixedReg, 0, 0, { 22, 38, 0, 0, 0, }  }, // RCL GenReg-Opsize, CL-Byte, 
  { 0x0'0'D3'D8'0'0, "RCR", OpcodeFlags::FixedMod | OpcodeFlags::FixedReg, 0, 0, { 22, 38, 0, 0, 0, }  }, // RCR GenReg-Opsize, CL-Byte, 
  { 0x0'0'D3'E0'0'0, "SHL", OpcodeFlags::FixedMod | OpcodeFlags::FixedReg, 0, 0, { 22, 38, 0, 0, 0, }  }, // SHL GenReg-Opsize, CL-Byte, 
  { 0x0'0'D3'E8'0'0, "SHR", OpcodeFlags::FixedMod | OpcodeFlags::FixedReg, 0, 0, { 22, 38, 0, 0, 0, }  }, // SHR GenReg-Opsize, CL-Byte, 
  { 0x0'0'D3'F0'0'0, "SHL", OpcodeFlags::FixedMod | OpcodeFlags::FixedReg, 0, 0, { 22, 38, 0, 0, 0, }  }, // SHL GenReg-Opsize, CL-Byte, 
  { 0x0'0'D3'F8'0'0, "SAR", OpcodeFlags::FixedMod | OpcodeFlags::FixedReg, 0, 0, { 22, 38, 0, 0, 0, }  }, // SAR GenReg-Opsize, CL-Byte, 
  { 0x0'0'D4'00'0'0, "AAM", OpcodeFlags::NoModRM | OpcodeFlags::Not64, 0, 0, { 9, 0, 0, 0, 0, }  }, // AAM Immidate-Byte, 
  { 0x0'0'D5'00'0'0, "AAD", OpcodeFlags::NoModRM | OpcodeFlags::Not64, 0, 0, { 9, 0, 0, 0, 0, }  }, // AAD Immidate-Byte, 
  { 0x0'0'D6'00'0'0, "SALC", OpcodeFlags::NoModRM | OpcodeFlags::Not64, 0, 0, { 0, 0, 0, 0, 0, }  }, // SALC 
  { 0x0'0'D7'00'0'0, "XLAT", OpcodeFlags::NoModRM, 0, 0, { 0, 0, 0, 0, 0, }  }, // XLAT 
  { 0x0'0'E0'00'0'0, "LOOPNE", OpcodeFlags::NoModRM | OpcodeFlags::Default64 | OpcodeFlags::Conditional | OpcodeFlags::ExtendedPrefix, 0, 0, { 14, 0, 0, 0, 0, }  }, // LOOPNE RelativeBranch-Byte, 
  { 0x0'0'E0'00'8'0, "LOOPE", OpcodeFlags::NoModRM | OpcodeFlags::Default64 | OpcodeFlags::Conditional | OpcodeFlags::ExtendedPrefix, 0, 0, { 14, 0, 0, 0, 0, }  }, // LOOPE RelativeBranch-Byte, 
  { 0x0'0'E0'00'C'0, "LOOPNE", OpcodeFlags::NoModRM | OpcodeFlags::Default64 | OpcodeFlags::Conditional | OpcodeFlags::ExtendedPrefix, 0, 0, { 14, 0, 0, 0, 0, }  }, // LOOPNE RelativeBranch-Byte, 
  { 0x0'0'E1'00'0'0, "LOOPE", OpcodeFlags::NoModRM | OpcodeFlags::Default64 | OpcodeFlags::Conditional | OpcodeFlags::ExtendedPrefix, 0, 0, { 14, 0, 0, 0, 0, }  }, // LOOPE RelativeBranch-Byte, 
  { 0x0'0'E1'00'8'0, "LOOPE", OpcodeFlags::NoModRM | OpcodeFlags::Default64 | OpcodeFlags::Conditional | OpcodeFlags::ExtendedPrefix, 0, 0, { 14, 0, 0, 0, 0, }  }, // LOOPE RelativeBranch-Byte, 
  { 0x0'0'E1'00'C'0, "LOOPNE", OpcodeFlags::NoModRM | OpcodeFlags::Default64 | OpcodeFlags::Conditional | OpcodeFlags::ExtendedPrefix, 0, 0, { 14, 0, 0, 0, 0, }  }, // LOOPNE RelativeBranch-Byte, 
  { 0x0'0'E2'00'0'0, "LOOP", OpcodeFlags::NoModRM | OpcodeFlags::Default64 | OpcodeFlags::Conditional, 0, 0, { 14, 0, 0, 0, 0, }  }, // LOOP RelativeBranch-Byte, 
  { 0x0'0'E3'00'1'0, "JCXZ", OpcodeFlags::NoModRM | OpcodeFlags::AddressSize | OpcodeFlags::Conditional | OpcodeFlags::ExtendedSize, 0, 0, { 14, 0, 0, 0, 0, } }, // JCXZ RelativeBranch-Byte, 
  { 0x0'0'E4'00'0'0, "IN", OpcodeFlags::NoModRM, 0, 0, { 37, 9, 0, 0, 0, }  }, // IN AL-Byte, Immidate-Byte, 
  { 0x0'0'E5'00'0'0, "IN", OpcodeFlags::NoModRM, 0, 0, { 29, 9, 0, 0, 0, }  }, // IN oAX-Opsize32, Immidate-Byte, 
  { 0x0'0'E6'00'0'0, "OUT", OpcodeFlags::NoModRM, 0, 0, { 9, 37, 0, 0, 0, }  }, // OUT Immidate-Byte, AL-Byte, 
  { 0x0'0'E7'00'0'0, "OUT", OpcodeFlags::NoModRM, 0, 0, { 9, 29, 0, 0, 0, }  }, // OUT Immidate-Byte, oAX-Opsize32, 
  { 0x0'0'E8'00'0'0, "CALL", OpcodeFlags::NoModRM | OpcodeFlags::Default64 | OpcodeFlags::Force64 | OpcodeFlags::Branch | OpcodeFlags::BNDPrefix | OpcodeFlags::ExtendedSize, 0, 0, { 15, 0, 0, 0, 0, }  }, // CALL RelativeBranch-Opsize32, 
  { 0x0'0'E9'00'0'0, "JMP", OpcodeFlags::NoModRM | OpcodeFlags::Force64 | OpcodeFlags::Branch | OpcodeFlags::BNDPrefix | OpcodeFlags::ExtendedSize, 0, 0, { 15, 0, 0, 0, 0, }  }, // JMP RelativeBranch-Opsize32, 
  { 0x0'0'EA'00'0'0, "JMP FAR", OpcodeFlags::NoModRM | OpcodeFlags::Branch | OpcodeFlags::Not64, 0, 0, { 13, 0, 0, 0, 0, }  }, // JMP FAR Immidate-FarPointer, 
  { 0x0'0'EB'00'0'0, "JMP", OpcodeFlags::NoModRM | OpcodeFlags::Force64 | OpcodeFlags::Branch, 0, 0, { 14, 0, 0, 0, 0, }  }, // JMP RelativeBranch-Byte, 
  { 0x0'0'EC'00'0'0, "IN", OpcodeFlags::NoModRM, 0, 0, { 37, 45, 0, 0, 0, }  }, // IN AL-Byte, DX-Word, 
  { 0x0'0'ED'00'0'0, "IN", OpcodeFlags::NoModRM, 0, 0, { 29, 45, 0, 0, 0, }  }, // IN oAX-Opsize32, DX-Word, 
  { 0x0'0'EE'00'0'0, "OUT", OpcodeFlags::NoModRM, 0, 0, { 45, 37, 0, 0, 0, }  }, // OUT DX-Word, AL-Byte, 
  { 0x0'0'EF'00'0'0, "OUT", OpcodeFlags::NoModRM, 0, 0, { 45, 29, 0, 0, 0, }  }, // OUT DX-Word, oAX-Opsize32, 
  { 0x0'0'F1'00'0'0, "INT1", OpcodeFlags::NoModRM, 0, 0, { 0, 0, 0, 0, 0, }  }, // INT1 
  { 0x0'0'F4'00'0'0, "HLT", OpcodeFlags::NoModRM, 0, 0, { 0, 0, 0, 0, 0, }  }, // HLT 
  { 0x0'0'F5'00'0'0, "CMC", OpcodeFlags::NoModRM, 0, 0, { 0, 0, 0, 0, 0, }  }, // CMC 
  { 0x0'0'F6'00'0'0, "TEST", OpcodeFlags::FixedReg, 0, 0, { 2, 9, 0, 0, 0, }  }, // TEST Memory-Byte, Immidate-Byte, 
  { 0x0'0'F6'08'0'0, "TEST", OpcodeFlags::FixedReg, 0, 0, { 2, 9, 0, 0, 0, }  }, // TEST Memory-Byte, Immidate-Byte, 
  { 0x0'0'F6'10'0'0, "NOT", OpcodeFlags::FixedReg, 0, 0, { 2, 0, 0, 0, 0, }  }, // NOT Memory-Byte, 
  { 0x0'0'F6'18'0'0, "NEG", OpcodeFlags::FixedReg, 0, 0, { 2, 0, 0, 0, 0, }  }, // NEG Memory-Byte, 
  { 0x0'0'F6'20'0'0, "MUL", OpcodeFlags::FixedReg, 0, 0, { 2, 0, 0, 0, 0, }  }, // MUL Memory-Byte, 
  { 0x0'0'F6'28'0'0, "IMUL", OpcodeFlags::FixedReg, 0, 0, { 2, 0, 0, 0, 0, }  }, // IMUL Memory-Byte, 
  { 0x0'0'F6'30'0'0, "DIV", OpcodeFlags::FixedReg, 0, 0, { 2, 0, 0, 0, 0, }  }, // DIV Memory-Byte, 
  { 0x0'0'F6'38'0'0, "IDIV", OpcodeFlags::FixedReg, 0, 0, { 2, 0, 0, 0, 0, }  }, // IDIV Memory-Byte, 
  { 0x0'0'F6'C0'0'0, "TEST", OpcodeFlags::FixedMod | OpcodeFlags::FixedReg, 0, 0, { 17, 9, 0, 0, 0, }  }, // TEST GenReg-Byte, Immidate-Byte, 
  { 0x0'0'F6'C8'0'0, "TEST", OpcodeFlags::FixedMod | OpcodeFlags::FixedReg, 0, 0, { 17, 9, 0, 0, 0, }  }, // TEST GenReg-Byte, Immidate-Byte, 
  { 0x0'0'F6'D0'0'0, "NOT", OpcodeFlags::FixedMod | OpcodeFlags::FixedReg, 0, 0, { 17, 0, 0, 0, 0, }  }, // NOT GenReg-Byte, 
  { 0x0'0'F6'D8'0'0, "NEG", OpcodeFlags::FixedMod | OpcodeFlags::FixedReg, 0, 0, { 17, 0, 0, 0, 0, }  }, // NEG GenReg-Byte, 
  { 0x0'0'F6'E0'0'0, "MUL", OpcodeFlags::FixedMod | OpcodeFlags::FixedReg, 0, 0, { 17, 0, 0, 0, 0, }  }, // MUL GenReg-Byte, 
  { 0x0'0'F6'E8'0'0, "IMUL", OpcodeFlags::FixedMod | OpcodeFlags::FixedReg, 0, 0, { 17, 0, 0, 0, 0, }  }, // IMUL GenReg-Byte, 
  { 0x0'0'F6'F0'0'0, "DIV", OpcodeFlags::FixedMod | OpcodeFlags::FixedReg, 0, 0, { 17, 0, 0, 0, 0, }  }, // DIV GenReg-Byte, 
  { 0x0'0'F6'F8'0'0, "IDIV", OpcodeFlags::FixedMod | OpcodeFlags::FixedReg, 0, 0, { 17, 0, 0, 0, 0, }  }, // IDIV GenReg-Byte, 
  { 0x0'0'F7'00'0'0, "TEST", OpcodeFlags::FixedReg, 0, 0, { 5, 12, 0, 0, 0, }  }, // TEST Memory-Opsize, Immidate-Opsize32, 
  { 0x0'0'F7'08'0'0, "TEST", OpcodeFlags::FixedReg, 0, 0, { 5, 12, 0, 0, 0, }  }, // TEST Memory-Opsize, Immidate-Opsize32, 
  { 0x0'0'F7'10'0'0, "NOT", OpcodeFlags::FixedReg, 0, 0, { 5, 0, 0, 0, 0, }  }, // NOT Memory-Opsize, 
  { 0x0'0'F7'18'0'0, "NEG", OpcodeFlags::FixedReg, 0, 0, { 5, 0, 0, 0, 0, }  }, // NEG Memory-Opsize, 
  { 0x0'0'F7'20'0'0, "MUL", OpcodeFlags::FixedReg, 0, 0, { 5, 0, 0, 0, 0, }  }, // MUL Memory-Opsize, 
  { 0x0'0'F7'28'0'0, "IMUL", OpcodeFlags::FixedReg, 0, 0, { 5, 0, 0, 0, 0, }  }, // IMUL Memory-Opsize, 
  { 0x0'0'F7'30'0'0, "DIV", OpcodeFlags::FixedReg, 0, 0, { 5, 0, 0, 0, 0, }  }, // DIV Memory-Opsize, 
  { 0x0'0'F7'38'0'0, "IDIV", OpcodeFlags::FixedReg, 0, 0, { 5, 0, 0, 0, 0, }  }, // IDIV Memory-Opsize, 
  { 0x0'0'F7'C0'0'0, "TEST", OpcodeFlags::FixedMod | OpcodeFlags::FixedReg, 0, 0, { 22, 12, 0, 0, 0, }  }, // TEST GenReg-Opsize, Immidate-Opsize32, 
  { 0x0'0'F7'C8'0'0, "TEST", OpcodeFlags::FixedMod | OpcodeFlags::FixedReg, 0, 0, { 22, 12, 0, 0, 0, }  }, // TEST GenReg-Opsize, Immidate-Opsize32, 
  { 0x0'0'F7'D0'0'0, "NOT", OpcodeFlags::FixedMod | OpcodeFlags::FixedReg, 0, 0, { 22, 0, 0, 0, 0, }  }, // NOT GenReg-Opsize, 
  { 0x0'0'F7'D8'0'0, "NEG", OpcodeFlags::FixedMod | OpcodeFlags::FixedReg, 0, 0, { 22, 0, 0, 0, 0, }  }, // NEG GenReg-Opsize, 
  { 0x0'0'F7'E0'0'0, "MUL", OpcodeFlags::FixedMod | OpcodeFlags::FixedReg, 0, 0, { 22, 0, 0, 0, 0, }  }, // MUL GenReg-Opsize, 
  { 0x0'0'F7'E8'0'0, "IMUL", OpcodeFlags::FixedMod | OpcodeFlags::FixedReg, 0, 0, { 22, 0, 0, 0, 0, }  }, // IMUL GenReg-Opsize, 
  { 0x0'0'F7'F0'0'0, "DIV", OpcodeFlags::FixedMod | OpcodeFlags::FixedReg, 0, 0, { 22, 0, 0, 0, 0, }  }, // DIV GenReg-Opsize, 
  { 0x0'0'F7'F8'0'0, "IDIV", OpcodeFlags::FixedMod | OpcodeFlags::FixedReg, 0, 0, { 22, 0, 0, 0, 0, }  }, // IDIV GenReg-Opsize, 
  { 0x0'0'F8'00'0'0, "CLC", OpcodeFlags::NoModRM, 0, 0, { 0, 0, 0, 0, 0, }  }, // CLC 
  { 0x0'0'F9'00'0'0, "STC", OpcodeFlags::NoModRM, 0, 0, { 0, 0, 0, 0, 0, }  }, // STC 
  { 0x0'0'FA'00'0'0, "CLI", OpcodeFlags::NoModRM, 0, 0, { 0, 0, 0, 0, 0, }  }, // CLI 
  { 0x0'0'FB'00'0'0, "STI", OpcodeFlags::NoModRM, 0, 0, { 0, 0, 0, 0, 0, }  }, // STI 
  { 0x0'0'FC'00'0'0, "CLD", OpcodeFlags::NoModRM, 0, 0, { 0, 0, 0, 0, 0, }  }, // CLD 
  { 0x0'0'FD'00'0'0, "STD", OpcodeFlags::NoModRM, 0, 0, { 0, 0, 0, 0, 0, }  }, // STD 
  { 0x0'0'FE'00'0'0, "INC", OpcodeFlags::FixedReg, 0, 0, { 2, 0, 0, 0, 0, }  }, // INC Memory-Byte, 
  { 0x0'0'FE'08'0'0, "DEC", OpcodeFlags::FixedReg, 0, 0, { 2, 0, 0, 0, 0, }  }, // DEC Memory-Byte, 
  { 0x0'0'FE'C0'0'0, "INC", OpcodeFlags::FixedMod | OpcodeFlags::FixedReg, 0, 0, { 17, 0, 0, 0, 0, }  }, // INC GenReg-Byte, 
  { 0x0'0'FE'C8'0'0, "DEC", OpcodeFlags::FixedMod | OpcodeFlags::FixedReg, 0, 0, { 17, 0, 0, 0, 0, }  }, // DEC GenReg-Byte, 
  { 0x0'0'FF'00'0'0, "INC", OpcodeFlags::FixedReg, 0, 0, { 5, 0, 0, 0, 0, }  }, // INC Memory-Opsize, 
  { 0x0'0'FF'08'0'0, "DEC", OpcodeFlags::FixedReg, 0, 0, { 5, 0, 0, 0, 0, }  }, // DEC Memory-Opsize, 
  { 0x0'0'FF'10'0'0, "CALL", OpcodeFlags::FixedReg | OpcodeFlags::Default64 | OpcodeFlags::Branch | OpcodeFlags::BNDPrefix, 0, 0, { 5, 0, 0, 0, 0, }  }, // CALL Memory-Opsize, 
  { 0x0'0'FF'18'0'0, "CALL FAR", OpcodeFlags::FixedReg | OpcodeFlags::Branch, 0, 0, { 6, 0, 0, 0, 0, }  }, // CALL FAR Memory-FarPointer, 
  { 0x0'0'FF'20'0'0, "JMP", OpcodeFlags::FixedReg | OpcodeFlags::Default64 | OpcodeFlags::Branch | OpcodeFlags::BNDPrefix, 0, 0, { 5, 0, 0, 0, 0, }  }, // JMP Memory-Opsize, 
  { 0x0'0'FF'28'0'0, "JMP FAR", OpcodeFlags::FixedReg | OpcodeFlags::Branch, 0, 0, { 6, 0, 0, 0, 0, }  }, // JMP FAR Memory-FarPointer, 
  { 0x0'0'FF'30'0'0, "PUSH", OpcodeFlags::FixedReg | OpcodeFlags::Default64, 0, 0, { 5, 0, 0, 0, 0, }  }, // PUSH Memory-Opsize, 
  { 0x0'0'FF'C0'0'0, "INC", OpcodeFlags::FixedMod | OpcodeFlags::FixedReg, 0, 0, { 22, 0, 0, 0, 0, }  }, // INC GenReg-Opsize, 
  { 0x0'0'FF'C8'0'0, "DEC", OpcodeFlags::FixedMod | OpcodeFlags::FixedReg, 0, 0, { 22, 0, 0, 0, 0, }  }, // DEC GenReg-Opsize, 
  { 0x0'0'FF'D0'0'0, "CALL", OpcodeFlags::FixedMod | OpcodeFlags::FixedReg | OpcodeFlags::Default64 | OpcodeFlags::Branch | OpcodeFlags::BNDPrefix, 0, 0, { 22, 0, 0, 0, 0, }  }, // CALL GenReg-Opsize, 
  { 0x0'0'FF'E0'0'0, "JMP", OpcodeFlags::FixedMod | OpcodeFlags::FixedReg | OpcodeFlags::Default64 | OpcodeFlags::Branch | OpcodeFlags::BNDPrefix, 0, 0, { 22, 0, 0, 0, 0, }  }, // JMP GenReg-Opsize, 
  { 0x0'0'FF'F0'0'0, "PUSH", OpcodeFlags::FixedMod | OpcodeFlags::FixedReg | OpcodeFlags::Default64, 0, 0, { 22, 0, 0, 0, 0, }  }, // PUSH GenReg-Opsize, 
  { 0x0'1'20'00'0'0, "MOV", OpcodeFlags::NoModRM | OpcodeFlags::Default64 | OpcodeFlags::ExtendedSize | OpcodeFlags::Not64, 0, 0, { 19, 24, 0, 0, 0, }  }, // MOV GenReg-DoubleWord, ControlReg-DoubleWord, 
  { 0x0'1'20'00'3'0, "MOV", OpcodeFlags::NoModRM | OpcodeFlags::Default64 | OpcodeFlags::ExtendedSize | OpcodeFlags::Mode64, 0, 0, { 20, 24, 0, 0, 0, }  }, // MOV GenReg-QuadWord, ControlReg-DoubleWord, 
  { 0x0'1'21'00'0'0, "MOV", OpcodeFlags::NoModRM | OpcodeFlags::Default64 | OpcodeFlags::ExtendedSize | OpcodeFlags::Not64, 0, 0, { 19, 25, 0, 0, 0, }  }, // MOV GenReg-DoubleWord, DebugReg-DoubleWord, 
  { 0x0'1'21'00'3'0, "MOV", OpcodeFlags::NoModRM | OpcodeFlags::Default64 | OpcodeFlags::ExtendedSize | OpcodeFlags::Mode64, 0, 0, { 20, 25, 0, 0, 0, }  }, // MOV GenReg-QuadWord, DebugReg-DoubleWord, 
  { 0x0'1'22'00'0'0, "MOV", OpcodeFlags::NoModRM | OpcodeFlags::Default64 | OpcodeFlags::ExtendedSize | OpcodeFlags::Not64, 0, 0, { 24, 19, 0, 0, 0, }  }, // MOV ControlReg-DoubleWord, GenReg-DoubleWord, 
  { 0x0'1'22'00'3'0, "MOV", OpcodeFlags::NoModRM | OpcodeFlags::Default64 | OpcodeFlags::ExtendedSize | OpcodeFlags::Mode64, 0, 0, { 24, 20, 0, 0, 0, }  }, // MOV ControlReg-DoubleWord, GenReg-QuadWord, 
  { 0x0'1'23'00'0'0, "MOV", OpcodeFlags::NoModRM | OpcodeFlags::Default64 | OpcodeFlags::ExtendedSize | OpcodeFlags::Not64, 0, 0, { 25, 19, 0, 0, 0, }  }, // MOV DebugReg-DoubleWord, GenReg-DoubleWord, 
  { 0x0'1'23'00'3'0, "MOV", OpcodeFlags::NoModRM | OpcodeFlags::Default64 | OpcodeFlags::ExtendedSize | OpcodeFlags::Mode64, 0, 0, { 25, 20, 0, 0, 0, }  }, // MOV DebugReg-DoubleWord, GenReg-QuadWord, 
  { 0x0'1'80'00'0'0, "JO", OpcodeFlags::NoModRM | OpcodeFlags::Force64 | OpcodeFlags::Conditional | OpcodeFlags::BNDPrefix | OpcodeFlags::ExtendedSize, 0, 0, { 15, 0, 0, 0, 0, }  }, // JO RelativeBranch-Opsize32, 
  { 0x0'1'81'00'0'0, "JNO", OpcodeFlags::NoModRM | OpcodeFlags::Force64 | OpcodeFlags::Conditional | OpcodeFlags::BNDPrefix | OpcodeFlags::ExtendedSize, 0, 0, { 15, 0, 0, 0, 0, }  }, // JNO RelativeBranch-Opsize32, 
  { 0x0'1'82'00'0'0, "JB", OpcodeFlags::NoModRM | OpcodeFlags::Force64 | OpcodeFlags::Conditional | OpcodeFlags::BNDPrefix | OpcodeFlags::ExtendedSize, 0, 0, { 15, 0, 0, 0, 0, }  }, // JB RelativeBranch-Opsize32, 
  { 0x0'1'83'00'0'0, "JNB", OpcodeFlags::NoModRM | OpcodeFlags::Force64 | OpcodeFlags::Conditional | OpcodeFlags::BNDPrefix | OpcodeFlags::ExtendedSize, 0, 0, { 15, 0, 0, 0, 0, }  }, // JNB RelativeBranch-Opsize32, 
  { 0x0'1'84'00'0'0, "JZ", OpcodeFlags::NoModRM | OpcodeFlags::Force64 | OpcodeFlags::Conditional | OpcodeFlags::BNDPrefix | OpcodeFlags::ExtendedSize, 0, 0, { 15, 0, 0, 0, 0, }  }, // JZ RelativeBranch-Opsize32, 
  { 0x0'1'85'00'0'0, "JNZ", OpcodeFlags::NoModRM | OpcodeFlags::Force64 | OpcodeFlags::Conditional | OpcodeFlags::BNDPrefix | OpcodeFlags::ExtendedSize, 0, 0, { 15, 0, 0, 0, 0, }  }, // JNZ RelativeBranch-Opsize32, 
  { 0x0'1'86'00'0'0, "JBE", OpcodeFlags::NoModRM | OpcodeFlags::Force64 | OpcodeFlags::Conditional | OpcodeFlags::BNDPrefix | OpcodeFlags::ExtendedSize, 0, 0, { 15, 0, 0, 0, 0, }  }, // JBE RelativeBranch-Opsize32, 
  { 0x0'1'87'00'0'0, "JNBE", OpcodeFlags::NoModRM | OpcodeFlags::Force64 | OpcodeFlags::Conditional | OpcodeFlags::BNDPrefix | OpcodeFlags::ExtendedSize, 0, 0, { 15, 0, 0, 0, 0, }  }, // JNBE RelativeBranch-Opsize32, 
  { 0x0'1'88'00'0'0, "JS", OpcodeFlags::NoModRM | OpcodeFlags::Force64 | OpcodeFlags::Conditional | OpcodeFlags::BNDPrefix | OpcodeFlags::ExtendedSize, 0, 0, { 15, 0, 0, 0, 0, }  }, // JS RelativeBranch-Opsize32, 
  { 0x0'1'89'00'0'0, "JNS", OpcodeFlags::NoModRM | OpcodeFlags::Force64 | OpcodeFlags::Conditional | OpcodeFlags::BNDPrefix | OpcodeFlags::ExtendedSize, 0, 0, { 15, 0, 0, 0, 0, }  }, // JNS RelativeBranch-Opsize32, 
  { 0x0'1'8A'00'0'0, "JP", OpcodeFlags::NoModRM | OpcodeFlags::Force64 | OpcodeFlags::Conditional | OpcodeFlags::BNDPrefix | OpcodeFlags::ExtendedSize, 0, 0, { 15, 0, 0, 0, 0, }  }, // JP RelativeBranch-Opsize32, 
  { 0x0'1'8B'00'0'0, "JNP", OpcodeFlags::NoModRM | OpcodeFlags::Force64 | OpcodeFlags::Conditional | OpcodeFlags::BNDPrefix | OpcodeFlags::ExtendedSize, 0, 0, { 15, 0, 0, 0, 0, }  }, // JNP RelativeBranch-Opsize32, 
  { 0x0'1'8C'00'0'0, "JL", OpcodeFlags::NoModRM | OpcodeFlags::Force64 | OpcodeFlags::Conditional | OpcodeFlags::BNDPrefix | OpcodeFlags::ExtendedSize, 0, 0, { 15, 0, 0, 0, 0, }  }, // JL RelativeBranch-Opsize32, 
  { 0x0'1'8D'00'0'0, "JNL", OpcodeFlags::NoModRM | OpcodeFlags::Force64 | OpcodeFlags::Conditional | OpcodeFlags::BNDPrefix | OpcodeFlags::ExtendedSize, 0, 0, { 15, 0, 0, 0, 0, }  }, // JNL RelativeBranch-Opsize32, 
  { 0x0'1'8E'00'0'0, "JLE", OpcodeFlags::NoModRM | OpcodeFlags::Force64 | OpcodeFlags::Conditional | OpcodeFlags::BNDPrefix | OpcodeFlags::ExtendedSize, 0, 0, { 15, 0, 0, 0, 0, }  }, // JLE RelativeBranch-Opsize32, 
  { 0x0'1'8F'00'0'0, "JNLE", OpcodeFlags::NoModRM | OpcodeFlags::Force64 | OpcodeFlags::Conditional | OpcodeFlags::BNDPrefix | OpcodeFlags::ExtendedSize, 0, 0, { 15, 0, 0, 0, 0, }  }, // JNLE RelativeBranch-Opsize32, 
  { 0x0'1'A0'00'0'0, "PUSH", OpcodeFlags::NoModRM | OpcodeFlags::Default64, 0, 0, { 51, 0, 0, 0, 0, }  }, // PUSH FS-Word, 
  { 0x0'1'A1'00'0'0, "POP", OpcodeFlags::NoModRM | OpcodeFlags::Default64, 0, 0, { 51, 0, 0, 0, 0, }  }, // POP FS-Word, 
  { 0x0'1'A8'00'0'0, "PUSH", OpcodeFlags::NoModRM | OpcodeFlags::Default64, 0, 0, { 50, 0, 0, 0, 0, }  }, // PUSH GS-Word, 
  { 0x0'1'A9'00'0'0, "POP", OpcodeFlags::NoModRM | OpcodeFlags::Default64, 0, 0, { 50, 0, 0, 0, 0, }  }, // POP GS-Word, 
  { 0x0'1'AF'00'0'0, "IMUL", OpcodeFlags::None, 0, 0, { 21, 5, 0, 0, 0, }  }, // IMUL GenReg-Opsize, Memory-Opsize, 
  { 0x0'1'AF'C0'0'0, "IMUL", OpcodeFlags::FixedMod, 0, 0, { 21, 22, 0, 0, 0, }  }, // IMUL GenReg-Opsize, GenReg-Opsize, 
  { 0xF'F'FF'FF'F'F, "(BAD)", OpcodeFlags::None, 0, 0, { 0, 0, 0, 0, 0, } }, // Invalid or unknown opcode
};

const OpcodeData* const OpcodeListBegin = std::begin(OpcodeList);
const OpcodeData* const OpcodeListEnd = std::end(OpcodeList);
const OpcodeData* const OpcodeBad = OpcodeListEnd - 1;

} //namespace X86Disasm
