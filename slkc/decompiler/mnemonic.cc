#include "mnemonic.hh"

using namespace Slake;

std::unordered_map<Opcode, const char*> Decompiler::mnemonics = {
	{ Opcode::NOP, "NOP" },
	{ Opcode::PUSH, "PUSH" },
	{ Opcode::POP, "POP" },
	{ Opcode::LOAD, "LOAD" },
	{ Opcode::RLOAD, "RLOAD" },
	{ Opcode::STORE, "STORE" },
	{ Opcode::LLOAD, "LLOAD" },
	{ Opcode::LSTORE, "LSTORE" },
	{ Opcode::EXPAND, "EXPAND" },
	{ Opcode::SHRINK, "SHRINK" },
	{ Opcode::ENTER, "ENTER" },
	{ Opcode::LEAVE, "LEAVE" },
	{ Opcode::ADD, "ADD" },
	{ Opcode::SUB, "SUB" },
	{ Opcode::MUL, "MUL" },
	{ Opcode::DIV, "DIV" },
	{ Opcode::MOD, "MOD" },
	{ Opcode::AND, "AND" },
	{ Opcode::OR, "OR" },
	{ Opcode::XOR, "XOR" },
	{ Opcode::LAND, "LAND" },
	{ Opcode::LOR, "LOR" },
	{ Opcode::EQ, "EQ" },
	{ Opcode::NEQ, "NEQ" },
	{ Opcode::LT, "LT" },
	{ Opcode::GT, "GT" },
	{ Opcode::LTEQ, "LTEQ" },
	{ Opcode::GTEQ, "GTEQ" },
	{ Opcode::REV, "REV" },
	{ Opcode::NOT, "NOT" },
	{ Opcode::INC, "INC" },
	{ Opcode::DEC, "DEC" },
	{ Opcode::NEG, "NEG" },
	{ Opcode::LSH, "LSH" },
	{ Opcode::RSH, "RSH" },
	{ Opcode::JMP, "JMP" },
	{ Opcode::JT, "JT" },
	{ Opcode::JF, "JF" },
	{ Opcode::CALL, "CALL" },
	{ Opcode::ACALL, "ACALL" },
	{ Opcode::RET, "RET" },
	{ Opcode::SYSCALL, "SYSCALL" },
	{ Opcode::ASYSCALL, "ASYSCALL" },
	{ Opcode::LRET, "LRET" },
	{ Opcode::THROW, "THROW" },
	{ Opcode::PUSHXH, "PUSHXH" },
	{ Opcode::ABORT, "ABORT" }
};
