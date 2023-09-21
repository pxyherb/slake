#ifndef _SLAKE_OPCODE_H_
#define _SLAKE_OPCODE_H_

#include <cstdint>

namespace slake {
	enum class Opcode : uint16_t {
		NOP = 0,  // No operation

		LOAD,	// Load a value onto stack
		RLOAD,	// Load a value onto stack by an existing reference
		STORE,	// Store a value from stack

		LVAR,  // Create a new local variable

		LVALUE,	 // Load value of a variable

		ENTER,	// Enter frame
		LEAVE,	// Leave frame

		ADD,   // Add
		SUB,   // Subtract
		MUL,   // Multiply
		DIV,   // Divide
		MOD,   // Modulo
		AND,   // AND
		OR,	   // OR
		XOR,   // XOR
		LAND,  // Logical AND
		LOR,   // Logical OR
		EQ,	   // Equal
		NEQ,   // Not Equal
		SEQ,   // Strictly Equal
		SNEQ,  // Strictly Not Equal
		LT,	   // Less than
		GT,	   // Greater than
		LTEQ,  // Less than or equal
		GTEQ,  // Greater than or equal
		LSH,   // Left shift
		RSH,   // Right shift
		SWAP,  // Swap

		REV,   // Bitwise NOT
		NOT,   // Logical NOT
		INCF,  // Forward Increase
		DECF,  // Forward Decrease
		INCB,  // Backward Increase
		DECB,  // Backward Decrease
		NEG,   // Negate

		AT,	 // Subscript

		JMP,  // Jump
		JT,	  // Jump if true
		JF,	  // Jump if false

		PUSHARG,  // Push an value into the argument stack

		CALL,	// Call
		MCALL,	// Method Call
		RET,	// Return

		ACALL,	 // Asynchronous Call
		AMCALL,	 // Asynchronous Method Call
		YIELD,	 // Yield
		AWAIT,	 // Await

		NEW,  // New

		THROW,	  // Throw an exception
		PUSHXH,	  // Push an exception handler
		LEXCEPT,  // Load current exception

		ABORT,	// Abort

		CAST,  // Cast

		TYPEOF,	 // Get type of an object

		CONSTSW, // Constant switch

		OPCODE_MAX
	};
}

#endif
