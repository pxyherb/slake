#include <slake/runtime.h>

#include <memory>
#include <slake/util/stream.hh>

using namespace slake;

/// @brief Read a single element from a stream.
/// @tparam T Type of element to read.
/// @param fs Stream to be read.
/// @return Element read from the stream.
template <typename T>
static T _read(std::istream &fs) {
	T value;
	fs.read((char *)&value, sizeof(value));
	return value;
}

/// @brief Load a single reference value from a stream.
/// @param rt Runtime for the new value.
/// @param fs Stream to be read.
/// @return Reference value loaded from the stream.
RefValue *Runtime::_loadRef(std::istream &fs) {
	auto ref = std::make_unique<RefValue>(this);

	slxfmt::RefEntryDesc i = { 0 };
	while (true) {
		i = _read<slxfmt::RefEntryDesc>(fs);

		std::string name(i.lenName, '\0');
		fs.read(name.data(), i.lenName);

		GenericArgList genericArgs;
		for (size_t j = i.nGenericArgs; j; --j)
			genericArgs.push_back(_loadType(fs, _read<slxfmt::Type>(fs)));

		ref->entries.push_back(RefEntry(name, genericArgs));
		if (!(i.flags & slxfmt::RSD_NEXT))
			break;
	};

	return ref.release();
}

/// @brief Load a single value from a stream.
/// @param rt Runtime for the new value.
/// @param fs Stream to be read.
/// @return Value loaded from the stream.
Value *Runtime::_loadValue(std::istream &fs) {
	slxfmt::ValueDesc i = {};
	fs.read((char *)&i, sizeof(i));
	switch (i.type) {
		case slxfmt::Type::None:
			return nullptr;
		case slxfmt::Type::I8:
			return new I8Value(this, _read<std::int8_t>(fs));
		case slxfmt::Type::I16:
			return new I16Value(this, _read<std::int16_t>(fs));
		case slxfmt::Type::I32:
			return new I32Value(this, _read<std::int32_t>(fs));
		case slxfmt::Type::I64:
			return new I64Value(this, _read<std::int64_t>(fs));
		case slxfmt::Type::U8:
			return new U8Value(this, _read<uint8_t>(fs));
		case slxfmt::Type::U16:
			return new U16Value(this, _read<uint16_t>(fs));
		case slxfmt::Type::U32:
			return new U32Value(this, _read<uint32_t>(fs));
		case slxfmt::Type::U64:
			return new U64Value(this, _read<uint64_t>(fs));
		case slxfmt::Type::Bool:
			return new BoolValue(this, _read<bool>(fs));
		case slxfmt::Type::F32:
			return new F32Value(this, _read<float>(fs));
		case slxfmt::Type::F64:
			return new F64Value(this, _read<double>(fs));
		case slxfmt::Type::String: {
			auto len = _read<uint32_t>(fs);
			std::string s(len, '\0');
			fs.read(&(s[0]), len);
			return new StringValue(this, s);
		}
		case slxfmt::Type::Ref:
			return _loadRef(fs);
		case slxfmt::Type::TypeName:
			return new TypeNameValue(this, _loadType(fs, _read<slxfmt::Type>(fs)));
		case slxfmt::Type::Reg:
			return new RegRefValue(this, _read<uint32_t>(fs));
		case slxfmt::Type::RegValue:
			return new RegRefValue(this, _read<uint32_t>(fs), true);
		case slxfmt::Type::LocalVar:
			return new LocalVarRefValue(this, _read<uint32_t>(fs));
		case slxfmt::Type::LocalVarValue:
			return new LocalVarRefValue(this, _read<uint32_t>(fs), true);
		case slxfmt::Type::Arg:
			return new ArgRefValue(this, _read<uint32_t>(fs));
		case slxfmt::Type::ArgValue:
			return new ArgRefValue(this, _read<uint32_t>(fs), true);
		default:
			throw LoaderError("Invalid value type detected");
	}
}

