#include "compile.h"
#include <slake/slxfmt.h>
#include <bcparse.hh>
#include <cstring>

using namespace slake;
using namespace slake::bcc;

template <typename T>
static void _write(std::ostream &fs, const T &value) {
	fs.write((const char *)&value, sizeof(T));
}

template <typename T>
static void _write(std::ostream &fs, T &&value) {
	const T v = value;
	fs.write((const char *)&v, sizeof(T));
}

template <typename T>
static void _write(std::ostream &fs, const T *ptr, size_t size) {
	fs.write((const char *)ptr, size);
}

void bcc::compile(std::ostream &fs) {
	{
		slxfmt::ImgHeader ih = {};

		memcpy(ih.magic, slxfmt::IMH_MAGIC, sizeof(ih.magic));
		ih.fmtVer = 0;
		ih.nImports = (uint16_t)rootScope->imports.size();

		if (moduleName)
			ih.flags |= slxfmt::IMH_MODNAME;

		fs.write((char *)&ih, sizeof(ih));
	}

	if (moduleName)
		compileRef(fs, moduleName);

	for (auto i : rootScope->imports) {
		_write(fs, (uint32_t)i.first.size());
		_write(fs, i.first.data(), i.first.length());
		compileRef(fs, i.second);
	}

	compileScope(fs, rootScope);
}

void bcc::compileGenericParam(std::ostream &fs, const GenericParam &param) {
	slxfmt::GenericParamDesc gpd{};

	gpd.lenName = (uint8_t)param.name.length();
	gpd.nQualifier = (uint8_t)param.qualifiers.size();

	_write<slxfmt::GenericParamDesc>(fs, gpd);
	_write(fs, param.name.data(), param.name.length());

	for (auto &i : param.qualifiers) {
		slxfmt::GenericQualifierDesc gqd{};

		switch (i.filter) {
			case GenericFilter::Extends:
				gqd.filter = slxfmt::GenericFilter::Extends;
				break;
			case GenericFilter::Implements:
				gqd.filter = slxfmt::GenericFilter::Implements;
				break;
			case GenericFilter::HasTrait:
				gqd.filter = slxfmt::GenericFilter::HasTrait;
				break;
		}

		_write<slxfmt::GenericQualifierDesc>(fs, gqd);
		compileTypeName(fs, i.typeName);
	}
}

