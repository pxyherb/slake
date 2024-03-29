#include "../compiler.h"
#include <type_traits>
#include <cmath>

using namespace slake::slkc;

struct UnaryOpRegistry {
	slake::Opcode opcode;
	bool lvalueOperand;

	inline UnaryOpRegistry(
		slake::Opcode opcode,
		bool lvalueOperand) : opcode(opcode), lvalueOperand(lvalueOperand) {}
};

static map<UnaryOp, UnaryOpRegistry> _unaryOpRegs = {
	{ UnaryOp::LNot, { slake::Opcode::LNOT, false } },
	{ UnaryOp::Not, { slake::Opcode::NOT, false } },
	{ UnaryOp::IncF, { slake::Opcode::INCF, true } },
	{ UnaryOp::DecF, { slake::Opcode::DECF, true } },
	{ UnaryOp::IncB, { slake::Opcode::INCB, true } },
	{ UnaryOp::DecB, { slake::Opcode::DECB, true } }
};

struct BinaryOpRegistry {
	slake::Opcode opcode;
	bool isLhsLvalue;
	bool isRhsLvalue;

	inline BinaryOpRegistry(
		slake::Opcode opcode,
		bool isLhsLvalue,
		bool isRhsLvalue)
		: opcode(opcode),
		  isLhsLvalue(isLhsLvalue),
		  isRhsLvalue(isRhsLvalue) {}
};

static map<BinaryOp, BinaryOpRegistry> _binaryOpRegs = {
	{ BinaryOp::Add, { slake::Opcode::ADD, false, false } },
	{ BinaryOp::Sub, { slake::Opcode::SUB, false, false } },
	{ BinaryOp::Mul, { slake::Opcode::MUL, false, false } },
	{ BinaryOp::Div, { slake::Opcode::DIV, false, false } },
	{ BinaryOp::Mod, { slake::Opcode::MOD, false, false } },
	{ BinaryOp::And, { slake::Opcode::AND, false, false } },
	{ BinaryOp::Or, { slake::Opcode::OR, false, false } },
	{ BinaryOp::Xor, { slake::Opcode::XOR, false, false } },
	{ BinaryOp::LAnd, { slake::Opcode::LAND, false, false } },
	{ BinaryOp::LOr, { slake::Opcode::LOR, false, false } },
	{ BinaryOp::Lsh, { slake::Opcode::LSH, false, false } },
	{ BinaryOp::Rsh, { slake::Opcode::RSH, false, false } },
	{ BinaryOp::Swap, { slake::Opcode::SWAP, true, true } },

	{ BinaryOp::Assign, { slake::Opcode::NOP, true, false } },
	{ BinaryOp::AssignAdd, { slake::Opcode::NOP, true, false } },
	{ BinaryOp::AssignSub, { slake::Opcode::NOP, true, false } },
	{ BinaryOp::AssignMul, { slake::Opcode::NOP, true, false } },
	{ BinaryOp::AssignDiv, { slake::Opcode::NOP, true, false } },
	{ BinaryOp::AssignMod, { slake::Opcode::NOP, true, false } },
	{ BinaryOp::AssignAnd, { slake::Opcode::NOP, true, false } },
	{ BinaryOp::AssignOr, { slake::Opcode::NOP, true, false } },
	{ BinaryOp::AssignXor, { slake::Opcode::NOP, true, false } },
	{ BinaryOp::AssignLsh, { slake::Opcode::NOP, true, false } },
	{ BinaryOp::AssignRsh, { slake::Opcode::NOP, true, false } },

	{ BinaryOp::Eq, { slake::Opcode::EQ, false, false } },
	{ BinaryOp::Neq, { slake::Opcode::NEQ, false, false } },
	{ BinaryOp::StrictEq, { slake::Opcode::SEQ, false, false } },
	{ BinaryOp::StrictNeq, { slake::Opcode::SNEQ, false, false } },
	{ BinaryOp::Lt, { slake::Opcode::LT, false, false } },
	{ BinaryOp::Gt, { slake::Opcode::GT, false, false } },
	{ BinaryOp::LtEq, { slake::Opcode::LTEQ, false, false } },
	{ BinaryOp::GtEq, { slake::Opcode::GTEQ, false, false } },
	{ BinaryOp::Subscript, { slake::Opcode::AT, false, false } }
};

