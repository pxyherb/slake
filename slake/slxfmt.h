///
/// @file slx.h
/// @author CodesBuilder (2602783536@qq.com)
/// @brief Definitions for Slake Executable (SLX) format.
/// @version 0.1
/// @date 2022-09-18
///
/// @copyright Copyright (c) 2022 Slake Contributors
///
#ifndef _SLAKE_SLXFMT_H_
#define _SLAKE_SLXFMT_H_

#include <cassert>
#include <cstdint>

#include "opcode.h"

#ifdef _MSC_VER
	#pragma pack(push)
	#pragma pack(1)
#endif

namespace Slake {
	namespace SlxFmt {
		constexpr static uint8_t GENERIC_PARAM_MAX = 16;

		///
		/// @brief IMage Header (IMH)
		///
		struct ImgHeader final {
			uint8_t magic[4];	  // Magic number
			uint8_t flags;		  // Flags
			uint8_t fmtVer;		  // Format version
			uint8_t nImports;	  // Number of imported modules
			uint8_t reserved[2];  // Reserved
		};
		constexpr static uint8_t IMH_MAGIC[4] = { 'S', 'L', 'A', 'X' };

		///
		/// @brief Instruction Header (IH)
		///
		struct InsHeader final {
			Opcode opcode : 14;		// Operation code
			uint8_t nOperands : 2;	// Number of operands

			inline InsHeader() : opcode(Opcode::NOP), nOperands(0) {}
			inline InsHeader(Opcode opcode, uint8_t nOperands) {
				assert((uint8_t)opcode < (1 << 6));
				assert(nOperands < 4);
				this->opcode = opcode;
				this->nOperands = nOperands;
			}
		};

		enum class ValueType : uint8_t {
			NONE = 0,  // None
			ANY,	   // Any
			I8,		   // i8
			I16,	   // i16
			I32,	   // i32
			I64,	   // i64
			U8,		   // u8
			U16,	   // u16
			U32,	   // u32
			U64,	   // u64
			F32,	   // f32
			F64,	   // f64
			STRING,	   // String
			BOOL,	   // Boolean
			ARRAY,	   // Array
			MAP,	   // Map
			OBJECT,	   // Object
			REF,	   // Reference
		};

		/// @brief Value Descriptor (VD)
		struct ValueDesc final {
			ValueType type : 5;	 // Data Type
			uint8_t flags : 3;	 // Flags
		};

		/// @brief Extra attribute for strings
		struct StringExAttr final {
			uint32_t len;  // Length in bytes
		};

		/// @brief Class Type Descriptor (CTD)
		struct ClassTypeDesc final {
			uint8_t flags;			 // Flags
			uint8_t nGenericParams;	 // Number of generic parameters
			uint8_t lenName;		 // Length of name
			uint8_t nImpls;			 // Number of implemented interfaces
		};
		constexpr static uint8_t
			CTD_PUB = 0x01,		  // Public
			CTD_FINAL = 0x02,	  // Final
			CTD_DERIVED = 0x40,	  // Is derived from parent
			CTD_INTERFACE = 0x80  // As a interface
			;

		/// @brief Structure Type Descriptor (STD)
		struct StructTypeDesc final {
			uint8_t flags;		// Flags
			uint8_t lenName;	// Length of name
			uint32_t nMembers;	// Number of members
		};
		struct StructMemberDesc final {
			ValueType type : 5;	  // Data Type
			uint8_t flags : 3;	  // Flags
			uint8_t lenName : 8;  // Name length
		};
		constexpr static uint8_t
			STD_PUB = 0x01	// Public
			;

		/// @brief Function Descriptor (FND)
		struct FnDesc final {
			uint8_t flags : 8;			 // Flags
			uint16_t lenName : 16;		 // Length of name
			uint8_t nGenericParams : 8;	 // Number of generic parameters
			uint8_t nParams : 8;		 // Number of parameters, only used by compilers
			uint32_t lenBody : 24;		 // Length of body
		};
		constexpr static uint8_t
			FND_PUB = 0x01,		  // Public
			FND_FINAL = 0x02,	  // Final
			FND_OVERRIDE = 0x04,  // Override
			FND_STATIC = 0x08,	  // Static
			FND_NATIVE = 0x10,	  // Native
			FND_VARG = 0x80		  // Variable arguments
			;

		/// @brief Variable Descriptonr (VAD)
		struct VarDesc final {
			uint8_t lenName;
			uint8_t flags;
		};
		constexpr static uint8_t
			VAD_PUB = 0x01,		// Public
			VAD_FINAL = 0x02,	// Final
			VAD_STATIC = 0x04,	// Static
			VAD_NATIVE = 0x08,	// Native
			VAD_INIT = 0x80		// Initialized
			;

		constexpr static uint8_t
			SRD_NEXT = 0x01,
			SRD_STATIC = 0x02;
		/// @brief Scope Reference Descriptor (SRD)
		struct ScopeRefDesc final {
			uint8_t lenName : 8;
			uint8_t flags : 4;
			uint8_t nGenericArgs : 4;
		};

		// @brief Array Descriptor (ARD)
		struct ArrayDesc final {
			uint32_t nMembers;
		};

		// @brief Array Descriptor (MPD)
		struct MapDesc final {
			uint32_t nPairs;
		};
	}
}

#ifdef _MSC_VER
	#pragma pack(pop)
#endif

#endif