/// @brief Load a single type name from a stream.
/// @param rt Runtime for the new type.
/// @param fs Stream to be read.
/// @param vt Previous read value type.
/// @return Loaded complete type name.
Type Runtime::_loadType(std::istream &fs, slxfmt::Type vt) {
	switch (vt) {
		case slxfmt::Type::I8:
			return TypeId::I8;
		case slxfmt::Type::I16:
			return TypeId::I16;
		case slxfmt::Type::I32:
			return TypeId::I32;
		case slxfmt::Type::I64:
			return TypeId::I64;
		case slxfmt::Type::U8:
			return TypeId::U8;
		case slxfmt::Type::U16:
			return TypeId::U16;
		case slxfmt::Type::U32:
			return TypeId::U32;
		case slxfmt::Type::U64:
			return TypeId::U64;
		case slxfmt::Type::F32:
			return TypeId::F32;
		case slxfmt::Type::F64:
			return TypeId::F64;
		case slxfmt::Type::String:
			return TypeId::String;
		case slxfmt::Type::Object:
			return _loadRef(fs);
		case slxfmt::Type::Any:
			return TypeId::Any;
		case slxfmt::Type::Bool:
			return TypeId::Bool;
		case slxfmt::Type::None:
			return TypeId::None;
		case slxfmt::Type::Array:
			return Type(_loadType(fs, _read<slxfmt::Type>(fs)));
		case slxfmt::Type::Map:
			return Type(
				_loadType(fs, _read<slxfmt::Type>(fs)),
				_loadType(fs, _read<slxfmt::Type>(fs)));
		case slxfmt::Type::TypeName:
			return TypeId::TypeName;
		case slxfmt::Type::GenericArg:
			return Type(_read<uint8_t>(fs));
		default:
			throw LoaderError("Invalid type ID");
	}
}

GenericParam Runtime::_loadGenericParam(std::istream &fs) {
	auto gpd = _read<slxfmt::GenericParamDesc>(fs);

	std::string name(gpd.lenName, '\0');
	fs.read(&(name[0]), gpd.lenName);

	GenericParam param;
	param.name = name;

	if (gpd.hasBaseType)
		param.baseType = _loadType(fs, _read<slxfmt::Type>(fs));

	for (size_t i = 0; i < gpd.nInterfaces; ++i) {
		param.interfaces.push_back(_loadType(fs, _read<slxfmt::Type>(fs)));
	}

	for (size_t i = 0; i < gpd.nTraits; ++i) {
		param.traits.push_back(_loadType(fs, _read<slxfmt::Type>(fs)));
	}

	return param;
}