void bcc::compileScope(std::ostream &fs, shared_ptr<Scope> scope) {
	//
	// Compile variables.
	//
	_write(fs, (uint32_t)scope->vars.size());
	for (auto &i : scope->vars) {
		slxfmt::VarDesc vad = {};

		if (i.second->access & ACCESS_PUB)
			vad.flags |= slxfmt::VAD_PUB;
		if (i.second->access & ACCESS_STATIC)
			vad.flags |= slxfmt::VAD_STATIC;
		if (i.second->access & ACCESS_FINAL)
			vad.flags |= slxfmt::VAD_FINAL;
		if (i.second->access & ACCESS_NATIVE)
			vad.flags |= slxfmt::VAD_NATIVE;
		if (i.second->initValue)
			vad.flags |= slxfmt::VAD_INIT;

		vad.lenName = i.first.length();
		_write(fs, vad);
		_write(fs, i.first.data(), i.first.length());

		compileTypeName(fs, i.second->type);

		if (i.second->initValue)
			compileOperand(fs, i.second->initValue);
	}

	//
	// Compile functions.
	//
	_write(fs, (uint32_t)scope->funcs.size());
	for (auto &i : scope->funcs) {
		slxfmt::FnDesc fnd = {};

		if (i.second->access & ACCESS_PUB)
			fnd.flags |= slxfmt::FND_PUB;
		if (i.second->access & ACCESS_STATIC)
			fnd.flags |= slxfmt::FND_STATIC;
		if (i.second->access & ACCESS_NATIVE)
			fnd.flags |= slxfmt::FND_NATIVE;
		if (i.second->access & ACCESS_OVERRIDE)
			fnd.flags |= slxfmt::FND_OVERRIDE;
		if (i.second->access & ACCESS_FINAL)
			fnd.flags |= slxfmt::FND_FINAL;

		if (i.second->params.isVariadic)
			fnd.flags |= slxfmt::FND_VARG;

		fnd.lenName = (uint16_t)i.first.length();
		fnd.lenBody = (uint32_t)i.second->body.size();
		fnd.nParams = (uint8_t)i.second->params.size();

		_write(fs, fnd);
		_write(fs, i.first.data(), i.first.length());

		compileTypeName(fs, i.second->returnType);

		for (auto &j : i.second->params)
			compileTypeName(fs, j);

		for (auto &j : i.second->body) {
			slxfmt::InsHeader ih;
			ih.opcode = j->opcode;

			if (j->operands.size() > 3)
				throw parser::syntax_error(j->getLocation(), "Too many operands");
			ih.nOperands = (uint8_t)j->operands.size();

			_write(fs, ih);

			for (auto &k : j->operands) {
				if (k->getOperandType() == OperandType::LABEL) {
					auto &label = static_pointer_cast<LabelOperand>(k)->data;
					if (!i.second->labels.count(label))
						throw parser::syntax_error(k->getLocation(), "Label was not defined: `" + label + "'");
					k = make_shared<U32Operand>(k->getLocation(), i.second->labels.at(label));
				}
				compileOperand(fs, k);
			}
		}
	}

	//
	// Compile classes.
	//
	_write(fs, (uint32_t)scope->classes.size());
	for (auto &i : scope->classes) {
		slxfmt::ClassTypeDesc ctd = {};

		if (i.second->access & ACCESS_PUB)
			ctd.flags |= slxfmt::CTD_PUB;
		if (i.second->access & ACCESS_FINAL)
			ctd.flags |= slxfmt::CTD_FINAL;
		if (i.second->parent)
			ctd.flags |= slxfmt::CTD_DERIVED;
		ctd.nImpls = i.second->impls.size();
		ctd.lenName = i.first.length();
		ctd.nGenericParams = (uint8_t)i.second->genericParams.size();

		_write(fs, ctd);
		_write(fs, i.first.data(), i.first.length());

		if (i.second->parent)
			compileRef(fs, i.second->parent);

		for (auto &j : i.second->impls)
			compileRef(fs, j);

		for (auto &j : i.second->genericParams)
			compileGenericParam(fs, j);

		compileScope(fs, i.second->scope);
	}

	//
	// Compile interfaces.
	//
	_write(fs, (uint32_t)scope->interfaces.size());
	for (auto &i : scope->interfaces) {
		slxfmt::InterfaceTypeDesc ctd = {};

		if (i.second->access & ACCESS_PUB)
			ctd.flags |= slxfmt::ITD_PUB;

		ctd.nParents = (uint8_t)i.second->parents.size();

		ctd.lenName = i.first.length();

		_write(fs, ctd);
		_write(fs, i.first.data(), i.first.length());

		for (auto j : i.second->parents) {
			compileRef(fs, j);
		}

		compileScope(fs, i.second->scope);
	}

	//
	// Compile traits
	//
	_write(fs, (uint32_t)scope->traits.size());
	for (auto &i : scope->traits) {
		slxfmt::TraitTypeDesc ctd = {};

		if (i.second->access & ACCESS_PUB)
			ctd.flags |= slxfmt::TTD_PUB;

		ctd.nParents = (uint8_t)i.second->parents.size();

		ctd.lenName = i.first.length();

		_write(fs, ctd);
		_write(fs, i.first.data(), i.first.length());

		for (auto j : i.second->parents) {
			compileRef(fs, j);
		}

		compileScope(fs, i.second->scope);
	}
}

void bcc::compileOperand(std::ostream &fs, shared_ptr<Operand> operand) {
	slxfmt::ValueDesc vd = {};
	switch (operand->getOperandType()) {
		case OperandType::NONE: {
			vd.type = slxfmt::Type::None;
			_write(fs, vd);
			break;
		}
		case OperandType::I8: {
			vd.type = slxfmt::Type::I8;
			_write(fs, vd);

			_write(fs, static_pointer_cast<I8Operand>(operand)->data);
			break;
		}
		case OperandType::I16: {
			vd.type = slxfmt::Type::I16;
			_write(fs, vd);

			_write(fs, static_pointer_cast<I16Operand>(operand)->data);
			break;
		}
		case OperandType::I32: {
			vd.type = slxfmt::Type::I32;
			_write(fs, vd);

			_write(fs, static_pointer_cast<I32Operand>(operand)->data);
			break;
		}
		case OperandType::I64: {
			vd.type = slxfmt::Type::I64;
			_write(fs, vd);

			_write(fs, static_pointer_cast<I64Operand>(operand)->data);
			break;
		}
		case OperandType::U8: {
			vd.type = slxfmt::Type::U8;
			_write(fs, vd);

			_write(fs, static_pointer_cast<U8Operand>(operand)->data);
			break;
		}
		case OperandType::U16: {
			vd.type = slxfmt::Type::U16;
			_write(fs, vd);

			_write(fs, static_pointer_cast<U16Operand>(operand)->data);
			break;
		}
		case OperandType::U32: {
			vd.type = slxfmt::Type::U32;
			_write(fs, vd);

			_write(fs, static_pointer_cast<U32Operand>(operand)->data);
			break;
		}
		case OperandType::U64: {
			vd.type = slxfmt::Type::U64;
			_write(fs, vd);

			_write(fs, static_pointer_cast<U64Operand>(operand)->data);
			break;
		}
		case OperandType::F32: {
			vd.type = slxfmt::Type::F32;
			_write(fs, vd);

			_write(fs, static_pointer_cast<F32Operand>(operand)->data);
			break;
		}
		case OperandType::F64: {
			vd.type = slxfmt::Type::F64;
			_write(fs, vd);

			_write(fs, static_pointer_cast<F64Operand>(operand)->data);
			break;
		}
		case OperandType::BOOL: {
			vd.type = slxfmt::Type::Bool;
			_write(fs, vd);

			_write(fs, static_pointer_cast<BoolOperand>(operand)->data);
			break;
		}
		case OperandType::STRING: {
			vd.type = slxfmt::Type::String;
			_write(fs, vd);

			auto &s = static_pointer_cast<StringOperand>(operand)->data;

			_write(fs, (uint32_t)s.length());
			_write(fs, s.data(), s.size());
			break;
		}
		case OperandType::REF: {
			vd.type = slxfmt::Type::Ref;
			_write(fs, vd);

			compileRef(fs, static_pointer_cast<RefOperand>(operand)->data);
			break;
		}
		case OperandType::TYPENAME: {
			vd.type = slxfmt::Type::TypeName;
			_write(fs, vd);

			compileTypeName(fs, static_pointer_cast<TypeNameOperand>(operand)->data);
			break;
		}
		case OperandType::REG:
			vd.type = operand->dereferenced ? slxfmt::Type::RegValue : slxfmt::Type::Reg;
			_write(fs, vd);

			_write(fs, static_pointer_cast<RegOperand>(operand)->data);
			break;
		case OperandType::ARG:
			vd.type = operand->dereferenced ? slxfmt::Type::ArgValue : slxfmt::Type::Arg;
			_write(fs, vd);

			_write(fs, static_pointer_cast<ArgOperand>(operand)->data);
			break;
		case OperandType::LVAR:
			vd.type = operand->dereferenced ? slxfmt::Type::LocalVarValue : slxfmt::Type::LocalVar;
			_write(fs, vd);

			_write(fs, static_pointer_cast<LocalVarOperand>(operand)->data);
			break;
		default:
			assert(false);
	}
}