static map<BinaryOp, BinaryOp> _assignBinaryOpToOrdinaryBinaryOpMap = {
	{ BinaryOp::Assign, BinaryOp::Assign },
	{ BinaryOp::AssignAdd, BinaryOp::Add },
	{ BinaryOp::AssignSub, BinaryOp::Sub },
	{ BinaryOp::AssignMul, BinaryOp::Mul },
	{ BinaryOp::AssignDiv, BinaryOp::Div },
	{ BinaryOp::AssignMod, BinaryOp::Mod },
	{ BinaryOp::AssignAnd, BinaryOp::And },
	{ BinaryOp::AssignOr, BinaryOp::Or },
	{ BinaryOp::AssignXor, BinaryOp::Xor },
	{ BinaryOp::AssignLsh, BinaryOp::Lsh },
	{ BinaryOp::AssignRsh, BinaryOp::Rsh }
};

void Compiler::compileExpr(shared_ptr<ExprNode> expr) {
	slxfmt::SourceLocDesc sld;
	sld.offIns = curFn->body.size();
	sld.line = expr->getLocation().line;
	sld.column = expr->getLocation().column;

	if (auto ce = evalConstExpr(expr); ce) {
		curFn->insertIns(Opcode::STORE, curMajorContext.curMinorContext.evalDest, ce);
		return;
	}

	switch (expr->getExprType()) {
		case ExprType::Unary: {
			auto e = static_pointer_cast<UnaryOpExprNode>(expr);

			uint32_t lhsRegIndex = allocReg();

			compileExpr(
				e->x,
				_unaryOpRegs.at(e->op).lvalueOperand ? EvalPurpose::LValue : EvalPurpose::RValue,
				make_shared<RegRefNode>(lhsRegIndex));
			if (curMajorContext.curMinorContext.evalPurpose == EvalPurpose::Stmt) {
				if (_unaryOpRegs.at(e->op).lvalueOperand) {
					curFn->insertIns(_unaryOpRegs.at(e->op).opcode, make_shared<RegRefNode>(lhsRegIndex), make_shared<RegRefNode>(lhsRegIndex));
				}
			} else {
				curFn->insertIns(_unaryOpRegs.at(e->op).opcode, curMajorContext.curMinorContext.evalDest, make_shared<RegRefNode>(lhsRegIndex, true));
			}

			break;
		}
		case ExprType::Binary: {
			auto e = static_pointer_cast<BinaryOpExprNode>(expr);

			uint32_t lhsRegIndex = allocReg(2);
			uint32_t rhsRegIndex = lhsRegIndex + 1;

			auto lhsType = evalExprType(e->lhs), rhsType = evalExprType(e->rhs);

			compileExpr(
				e->lhs,
				_binaryOpRegs.at(e->op).isLhsLvalue
					? EvalPurpose::LValue
					: EvalPurpose::RValue,
				make_shared<RegRefNode>(lhsRegIndex));

			if (!isSameType(lhsType, rhsType)) {
				if (!areTypesConvertible(rhsType, lhsType))
					throw FatalCompilationError(
						{ e->rhs->getLocation(),
							MessageType::Error,
							"Incompatible initial value type" });

				compileExpr(
					make_shared<CastExprNode>(e->rhs->getLocation(), lhsType, e->rhs),
					_binaryOpRegs.at(e->op).isRhsLvalue
						? EvalPurpose::LValue
						: EvalPurpose::RValue,
					make_shared<RegRefNode>(rhsRegIndex));
			} else
				compileExpr(e->rhs, _binaryOpRegs.at(e->op).isRhsLvalue ? EvalPurpose::LValue : EvalPurpose::RValue, make_shared<RegRefNode>(rhsRegIndex));

			if (isAssignBinaryOp(e->op)) {
				// RHS of the assignment expression.
				if (auto opcode = _binaryOpRegs.at(_assignBinaryOpToOrdinaryBinaryOpMap.at(e->op)).opcode; opcode != Opcode::NOP) {
					uint32_t lhsValueRegIndex = allocReg();

					curFn->insertIns(
						Opcode::STORE,
						make_shared<RegRefNode>(lhsValueRegIndex),
						make_shared<RegRefNode>(lhsRegIndex, true));
					curFn->insertIns(
						opcode,
						make_shared<RegRefNode>(rhsRegIndex),
						make_shared<RegRefNode>(lhsValueRegIndex, true),
						make_shared<RegRefNode>(rhsRegIndex, true));
				}

				// LHS of the assignment expression.
				curFn->insertIns(Opcode::STORE, make_shared<RegRefNode>(lhsRegIndex, true), make_shared<RegRefNode>(rhsRegIndex, true));

				if ((curMajorContext.curMinorContext.evalPurpose != EvalPurpose::Stmt) && (curMajorContext.curMinorContext.evalDest))
					curFn->insertIns(Opcode::STORE, curMajorContext.curMinorContext.evalDest, make_shared<RegRefNode>(rhsRegIndex, true));
			} else if (curMajorContext.curMinorContext.evalPurpose != EvalPurpose::Stmt) {
				curFn->insertIns(
					_binaryOpRegs.at(e->op).opcode,
					curMajorContext.curMinorContext.evalDest,
					make_shared<RegRefNode>(lhsRegIndex, true),
					make_shared<RegRefNode>(rhsRegIndex, true));
			} else
				curFn->insertIns(
					_binaryOpRegs.at(e->op).opcode,
					make_shared<RegRefNode>(lhsRegIndex),
					make_shared<RegRefNode>(lhsRegIndex, true),
					make_shared<RegRefNode>(rhsRegIndex, true));
			break;
		}
		case ExprType::Ternary: {
			auto e = static_pointer_cast<TernaryOpExprNode>(expr);

			uint32_t conditionRegIndex = allocReg();

			auto loc = e->getLocation();
			string falseBranchLabel = "$ternary_" + to_string(loc.line) + "_" + to_string(loc.column) + "_false",
				   endLabel = "$ternary_" + to_string(loc.line) + "_" + to_string(loc.column) + "_end";

			compileExpr(e->condition, EvalPurpose::RValue, make_shared<RegRefNode>(conditionRegIndex));
			if (evalExprType(e->condition)->getTypeId() != Type::Bool)
				curFn->insertIns(
					Opcode::CAST,
					make_shared<RegRefNode>(conditionRegIndex),
					make_shared<BoolTypeNameNode>(e->getLocation(), true),
					make_shared<RegRefNode>(conditionRegIndex, true));

			curFn->insertIns(
				Opcode::JF,
				make_shared<LabelRefNode>(falseBranchLabel),
				make_shared<RegRefNode>(conditionRegIndex, true));

			// Compile the true expression.
			compileExpr(e->x, EvalPurpose::RValue, curMajorContext.curMinorContext.evalDest);
			curFn->insertIns(Opcode::JMP, make_shared<LabelRefNode>(endLabel));

			// Compile the false expression.
			curFn->insertLabel(falseBranchLabel);
			compileExpr(e->y, EvalPurpose::RValue, curMajorContext.curMinorContext.evalDest);

			curFn->insertLabel(endLabel);
			break;
		}
		case ExprType::Match: {
			auto e = static_pointer_cast<MatchExprNode>(expr);

			auto loc = e->getLocation();

			string labelPrefix = "$match_" + to_string(loc.line) + "_" + to_string(loc.column),
				   condLocalVarName = labelPrefix + "_cond",
				   defaultLabel = labelPrefix + "_label",
				   endLabel = labelPrefix + "_end";

			uint32_t matcheeRegIndex = allocReg();

			// Create a local variable to store result of the condition expression.
			compileExpr(e->condition, EvalPurpose::RValue, make_shared<RegRefNode>(matcheeRegIndex));

			pair<shared_ptr<ExprNode>, shared_ptr<ExprNode>> defaultCase;

			for (auto i : e->cases) {
				string caseEndLabel = "$match_" + to_string(i.second->getLocation().line) + "_" + to_string(i.second->getLocation().column) + "_caseEnd";

				if (!i.first) {
					if (defaultCase.second)
						// The default case is already exist.
						throw FatalCompilationError(
							{ i.second->getLocation(),
								MessageType::Error,
								"Duplicated default case" });
					defaultCase = i;
				}

				uint32_t conditionRegIndex = allocReg();

				compileExpr(i.first, EvalPurpose::RValue, make_shared<RegRefNode>(conditionRegIndex));
				curFn->insertIns(
					Opcode::EQ,
					make_shared<RegRefNode>(conditionRegIndex),
					make_shared<RegRefNode>(matcheeRegIndex, true),
					make_shared<RegRefNode>(conditionRegIndex, true));
				curFn->insertIns(Opcode::JF, make_shared<LabelRefNode>(caseEndLabel), make_shared<RegRefNode>(conditionRegIndex, true));

				// Leave the minor stack that is created for the local variable.
				compileExpr(i.second, curMajorContext.curMinorContext.evalPurpose, curMajorContext.curMinorContext.evalDest);

				curFn->insertLabel(caseEndLabel);
				curFn->insertIns(Opcode::JMP, make_shared<LabelRefNode>(endLabel));
			}

			if (defaultCase.second)
				compileExpr(defaultCase.second);

			curFn->insertLabel(endLabel);

			break;
		}
		case ExprType::Closure: {
			auto e = static_pointer_cast<ClosureExprNode>(expr);
			break;
		}
		case ExprType::Call: {
			auto e = static_pointer_cast<CallExprNode>(expr);

			curMajorContext.curMinorContext.isArgTypesSet = true;
			curMajorContext.curMinorContext.argTypes = {};

			for (auto &i : e->args) {
				curMajorContext.curMinorContext.argTypes.push_back(evalExprType(i));
			}

			if (auto ce = evalConstExpr(e); ce) {
				curFn->insertIns(Opcode::CALL, ce);
			} else {
				uint32_t callTargetRegIndex = allocReg(2);
				uint32_t tmpRegIndex = callTargetRegIndex + 1;

				for (auto i : e->args) {
					if (auto ce = evalConstExpr(i); ce)
						curFn->insertIns(Opcode::PUSHARG, ce);
					else {
						compileExpr(i, EvalPurpose::RValue, make_shared<RegRefNode>(tmpRegIndex));
						curFn->insertIns(Opcode::PUSHARG, make_shared<RegRefNode>(tmpRegIndex, true));
					}
				}

				compileExpr(e->target, EvalPurpose::Call, make_shared<RegRefNode>(callTargetRegIndex), make_shared<RegRefNode>(tmpRegIndex));

				if (curMajorContext.curMinorContext.isLastCallTargetStatic)
					curFn->insertIns(
						e->isAsync ? Opcode::ACALL : Opcode::CALL,
						make_shared<RegRefNode>(callTargetRegIndex, true));
				else
					curFn->insertIns(
						e->isAsync ? Opcode::AMCALL : Opcode::MCALL,
						make_shared<RegRefNode>(callTargetRegIndex, true),
						make_shared<RegRefNode>(tmpRegIndex, true));

				if (curMajorContext.curMinorContext.evalPurpose != EvalPurpose::Stmt) {
					curFn->insertIns(
						Opcode::LRET,
						curMajorContext.curMinorContext.evalDest);
				}
			}

			break;
		}
		case ExprType::Await: {
			uint32_t awaitTargetRegIndex = allocReg();

			auto e = static_pointer_cast<AwaitExprNode>(expr);

			if (auto ce = evalConstExpr(e); ce) {
				curFn->insertIns(Opcode::AWAIT, ce);
			} else {
				// TODO: Check if the target is a method.
				// compileExpr(e->target, curMajorContext.curMinorContext.evalPurpose, make_shared<RegRefNode>(RegId::R0));
				// curFn->insertIns(Opcode::AWAIT, make_shared<RegRefNode>(RegId::R0));
			}

			break;
		}
		case ExprType::New: {
			auto e = static_pointer_cast<NewExprNode>(expr);

			for (auto i : e->args) {
				if (auto ce = evalConstExpr(i); ce)
					curFn->insertIns(Opcode::PUSHARG, ce);
				else {
					uint32_t tmpRegIndex = allocReg();

					compileExpr(i, EvalPurpose::RValue, make_shared<RegRefNode>(tmpRegIndex));
					curFn->insertIns(Opcode::PUSHARG, make_shared<RegRefNode>(tmpRegIndex, true));
				}
			}

			curFn->insertIns(Opcode::NEW, curMajorContext.curMinorContext.evalDest, e->type);
			break;
		}
		case ExprType::Typeof: {
			auto e = static_pointer_cast<TypeofExprNode>(expr);

			if (auto ce = evalConstExpr(e->target); ce) {
				curFn->insertIns(Opcode::TYPEOF, curMajorContext.curMinorContext.evalDest, ce);
			} else {
				uint32_t tmpRegIndex = allocReg();

				compileExpr(e->target, EvalPurpose::RValue, make_shared<RegRefNode>(tmpRegIndex));
				curFn->insertIns(Opcode::TYPEOF, curMajorContext.curMinorContext.evalDest, make_shared<RegRefNode>(tmpRegIndex));
			}

			break;
		}
		case ExprType::Cast: {
			auto e = static_pointer_cast<CastExprNode>(expr);

			if (auto ce = evalConstExpr(e->target); ce) {
				if (areTypesConvertible(evalExprType(ce), e->targetType)) {
					curFn->insertIns(Opcode::CAST, curMajorContext.curMinorContext.evalDest, e->targetType, ce);
				} else {
					throw FatalCompilationError({ e->getLocation(), MessageType::Error, "Invalid type conversion" });
				}
			} else {
				if (areTypesConvertible(evalExprType(e->target), e->targetType)) {
					uint32_t tmpRegIndex = allocReg();

					compileExpr(e->target, EvalPurpose::RValue, make_shared<RegRefNode>(tmpRegIndex));
					curFn->insertIns(Opcode::CAST, curMajorContext.curMinorContext.evalDest, e->targetType, make_shared<RegRefNode>(tmpRegIndex, true));
				} else {
					throw FatalCompilationError({ e->getLocation(), MessageType::Error, "Invalid type conversion" });
				}
			}

			break;
		}
		case ExprType::HeadedRef: {
			auto e = static_pointer_cast<HeadedRefExprNode>(expr);

			break;
		}
		case ExprType::Ref: {
			auto e = static_pointer_cast<RefExprNode>(expr);

			deque<pair<Ref, shared_ptr<AstNode>>> resolvedParts;
			if (!resolveRef(e->ref, resolvedParts))
				// The default case is already exist.
				throw FatalCompilationError(
					{ e->getLocation(),
						MessageType::Error,
						"Identifier not found: `" + to_string(e->ref, this) + "'" });

			if (curMajorContext.curMinorContext.evalPurpose == EvalPurpose::Call) {
				if (isDynamicMember(resolvedParts.back().second)) {
					Ref thisRef = e->ref;
					thisRef.pop_back();
					compileExpr(
						make_shared<RefExprNode>(thisRef),
						EvalPurpose::RValue,
						curMajorContext.curMinorContext.thisDest);
					curMajorContext.curMinorContext.isLastCallTargetStatic = false;
				} else
					curMajorContext.curMinorContext.isLastCallTargetStatic = true;
			}

			uint32_t tmpRegIndex = allocReg();

			auto &x = resolvedParts.front().second;
			switch (x->getNodeType()) {
				case NodeType::LocalVar:
					curFn->insertIns(
						Opcode::STORE,
						make_shared<RegRefNode>(tmpRegIndex),
						make_shared<LocalVarRefNode>(
							static_pointer_cast<LocalVarNode>(x)->index,
							resolvedParts.size() > 1
								? true
								: curMajorContext.curMinorContext.evalPurpose != EvalPurpose::LValue));

					resolvedParts.pop_front();

					if (resolvedParts.size()) {
						for (auto i : resolvedParts) {
							curFn->insertIns(
								Opcode::RLOAD,
								make_shared<RegRefNode>(tmpRegIndex),
								make_shared<RegRefNode>(tmpRegIndex, true),
								make_shared<RefExprNode>(i.first));
						}

						if (resolvedParts.back().second->getNodeType() == NodeType::Var) {
							if (curMajorContext.curMinorContext.evalPurpose == EvalPurpose::RValue) {
								curFn->insertIns(
									Opcode::LVALUE,
									make_shared<RegRefNode>(tmpRegIndex),
									make_shared<RegRefNode>(tmpRegIndex, true));
							}
						}
					}

					curFn->insertIns(
						Opcode::STORE,
						curMajorContext.curMinorContext.evalDest,
						make_shared<RegRefNode>(tmpRegIndex, true));
					break;
				case NodeType::ArgRef:
					static_pointer_cast<ArgRefNode>(x)->unwrapData = (curMajorContext.curMinorContext.evalPurpose == EvalPurpose::RValue);
					curFn->insertIns(
						Opcode::STORE,
						curMajorContext.curMinorContext.evalDest, x);
					break;
				case NodeType::Var:
				case NodeType::Fn:
				case NodeType::ThisRef:
				case NodeType::BaseRef: {
					//
					// Resolve the head of the reference.
					// After that, we will use RLOAD instructions to load the members one by one.
					//
					switch (x->getNodeType()) {
						case NodeType::Var: {
							Ref ref = getFullName((VarNode *)x.get());
							curFn->insertIns(
								Opcode::LOAD,
								make_shared<RegRefNode>(tmpRegIndex),
								make_shared<RefExprNode>(ref));
							break;
						}
						case NodeType::Fn: {
							Ref ref;

							FnNode *fn = (FnNode *)x.get();
							_getFullName(fn, ref);

							FnOverloadingRegistry *overloadingRegistry = nullptr;

							if ((resolvedParts.size() > 2) || (curMajorContext.curMinorContext.evalPurpose != EvalPurpose::Call)) {
								//
								// Reference to a overloaded function is always ambiguous,
								// because we cannot determine which overloading is the user wanted.
								//
								if (fn->overloadingRegistries.size() > 1) {
									throw FatalCompilationError(
										Message(
											e->getLocation(),
											MessageType::Error,
											"Reference to a overloaded function is ambiguous"));
								}

								overloadingRegistry = &fn->overloadingRegistries.front();
							} else {
								overloadingRegistry = argDependentLookup(e->getLocation(), fn, curMajorContext.curMinorContext.argTypes);
							}

							deque<shared_ptr<TypeNameNode>> paramTypes;
							for (auto i : overloadingRegistry->params) {
								paramTypes.push_back(i.type);
							}

							if (overloadingRegistry->isVaridic())
								paramTypes.pop_back();

							ref.back().name = mangleName(
								resolvedParts.front().first.back().name,
								paramTypes,
								false);

							curFn->insertIns(
								Opcode::LOAD,
								make_shared<RegRefNode>(tmpRegIndex),
								make_shared<RefExprNode>(ref));
							break;
						}
						case NodeType::ThisRef:
							curFn->insertIns(Opcode::LTHIS, make_shared<RegRefNode>(tmpRegIndex));
							break;
						case NodeType::BaseRef:
							curFn->insertIns(Opcode::LOAD, make_shared<RegRefNode>(tmpRegIndex), make_shared<RefExprNode>(Ref{ RefEntry(e->getLocation(), "base") }));
							break;
						default:
							assert(false);
					}

					// Check if the target is static.
					resolvedParts.pop_front();
					for (size_t i = 0; i < resolvedParts.size(); ++i) {
						curFn->insertIns(
							Opcode::RLOAD,
							make_shared<RegRefNode>(tmpRegIndex),
							make_shared<RegRefNode>(tmpRegIndex, true),
							make_shared<RefExprNode>(resolvedParts[i].first));
					}

					//
					// TODO: Do some checks about if the final part is a function, if it is,
					// we have to redirect it to overloading which user wanted.
					//

					if (resolvedParts.size()) {
						if (curMajorContext.curMinorContext.evalPurpose == EvalPurpose::RValue)
							curFn->insertIns(
								Opcode::LVALUE,
								make_shared<RegRefNode>(tmpRegIndex),
								make_shared<RegRefNode>(tmpRegIndex, true));
					}

					curFn->insertIns(
						Opcode::STORE,
						curMajorContext.curMinorContext.evalDest,
						make_shared<RegRefNode>(tmpRegIndex, true));
					break;
				}
				default:
					assert(false);
			}

			break;
		}
		case ExprType::I8:
		case ExprType::I16:
		case ExprType::I32:
		case ExprType::I64:
		case ExprType::U32:
		case ExprType::U64:
		case ExprType::F32:
		case ExprType::F64:
		case ExprType::String:
		case ExprType::Bool:
		case ExprType::Array:
		case ExprType::Map:
			curFn->insertIns(Opcode::STORE, curMajorContext.curMinorContext.evalDest, expr);
			break;
		default:
			assert(false);
	}

	sld.nIns = curFn->body.size() - sld.offIns;
	curFn->srcLocDescs.push_back(sld);
}
