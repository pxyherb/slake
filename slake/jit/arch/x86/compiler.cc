#include <cstdint>
#include <cstring>

#include "../../base.hh"

#define MODRM(mod, rm, reg) ((rm) | ((reg) << 3) | ((mod) << 6))
#define SIB(scale, index, base) ((base) | ((index) << 3) | ((scale) << 6))
#define REXPF(w, r, x, b) (0b01000000 | ((w) << 3) | ((r) << 2) | ((x) << 1) | (b))

enum LegacyPrefix : std::uint8_t {
	LPF_NONE = 0,
	LPF_LOCK = 0xf0,
	LPF_REPNZ = 0xf2,
	LPF_REPNE = 0xf2,
	LPF_REP = 0xf3,
	LPF_REPE = 0xf3,
	LPF_REPZ = 0xf3,
	LPF_BND = 0xf2,

	LPF_CS_SEGOD = 0x2e,
	LPF_SS_SEGOD = 0x36,
	LPF_DS_SEGOD = 0x3e,
	LPF_ES_SEGOD = 0x26,
	LPF_FS_SEGOD = 0x64,
	LPF_GS_SEGOD = 0x65,
	LPF_BRANCH_TAKEN = 0x3e,
	LPF_BRANCH_NOT_TAKEN = 0x2e,

	LPF_OPRDSZ_OD = 0x66,
	LPF_ADDRSZ_OD = 0x67
};

enum ModType : std::uint8_t {
	MOD_BASE = 0,
	MOD_DISP8,
	MOD_DISP32,
	MOD_REG
};

enum RegType : std::uint8_t {
	REG_EAX = 0,
	REG_ECX,
	REG_EDX,
	REG_EBX,
	REG_ESP,  // Also represents `none' in SIB byte.
	REG_EBP,
	REG_ESI,
	REG_EDI
};

template <typename T>
std::uint8_t genIns(
	char *dest,
	std::uint32_t opcode,
	std::uint8_t modRm,
	std::uint8_t sib,
	std::uint32_t disp,
	T immediate,
	std::uint8_t rexPrefix,
	std::uint8_t legacyPrefix = LPF_NONE) {
	auto origin = dest;
	char buf[15] = { 0 };
	if (!dest)
		dest = buf;

	// Legacy Prefix
	if (legacyPrefix)
		*(std::uint8_t *)(dest++) = legacyPrefix;

	// Prefix
	if (rexPrefix & 0b01000000)
		*(std::uint8_t *)(dest++) = rexPrefix;

	// Opcode
	if ((opcode & 0xff0000))
		*(std::uint8_t *)(dest++) = opcode & 0xff0000;
	if ((opcode & 0xff00))
		*(std::uint8_t *)(dest++) = opcode & 0xff00;
	*(std::uint8_t *)(dest++) = opcode & 0xff;

	*(std::uint8_t *)(dest++) = modRm;	// Mod R/M

	if ((modRm & 0b111) == 0b100)
		*(std::uint8_t *)(dest++) = sib;  // SIB

	// Displacement
	if ((modRm >> 6) == MOD_DISP8)
		*(std::uint8_t *)(dest++) = disp & 0xff;
	else if ((modRm >> 6) == MOD_DISP32)
		*(std::uint32_t *)dest = disp, dest += sizeof(std::uint32_t);

	if constexpr (!std::is_same<T, nullptr_t>::value) {
		*(T *)dest = immediate;
		dest += sizeof(T);
	}

	return dest - origin;
}

std::uint8_t enterIns[] = {
	0x55,		// push ebp
	0x89, 0xe5	// mov ebp, esp
};
std::uint8_t leaveIns[] = {
	0x89, 0xec,	 // mov esp, ebp
	0x5d,		 // pop ebp
	0xc3		 // ret
};

Slake::ICodePage *Slake::compileFn(FnValue *fn) {
	Slake::ICodePage *codePage;
	std::size_t size;
	// Enter instructions
	std::memcpy(codePage->getPtr(), enterIns, sizeof(enterIns));
	// Leave instructions
	std::memcpy(&(((char *)codePage->getPtr())[size + sizeof(enterIns)]), leaveIns, sizeof(leaveIns));

	return codePage;
}