void bcc::compileRef(std::ostream &fs, shared_ptr<Ref> ref) {
	for (size_t i = 0; i < ref->entries.size(); ++i) {
		slxfmt::RefEntryDesc rsd = {};

		auto &entry = ref->entries[i];

		if (i + 1 < ref->entries.size())
			rsd.flags |= slxfmt::RSD_NEXT;

		rsd.lenName = entry->name.size();
		rsd.nGenericArgs = entry->genericArgs.size();
		_write(fs, rsd);
		_write(fs, entry->name.data(), entry->name.length());

		for (auto &j : entry->genericArgs)
			compileTypeName(fs, j);
	}
}

void bcc::compileTypeName(std::ostream &fs, shared_ptr<TypeName> typeName) {
	switch (typeName->type) {
		case Type::I8: {
			_write(fs, slxfmt::Type::I8);
			break;
		}
		case Type::I16: {
			_write(fs, slxfmt::Type::I16);
			break;
		}
		case Type::I32: {
			_write(fs, slxfmt::Type::I32);
			break;
		}
		case Type::I64: {
			_write(fs, slxfmt::Type::I64);
			break;
		}
		case Type::U8: {
			_write(fs, slxfmt::Type::U8);
			break;
		}
		case Type::U16: {
			_write(fs, slxfmt::Type::U16);
			break;
		}
		case Type::U32: {
			_write(fs, slxfmt::Type::U32);
			break;
		}
		case Type::U64: {
			_write(fs, slxfmt::Type::U64);
			break;
		}
		case Type::F32: {
			_write(fs, slxfmt::Type::F32);
			break;
		}
		case Type::F64: {
			_write(fs, slxfmt::Type::F64);
			break;
		}
		case Type::Bool: {
			_write(fs, slxfmt::Type::Bool);
			break;
		}
		case Type::String: {
			_write(fs, slxfmt::Type::String);
			break;
		}
		case Type::Void: {
			_write(fs, slxfmt::Type::None);
			break;
		}
		case Type::Any: {
			_write(fs, slxfmt::Type::Any);
			break;
		}
		case Type::Array: {
			_write(fs, slxfmt::Type::Array);
			compileTypeName(fs, static_pointer_cast<ArrayTypeName>(typeName)->elementType);
			break;
		}
		case Type::Map: {
			_write(fs, slxfmt::Type::Map);
			compileTypeName(fs, static_pointer_cast<MapTypeName>(typeName)->keyType);
			compileTypeName(fs, static_pointer_cast<MapTypeName>(typeName)->type);
			break;
		}
		case Type::Fn: {
			// stub
			break;
		}
		case Type::Custom: {
			_write(fs, slxfmt::Type::Object);
			compileRef(fs, static_pointer_cast<CustomTypeName>(typeName)->ref);
			break;
		}
		case Type::Generic: {
			_write(fs, slxfmt::Type::GenericArg);
			_write(fs, static_pointer_cast<GenericTypeName>(typeName)->index);
			break;
		}
		default:
			assert(false);
	}
}