/// @brief Load a single scope.
/// @param mod Module value which is treated as a scope.
/// @param fs The input stream.
void Runtime::_loadScope(ModuleValue *mod, std::istream &fs) {
	uint32_t nItemsToRead;

	//
	// Load variables.
	//
	nItemsToRead = _read<uint32_t>(fs);
	for (slxfmt::VarDesc i = { 0 }; nItemsToRead--;) {
		i = _read<slxfmt::VarDesc>(fs);

		std::string name(i.lenName, '\0');
		fs.read(name.data(), i.lenName);

		AccessModifier access = 0;
		if (i.flags & slxfmt::VAD_PUB)
			access |= ACCESS_PUB;
		if (i.flags & slxfmt::VAD_STATIC)
			access |= ACCESS_STATIC;
		if (i.flags & slxfmt::VAD_FINAL)
			access |= ACCESS_FINAL;
		if (i.flags & slxfmt::VAD_NATIVE)
			access |= ACCESS_NATIVE;

		std::unique_ptr<VarValue> var =
			std::make_unique<VarValue>(
				this,
				access,
				_loadType(fs, _read<slxfmt::Type>(fs)));

		// Load initial value.
		if (i.flags & slxfmt::VAD_INIT)
			var->setData(_loadValue(fs));

		mod->scope->putMember(name, var.release());
	}

	//
	// Load functions.
	//
	nItemsToRead = _read<uint32_t>(fs);
	for (slxfmt::FnDesc i = { 0 }; nItemsToRead--;) {
		i = _read<slxfmt::FnDesc>(fs);

		std::string name(i.lenName, '\0');
		fs.read(name.data(), i.lenName);

		AccessModifier access = 0;
		if (i.flags & slxfmt::FND_PUB)
			access |= ACCESS_PUB;
		if (i.flags & slxfmt::FND_STATIC)
			access |= ACCESS_STATIC;
		if (i.flags & slxfmt::FND_FINAL)
			access |= ACCESS_FINAL;
		if (i.flags & slxfmt::FND_OVERRIDE)
			access |= ACCESS_OVERRIDE;
		// if (i.flags & slxfmt::FND_NATIVE)
		//	access |= ACCESS_NATIVE;

		std::unique_ptr<FnValue> fn = std::make_unique<FnValue>(this, (uint32_t)i.lenBody, access, _loadType(fs, _read<slxfmt::Type>(fs)));

		for (size_t j = 0; j < i.nGenericParams; ++j) {
			fn->genericParams.push_back(_loadGenericParam(fs));
		}

		for (uint8_t j = 0; j < i.nParams; j++) {
			fn->paramTypes.push_back(_loadType(fs, _read<slxfmt::Type>(fs)));
		}

		if (i.flags & slxfmt::FND_VARG)
			/* stub */;

		if (i.lenBody) {
			auto body = fn->getBody();

			for (uint32_t j = 0; j < i.lenBody; j++) {
				slxfmt::InsHeader ih = _read<slxfmt::InsHeader>(fs);
				body[j].opcode = ih.opcode;
				for (uint8_t k = 0; k < ih.nOperands; k++)
					body[j].operands.push_back(_loadValue(fs));
			}
		}

		for (uint32_t j = 0; j < i.nSourceLocDescs; ++j) {
			slxfmt::SourceLocDesc sld = _read<slxfmt::SourceLocDesc>(fs);
			fn->sourceLocDescs.push_back(sld);
		}

		mod->scope->putMember(name, fn.release());
	}

	//
	// Load classes.
	//
	nItemsToRead = _read<uint32_t>(fs);
	for (slxfmt::ClassTypeDesc i = {}; nItemsToRead--;) {
		i = _read<slxfmt::ClassTypeDesc>(fs);

		std::string name(i.lenName, '\0');
		fs.read(name.data(), i.lenName);

		AccessModifier access = 0;
		if (i.flags & slxfmt::CTD_PUB)
			access |= ACCESS_PUB;
		if (i.flags & slxfmt::CTD_FINAL)
			access |= ACCESS_FINAL;

		std::unique_ptr<ClassValue> value = std::make_unique<ClassValue>(this, access);

		for (size_t j = 0; j < i.nGenericParams; ++j)
			value->genericParams.push_back(_loadGenericParam(fs));

		// Load reference to the parent class.
		if (i.flags & slxfmt::CTD_DERIVED)
			value->parentClass = Type(TypeId::Class, _loadRef(fs));

		// Load references to implemented interfaces.
		for (auto j = i.nImpls; j; j--)
			value->implInterfaces.push_back(_loadRef(fs));

		_loadScope(value.get(), fs);

		mod->scope->putMember(name, value.release());
	}

	//
	// Load interfaces.
	//
	nItemsToRead = _read<uint32_t>(fs);
	for (slxfmt::InterfaceTypeDesc i = {}; nItemsToRead--;) {
		i = _read<slxfmt::InterfaceTypeDesc>(fs);

		std::string name(i.lenName, '\0');
		fs.read(name.data(), i.lenName);

		AccessModifier access = 0;
		if (i.flags & slxfmt::ITD_PUB)
			access |= ACCESS_PUB;

		std::unique_ptr<InterfaceValue> value = std::make_unique<InterfaceValue>(this, access);

		for (size_t j = 0; j < i.nGenericParams; ++j)
			value->genericParams.push_back(_loadGenericParam(fs));

		for (auto j = i.nParents; j; j--)
			value->parents.push_back(_loadRef(fs));

		_loadScope(value.get(), fs);

		mod->scope->putMember(name, value.release());
	}

	//
	// Load traits.
	//
	nItemsToRead = _read<uint32_t>(fs);
	for (slxfmt::TraitTypeDesc i = {}; nItemsToRead--;) {
		i = _read<slxfmt::TraitTypeDesc>(fs);

		std::string name(i.lenName, '\0');
		fs.read(name.data(), i.lenName);

		AccessModifier access = 0;
		if (i.flags & slxfmt::TTD_PUB)
			access |= ACCESS_PUB;

		std::unique_ptr<TraitValue> value = std::make_unique<TraitValue>(this, access);

		for (size_t j = 0; j < i.nGenericParams; ++j)
			value->genericParams.push_back(_loadGenericParam(fs));

		for (auto j = i.nParents; j; j--)
			value->parents.push_back(_loadRef(fs));

		_loadScope(value.get(), fs);

		mod->scope->putMember(name, value.release());
	}
}

