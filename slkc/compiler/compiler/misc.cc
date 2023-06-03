#include "misc.hh"

#include <slkparse.hh>

using namespace Slake;
using namespace Slake::Compiler;

void State::compileArg(std::shared_ptr<Expr> expr) {
	compileRightExpr(expr);
	fnDefs[currentFn]->insertIns({ Opcode::SARG, {} });
}

void State::compileTypeName(std::fstream &fs, std::shared_ptr<TypeName> j) {
	_writeValue(_tnKind2vtMap.at(j->kind), fs);
	switch (j->kind) {
		case TypeNameKind::ARRAY: {
			auto tn = std::static_pointer_cast<ArrayTypeName>(j);
			compileTypeName(fs, tn->type);
			break;
		}
		case TypeNameKind::MAP: {
			auto tn = std::static_pointer_cast<MapTypeName>(j);
			compileTypeName(fs, tn->keyType);
			compileTypeName(fs, tn->valueType);
			break;
		}
		case TypeNameKind::CUSTOM: {
			auto tn = std::static_pointer_cast<CustomTypeName>(j);
			auto t = Scope::getCustomType(tn);
			if (!t)
				throw parser::syntax_error(tn->getLocation(), "Type `" + std::to_string(*tn) + "' was not defined");
			// ! FIXME
			writeValue(tn->typeRef, fs);
			break;
		}
	}
}

void State::writeIns(Opcode opcode, std::fstream &fs, std::initializer_list<std::shared_ptr<Expr>> operands) {
	assert(operands.size() <= 3);
	_writeValue(SlxFmt::InsHeader{ opcode, (uint8_t)operands.size() }, fs);
	for (auto &i : operands)
		writeValue(i, fs);
}

void State::writeValue(std::shared_ptr<Expr> src, std::fstream &fs) {
	SlxFmt::ValueDesc vd = {};
	switch (src->getExprKind()) {
		case ExprKind::LITERAL: {
			auto literalExpr = std::static_pointer_cast<LiteralExpr>(src);
			vd.type = _lt2vtMap.at(literalExpr->getLiteralType());
			_writeValue(vd, fs);
			switch (literalExpr->getLiteralType()) {
				case LT_INT:
					_writeValue(std::static_pointer_cast<IntLiteralExpr>(literalExpr)->data, fs);
					break;
				case LT_UINT:
					_writeValue(std::static_pointer_cast<UIntLiteralExpr>(literalExpr)->data, fs);
					break;
				case LT_LONG:
					_writeValue(std::static_pointer_cast<LongLiteralExpr>(literalExpr)->data, fs);
					break;
				case LT_ULONG:
					_writeValue(std::static_pointer_cast<ULongLiteralExpr>(literalExpr)->data, fs);
					break;
				case LT_FLOAT:
					_writeValue(std::static_pointer_cast<FloatLiteralExpr>(literalExpr)->data, fs);
					break;
				case LT_DOUBLE:
					_writeValue(std::static_pointer_cast<DoubleLiteralExpr>(literalExpr)->data, fs);
					break;
				case LT_BOOL: {
					_writeValue(std::static_pointer_cast<BoolLiteralExpr>(literalExpr)->data, fs);
					break;
				}
				case LT_STRING: {
					auto expr = std::static_pointer_cast<StringLiteralExpr>(literalExpr);
					_writeValue((uint32_t)expr->data.size(), fs);
					_writeValue(*(expr->data.c_str()), (std::streamsize)expr->data.size(), fs);
					break;
				}
			}
			break;
		}
		case ExprKind::REF: {
			auto expr = std::static_pointer_cast<RefExpr>(src);
			vd.type = SlxFmt::ValueType::REF;
			_writeValue(vd, fs);

			for (auto &i = expr; i; i = i->next) {
				SlxFmt::ScopeRefDesc srd = { 0 };
				if (i->next)
					srd.flags |= SlxFmt::SRD_NEXT;
				srd.lenName = i->name.length();
				_writeValue(srd, fs);
				_writeValue(*(i->name.c_str()), i->name.length(), fs);
			}
			break;
		}
		case ExprKind::ARRAY: {
			auto expr = std::static_pointer_cast<ArrayExpr>(src);
			vd.type = SlxFmt::ValueType::ARRAY;
			_writeValue(vd, fs);

			SlxFmt::ArrayDesc ard;
			ard.nMembers = expr->elements.size();
			_writeValue(ard, fs);

			for (auto &i : expr->elements) {
				auto constExpr = evalConstExpr(i);
				if (!constExpr)
					throw parser::syntax_error(i->getLocation(), "Expression cannot be evaluated in compile time");
				writeValue(constExpr, fs);
			}
			break;
		}
		default:
			throw parser::syntax_error(src->getLocation(), "Expression cannot be evaluated in compile time");
	}
}

const std::unordered_map<LiteralType, SlxFmt::ValueType> Slake::Compiler::_lt2vtMap = {
	{ LT_INT, SlxFmt::ValueType::I32 },
	{ LT_UINT, SlxFmt::ValueType::U32 },
	{ LT_LONG, SlxFmt::ValueType::I64 },
	{ LT_ULONG, SlxFmt::ValueType::U64 },
	{ LT_FLOAT, SlxFmt::ValueType::F32 },
	{ LT_DOUBLE, SlxFmt::ValueType::F64 },
	{ LT_BOOL, SlxFmt::ValueType::BOOL },
	{ LT_STRING, SlxFmt::ValueType::STRING },
	{ LT_NULL, SlxFmt::ValueType::NONE }
};

const std::unordered_map<LiteralType, TypeNameKind> Slake::Compiler::_lt2tnKindMap = {
	{ LT_INT, TypeNameKind::I32 },
	{ LT_UINT, TypeNameKind::U32 },
	{ LT_LONG, TypeNameKind::I64 },
	{ LT_ULONG, TypeNameKind::U64 },
	{ LT_FLOAT, TypeNameKind::F32 },
	{ LT_DOUBLE, TypeNameKind::F64 },
	{ LT_BOOL, TypeNameKind::BOOL },
	{ LT_STRING, TypeNameKind::STRING },
	{ LT_NULL, TypeNameKind::NONE }
};

const std::unordered_map<TypeNameKind, SlxFmt::ValueType> Slake::Compiler::_tnKind2vtMap = {
	{ TypeNameKind::I8, SlxFmt::ValueType::I8 },
	{ TypeNameKind::I16, SlxFmt::ValueType::I16 },
	{ TypeNameKind::I32, SlxFmt::ValueType::I32 },
	{ TypeNameKind::I64, SlxFmt::ValueType::I64 },
	{ TypeNameKind::U8, SlxFmt::ValueType::U8 },
	{ TypeNameKind::U16, SlxFmt::ValueType::U16 },
	{ TypeNameKind::U32, SlxFmt::ValueType::U32 },
	{ TypeNameKind::U64, SlxFmt::ValueType::U64 },
	{ TypeNameKind::F32, SlxFmt::ValueType::F32 },
	{ TypeNameKind::F64, SlxFmt::ValueType::F64 },
	{ TypeNameKind::BOOL, SlxFmt::ValueType::BOOL },
	{ TypeNameKind::STRING, SlxFmt::ValueType::STRING },
	{ TypeNameKind::NONE, SlxFmt::ValueType::NONE },
	{ TypeNameKind::ANY, SlxFmt::ValueType::ANY },
	{ TypeNameKind::CUSTOM, SlxFmt::ValueType::OBJECT },
	{ TypeNameKind::ARRAY, SlxFmt::ValueType::ARRAY },
	{ TypeNameKind::MAP, SlxFmt::ValueType::MAP }
};

const std::unordered_map<UnaryOp, Opcode> Slake::Compiler::_unaryOp2opcodeMap = {
	{ UnaryOp::INC_F, Opcode::INCF },
	{ UnaryOp::INC_B, Opcode::INCB },
	{ UnaryOp::DEC_F, Opcode::DECF },
	{ UnaryOp::DEC_B, Opcode::DECB },
	{ UnaryOp::NEG, Opcode::NEG },
	{ UnaryOp::NOT, Opcode::NOT },
	{ UnaryOp::REV, Opcode::REV }
};

const std::unordered_map<BinaryOp, Opcode> Slake::Compiler::_binaryOp2opcodeMap = {
	{ BinaryOp::ADD, Opcode::ADD },
	{ BinaryOp::SUB, Opcode::SUB },
	{ BinaryOp::MUL, Opcode::MUL },
	{ BinaryOp::DIV, Opcode::DIV },
	{ BinaryOp::MOD, Opcode::MOD },
	{ BinaryOp::AND, Opcode::AND },
	{ BinaryOp::OR, Opcode::OR },
	{ BinaryOp::XOR, Opcode::XOR },
	{ BinaryOp::LAND, Opcode::LAND },
	{ BinaryOp::LOR, Opcode::LOR },
	{ BinaryOp::LSH, Opcode::LSH },
	{ BinaryOp::RSH, Opcode::RSH },
	{ BinaryOp::EQ, Opcode::EQ },
	{ BinaryOp::NEQ, Opcode::NEQ },
	{ BinaryOp::GTEQ, Opcode::GTEQ },
	{ BinaryOp::LTEQ, Opcode::LTEQ },
	{ BinaryOp::GT, Opcode::GT },
	{ BinaryOp::LT, Opcode::LT },
	{ BinaryOp::ADD_ASSIGN, Opcode::ADD },
	{ BinaryOp::SUB_ASSIGN, Opcode::SUB },
	{ BinaryOp::MUL_ASSIGN, Opcode::MUL },
	{ BinaryOp::DIV_ASSIGN, Opcode::DIV },
	{ BinaryOp::MOD_ASSIGN, Opcode::MOD },
	{ BinaryOp::AND_ASSIGN, Opcode::AND },
	{ BinaryOp::OR_ASSIGN, Opcode::OR },
	{ BinaryOp::XOR_ASSIGN, Opcode::XOR },
	{ BinaryOp::LSH_ASSIGN, Opcode::LSH },
	{ BinaryOp::RSH_ASSIGN, Opcode::RSH }
};