ValueRef<ModuleValue> slake::Runtime::loadModule(std::istream &fs, LoadModuleFlags flags) {
	std::unique_ptr<ModuleValue> mod = std::make_unique<ModuleValue>(this, ACCESS_PUB);

	slxfmt::ImgHeader ih;
	fs.read((char *)&ih, sizeof(ih));
	if (memcmp(ih.magic, slxfmt::IMH_MAGIC, sizeof(slxfmt::IMH_MAGIC)))
		throw LoaderError("Bad SLX magic");
	if (ih.fmtVer != 0)
		throw LoaderError("Bad SLX format version");

	if (ih.flags & slxfmt::IMH_MODNAME) {
		auto modName = _loadRef(fs);
		if (!modName->entries.size())
			throw LoaderError("Empty module name with module name flag set");

		ValueRef<> curValue = (Value *)_rootValue;

		// Create parent modules.
		for (size_t i = 0; i < modName->entries.size() - 1; ++i) {
			auto &name = modName->entries[i].name;

			if (!curValue->getMember(name)) {
				// Create a new one if corresponding module does not present.
				auto mod = new ModuleValue(this, ACCESS_PUB);

				if (curValue->getType() == TypeId::RootValue)
					((RootValue *)curValue.get())->scope->putMember(name, mod);
				else
					((ModuleValue *)curValue.get())->scope->putMember(name, mod);

				curValue = (Value *)mod;
			} else {
				// Continue if the module presents.
				curValue = curValue->getMember(name);
			}
		}

		auto lastName = modName->entries.back().name;
		// Add current module.
		if (curValue->getType() == TypeId::RootValue)
			((RootValue *)curValue.get())->scope->putMember(lastName, mod.get());
		else {
			auto moduleValue = (ModuleValue *)curValue.get();

			if (auto member = moduleValue->getMember(lastName); member) {
				if (flags & LMOD_NORELOAD) {
					if (member->getType() != TypeId::Module)
						throw LoaderError(
							"Value which corresponds to module name \"" + std::to_string(modName, this) + "\" was found, but is not a module");
				}
				if (flags & LMOD_NOCONFLICT)
					throw LoaderError("Module \"" + std::to_string(modName, this) + "\" conflicted with existing value which is on the same path");
			}

			moduleValue->scope->putMember(modName->entries.back().name, mod.get());
		}
	}

	for (uint8_t i = 0; i < ih.nImports; i++) {
		auto len = _read<uint32_t>(fs);
		std::string name(len, '\0');
		fs.read(name.data(), len);

		ValueRef<RefValue> moduleName = _loadRef(fs);

		if (!(flags & LMOD_NOIMPORT)) {
			std::unique_ptr<std::istream> moduleStream(_moduleLocator(this, moduleName));
			if (!moduleStream)
				throw LoaderError("Error finding module `" + std::to_string(moduleName) + "' for dependencies");
			mod->scope->putMember(name, (MemberValue *)new AliasValue(this, 0, loadModule(*moduleStream.get(), LMOD_NORELOAD).get()));
		}

		mod->imports[name] = moduleName.get();
	}

	_loadScope(mod.get(), fs);
	return mod.release();
}

ValueRef<ModuleValue> slake::Runtime::loadModule(const void *buf, size_t size, LoadModuleFlags flags) {
	util::InputMemStream fs(buf, size);
	return loadModule(fs, flags);
}
