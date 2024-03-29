#include "compiler.h"

using namespace slake::slkc;

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

class SlakeErrorListener : public antlr4::BaseErrorListener {
	virtual void syntaxError(
		antlr4::Recognizer *recognizer,
		antlr4::Token *offendingSymbol,
		size_t line,
		size_t charPositionInLine,
		const std::string &msg,
		std::exception_ptr e) override {
		throw FatalCompilationError(
			Message(
				Location(line, charPositionInLine),
				MessageType::Error,
				msg));
	}
};

Compiler::~Compiler() {
	flags |= COMP_DELETING;
}

class PseudoOutputStream : public std::ostream {
public:
	inline PseudoOutputStream() : ostream(nullptr) {}
	virtual ~PseudoOutputStream() = default;
};

void Compiler::compile(std::istream &is, std::ostream &os) {
	antlr4::ANTLRInputStream s(is);

	SlakeLexer lexer(&s);
	antlr4::CommonTokenStream tokens(&lexer);
	SlakeErrorListener errorListener;

	SlakeParser parser(&tokens);
	parser.removeErrorListeners();
	parser.addErrorListener(&errorListener);
	auto tree = parser.prog();

	slake::slkc::AstVisitor visitor(this);
	visitor.visit(tree);

	{
		slxfmt::ImgHeader ih = {};

		memcpy(ih.magic, slxfmt::IMH_MAGIC, sizeof(ih.magic));
		ih.fmtVer = 0;
		ih.nImports = (uint16_t)_targetModule->imports.size();

		if (_targetModule->moduleName.size())
			ih.flags |= slxfmt::IMH_MODNAME;

		os.write((char *)&ih, sizeof(ih));
	}

	if (_targetModule->moduleName.size()) {
		// Build supposed parent modules in the root scope.
		auto scope = _rootScope;
		for (size_t i = 0; i < _targetModule->moduleName.size(); ++i) {
			string name = _targetModule->moduleName[i].name;
			if (auto it = scope->members.find(name); it != scope->members.end()) {
				switch (it->second->getNodeType()) {
					case NodeType::Class:
						scope = static_pointer_cast<ClassNode>(it->second)->scope;
						break;
					case NodeType::Interface:
						scope = static_pointer_cast<InterfaceNode>(it->second)->scope;
						break;
					case NodeType::Trait:
						scope = static_pointer_cast<TraitNode>(it->second)->scope;
						break;
					case NodeType::Module:
						scope = static_pointer_cast<ModuleNode>(it->second)->scope;
						break;
					default:
						// I give up - such situations shouldn't be here.
						assert(false);
				}
			} else {
				if (i + 1 == _targetModule->moduleName.size()) {
					(scope->members[name] = _targetModule)->bind((MemberNode *)scope->owner);
					_targetModule->scope->parent = scope.get();
				} else {
					auto newMod = make_shared<ModuleNode>(this, Location());
					(scope->members[name] = newMod)->bind((MemberNode *)scope->owner);
					newMod->scope->parent = scope.get();
				}
			}
		}

		compileRef(os, _targetModule->moduleName);
	}

	pushMajorContext();

	for (auto i : _targetModule->imports) {
		_write(os, (uint32_t)i.first.size());
		_write(os, i.first.data(), i.first.length());
		compileRef(os, i.second);

		importModule(i.first, i.second, _targetModule->scope);

		_targetModule->scope->members[i.first] = make_shared<AliasNode>(i.second[0].loc, this, i.first, i.second);
	}

	popMajorContext();

	curMajorContext = MajorContext();
	compileScope(is, os, _targetModule->scope);
}

void Compiler::importModule(string name, const Ref &ref, shared_ptr<Scope> scope) {
	if (importedModules.count(ref))
		return;
	importedModules.insert(ref);

	string path;

	for (auto j : ref) {
		path += "/" + j.name;
	}

	std::ifstream is;
	for (auto j : modulePaths) {
		is.open(j + path + ".slk");

		if (is.good()) {
			path = j + path + ".slk";

			auto savedTargetModule = _targetModule;
			PseudoOutputStream pseudoOs;
			compile(is, pseudoOs);
			_targetModule = savedTargetModule;

			goto succeeded;
		}

		is.clear();

		is.open(j + path + ".slx");

		if (is.good()) {
			path = j + path + ".slx";

			try {
				auto mod = _rt->loadModule(is, LMOD_NOIMPORT | LMOD_NORELOAD);

				for (auto j : mod->imports)
					importModule(j.first, toAstRef(j.second->entries), scope);

				importDefinitions(_rootScope, {}, mod.get());

				goto succeeded;
			} catch (LoaderError e) {
				printf("%s\n", e.what());
			}
		}

		is.clear();
	}

	throw FatalCompilationError(
		Message(
			ref[0].loc,
			MessageType::Error,
			"Cannot find module " + std::to_string(ref, this)));

succeeded:;
}

void Compiler::compileScope(std::istream &is, std::ostream &os, shared_ptr<Scope> scope) {
	unordered_map<string, shared_ptr<VarNode>> vars;
	unordered_map<string, shared_ptr<FnNode>> funcs;
	unordered_map<string, shared_ptr<CompiledFnNode>> compiledFuncs;
	unordered_map<string, shared_ptr<ClassNode>> classes;
	unordered_map<string, shared_ptr<InterfaceNode>> interfaces;
	unordered_map<string, shared_ptr<TraitNode>> traits;

	curMajorContext.curMinorContext.curScope = scope;

	for (auto i : scope->members) {
		switch (i.second->getNodeType()) {
			case NodeType::Var:
				vars[i.first] = static_pointer_cast<VarNode>(i.second);
				break;
			case NodeType::Fn:
				funcs[i.first] = static_pointer_cast<FnNode>(i.second);
				break;
			case NodeType::Class:
				classes[i.first] = static_pointer_cast<ClassNode>(i.second);
				break;
			case NodeType::Interface:
				interfaces[i.first] = static_pointer_cast<InterfaceNode>(i.second);
				break;
			case NodeType::Trait:
				traits[i.first] = static_pointer_cast<TraitNode>(i.second);
				break;
			case NodeType::Alias:
			case NodeType::Module:
				break;
			default:
				assert(false);
		}
	}

	_write<uint32_t>(os, vars.size());
	for (auto &i : vars) {
		slxfmt::VarDesc vad = {};

		//
		// Check if the member will shadow member(s) from parent scopes.
		//
		switch (scope->owner->getNodeType()) {
			case NodeType::Class: {
				auto j = (ClassNode *)(scope->owner);

				while (j->parentClass) {
					j = (ClassNode *)resolveCustomType(j->parentClass.get()).get();
					if (j->scope->members.count(i.first))
						throw FatalCompilationError(
							Message(
								i.second->getLocation(),
								MessageType::Error,
								"This member shadows members from parent classes"));
				};

				break;
			}
		}

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

		vad.lenName = (uint8_t)i.first.length();
		_write(os, vad);
		_write(os, i.first.data(), i.first.length());

		compileTypeName(os, i.second->type);

		if (i.second->initValue) {
			if (auto ce = evalConstExpr(i.second->initValue); ce)
				compileExpr(i.second->initValue);
			else
				throw FatalCompilationError(
					Message(
						i.second->initValue->getLocation(),
						MessageType::Error,
						"Expecting a compiling-time expression"));
		}
	}

	for (auto &i : funcs) {
		for (auto &j : i.second->overloadingRegistries) {
			pushMajorContext();

			curMajorContext.mergeGenericParams(j.genericParams);

			string mangledFnName = i.first;

			if (i.first != "new") {
				{
					deque<shared_ptr<TypeNameNode>> argTypes;

					for (auto &k : j.params) {
						argTypes.push_back(k.type);
					}

					mangledFnName = mangleName(i.first, argTypes, false);
				}

				if (compiledFuncs.count(mangledFnName)) {
					throw FatalCompilationError(
						Message(
							j.loc,
							MessageType::Error,
							"Duplicated function overloading"));
				}
			}

			auto compiledFn = make_shared<CompiledFnNode>(j.loc, mangledFnName);
			compiledFn->returnType = j.returnType;
			compiledFn->params = j.params;
			compiledFn->paramIndices = j.paramIndices;
			compiledFn->genericParams = j.genericParams;
			compiledFn->genericParamIndices = j.genericParamIndices;
			compiledFn->access = j.access;

			curFn = compiledFn;

			if (j.body)
				compileStmt(j.body);

			compiledFuncs[mangledFnName] = compiledFn;

			popMajorContext();
		}
	}

	_write(os, (uint32_t)compiledFuncs.size());
	for (auto &i : compiledFuncs) {
		slxfmt::FnDesc fnd = {};
		bool hasVarArg = false;

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

		if (i.second->paramIndices.count("..."))
			hasVarArg = true;

		if (hasVarArg)
			fnd.flags |= slxfmt::FND_VARG;

		fnd.lenName = (uint16_t)i.first.length();
		fnd.lenBody = (uint32_t)i.second->body.size();
		fnd.nParams = (uint8_t)i.second->params.size() - hasVarArg;
		fnd.nSourceLocDescs = (uint32_t)i.second->srcLocDescs.size();
		fnd.nGenericParams = i.second->genericParams.size();

		_write(os, fnd);
		_write(os, i.first.data(), i.first.length());

		for (size_t j = 0; j < i.second->genericParams.size(); ++j)
			compileGenericParam(os, i.second->genericParams[j]);

		compileTypeName(os, i.second->returnType);

		for (size_t j = 0; j < i.second->params.size() - hasVarArg; ++j)
			compileTypeName(os, i.second->params[j].type);

		for (auto &j : i.second->body) {
			slxfmt::InsHeader ih;
			ih.opcode = j.opcode;

			if (j.operands.size() > 3)
				throw FatalCompilationError(
					Message(
						i.second->getLocation(),
						MessageType::Error,
						"Too many operands"));
			ih.nOperands = (uint8_t)j.operands.size();

			_write(os, ih);

			for (auto &k : j.operands) {
				if (k) {
					if (k->getNodeType() == NodeType::LabelRef) {
						auto &label = static_pointer_cast<LabelRefNode>(k)->label;
						if (!i.second->labels.count(label))
							throw FatalCompilationError(
								Message(
									i.second->getLocation(),
									MessageType::Error,
									"Undefined label: " + static_pointer_cast<LabelRefNode>(k)->label));
						k = make_shared<U32LiteralExprNode>(i.second->getLocation(), i.second->labels.at(label));
					}
				}
				compileValue(os, k);
			}
		}

		for (auto &j : i.second->srcLocDescs) {
			_write(os, j);
		}
	}

	_write(os, (uint32_t)classes.size());
	for (auto &i : classes) {
		pushMajorContext();

		// curMajorContext = MajorContext();
		curMajorContext.mergeGenericParams(i.second->genericParams);

		slxfmt::ClassTypeDesc ctd = {};

		if (i.second->access & ACCESS_PUB)
			ctd.flags |= slxfmt::CTD_PUB;
		if (i.second->access & ACCESS_FINAL)
			ctd.flags |= slxfmt::CTD_FINAL;
		if (i.second->parentClass)
			ctd.flags |= slxfmt::CTD_DERIVED;
		ctd.nImpls = (uint8_t)i.second->implInterfaces.size();
		ctd.lenName = (uint8_t)i.first.length();
		ctd.nGenericParams = (uint8_t)i.second->genericParams.size();

		_write(os, ctd);
		_write(os, i.first.data(), i.first.length());

		for (auto &j : i.second->genericParams)
			compileGenericParam(os, j);

		if (i.second->parentClass)
			compileRef(os, getFullName((MemberNode *)resolveCustomType(i.second->parentClass.get()).get()));

		for (auto &j : i.second->implInterfaces)
			compileRef(os, j->ref);

		compileScope(is, os, i.second->scope);

		popMajorContext();
	}

	_write(os, (uint32_t)interfaces.size());
	for (auto &i : interfaces) {
		pushMajorContext();

		// curMajorContext = MajorContext();
		curMajorContext.mergeGenericParams(i.second->genericParams);

		slxfmt::InterfaceTypeDesc itd = {};

		if (i.second->access & ACCESS_PUB)
			itd.flags |= slxfmt::ITD_PUB;

		itd.nParents = (uint8_t)i.second->parentInterfaces.size();
		itd.nGenericParams = i.second->genericParams.size();

		itd.lenName = (uint8_t)i.first.length();

		_write(os, itd);
		_write(os, i.first.data(), i.first.length());

		for (size_t j = 0; j < i.second->genericParams.size(); ++j)
			compileGenericParam(os, i.second->genericParams[j]);

		for (auto j : i.second->parentInterfaces) {
			compileRef(os, j->ref);
		}

		compileScope(is, os, i.second->scope);

		popMajorContext();
	}

	_write(os, (uint32_t)traits.size());
	for (auto &i : traits) {
		pushMajorContext();

		// curMajorContext = MajorContext();
		curMajorContext.mergeGenericParams(i.second->genericParams);

		slxfmt::TraitTypeDesc ttd = {};

		if (i.second->access & ACCESS_PUB)
			ttd.flags |= slxfmt::TTD_PUB;

		ttd.nParents = (uint8_t)i.second->parentTraits.size();

		ttd.lenName = (uint8_t)i.first.length();
		ttd.nGenericParams = i.second->genericParams.size();

		_write(os, ttd);
		_write(os, i.first.data(), i.first.length());

		for (size_t j = 0; j < i.second->genericParams.size(); ++j)
			compileGenericParam(os, i.second->genericParams[j]);

		for (auto j : i.second->parentTraits) {
			compileRef(os, j->ref);
		}

		compileScope(is, os, i.second->scope);

		popMajorContext();
	}
}

void Compiler::compileTypeName(std::ostream &fs, shared_ptr<TypeNameNode> typeName) {
	switch (typeName->getTypeId()) {
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
			compileTypeName(fs, static_pointer_cast<ArrayTypeNameNode>(typeName)->elementType);
			break;
		}
		case Type::Map: {
			_write(fs, slxfmt::Type::Map);
			compileTypeName(fs, static_pointer_cast<MapTypeNameNode>(typeName)->keyType);
			compileTypeName(fs, static_pointer_cast<MapTypeNameNode>(typeName)->valueType);
			break;
		}
		case Type::Fn: {
			// stub
			break;
		}
		case Type::Custom: {
			auto dest = resolveCustomType((CustomTypeNameNode *)typeName.get());

			if (dest->getNodeType() == NodeType::GenericParam) {
				_write(fs, slxfmt::Type::GenericArg);

				_write(fs, (uint8_t)curMajorContext.genericParamIndices[static_pointer_cast<GenericParamNode>(dest)->name]);
			} else {
				_write(fs, slxfmt::Type::Object);
				compileRef(fs, getFullName((MemberNode *)dest.get()));
			}
			break;
		}
		default:
			assert(false);
	}
}

void Compiler::compileRef(std::ostream &fs, const Ref &ref) {
	for (size_t i = 0; i < ref.size(); ++i) {
		slxfmt::RefEntryDesc rsd = {};

		auto &entry = ref[i];

		if (i + 1 < ref.size())
			rsd.flags |= slxfmt::RSD_NEXT;

		rsd.lenName = entry.name.size();
		rsd.nGenericArgs = entry.genericArgs.size();
		_write(fs, rsd);
		_write(fs, entry.name.data(), entry.name.length());

		for (auto &j : entry.genericArgs)
			compileTypeName(fs, j);
	}
}

void Compiler::compileValue(std::ostream &fs, shared_ptr<AstNode> value) {
	slxfmt::ValueDesc vd = {};

	if (!value) {
		vd.type = slxfmt::Type::None;
		_write(fs, vd);
		return;
	}

	switch (value->getNodeType()) {
		case NodeType::TypeName:
			vd.type = slxfmt::Type::TypeName;
			_write(fs, vd);

			compileTypeName(fs, static_pointer_cast<TypeNameNode>(value));
			break;
		case NodeType::ArgRef: {
			auto v = static_pointer_cast<ArgRefNode>(value);
			vd.type = v->unwrapData ? slxfmt::Type::ArgValue : slxfmt::Type::Arg;
			_write(fs, vd);

			_write(fs, v->index);
			break;
		}
		case NodeType::LocalVarRef: {
			auto v = static_pointer_cast<LocalVarRefNode>(value);
			vd.type = v->unwrapData ? slxfmt::Type::LocalVarValue : slxfmt::Type::LocalVar;
			_write(fs, vd);

			_write(fs, v->index);
			break;
		}
		case NodeType::RegRef: {
			auto v = static_pointer_cast<RegRefNode>(value);
			vd.type = v->unwrapData ? slxfmt::Type::RegValue : slxfmt::Type::Reg;
			_write(fs, vd);

			_write(fs, v->index);
			break;
		}
		case NodeType::Expr: {
			shared_ptr<ExprNode> expr = static_pointer_cast<ExprNode>(value);
			switch (expr->getExprType()) {
				case ExprType::I8: {
					vd.type = slxfmt::Type::I8;
					_write(fs, vd);

					_write(fs, static_pointer_cast<I8LiteralExprNode>(expr)->data);
					break;
				}
				case ExprType::I16: {
					vd.type = slxfmt::Type::I16;
					_write(fs, vd);

					_write(fs, static_pointer_cast<I16LiteralExprNode>(expr)->data);
					break;
				}
				case ExprType::I32: {
					vd.type = slxfmt::Type::I32;
					_write(fs, vd);

					_write(fs, static_pointer_cast<I32LiteralExprNode>(expr)->data);
					break;
				}
				case ExprType::I64: {
					vd.type = slxfmt::Type::I64;
					_write(fs, vd);

					_write(fs, static_pointer_cast<I64LiteralExprNode>(expr)->data);
					break;
				}
				case ExprType::U8: {
					vd.type = slxfmt::Type::U8;
					_write(fs, vd);

					_write(fs, static_pointer_cast<U8LiteralExprNode>(expr)->data);
					break;
				}
				case ExprType::U16: {
					vd.type = slxfmt::Type::U16;
					_write(fs, vd);

					_write(fs, static_pointer_cast<U16LiteralExprNode>(expr)->data);
					break;
				}
				case ExprType::U32: {
					vd.type = slxfmt::Type::U32;
					_write(fs, vd);

					_write(fs, static_pointer_cast<U32LiteralExprNode>(expr)->data);
					break;
				}
				case ExprType::U64: {
					vd.type = slxfmt::Type::U64;
					_write(fs, vd);

					_write(fs, static_pointer_cast<U64LiteralExprNode>(expr)->data);
					break;
				}
				case ExprType::F32: {
					vd.type = slxfmt::Type::F32;
					_write(fs, vd);

					_write(fs, static_pointer_cast<F32LiteralExprNode>(expr)->data);
					break;
				}
				case ExprType::F64: {
					vd.type = slxfmt::Type::F64;
					_write(fs, vd);

					_write(fs, static_pointer_cast<F64LiteralExprNode>(expr)->data);
					break;
				}
				case ExprType::Bool: {
					vd.type = slxfmt::Type::Bool;
					_write(fs, vd);

					_write(fs, static_pointer_cast<BoolLiteralExprNode>(expr)->data);
					break;
				}
				case ExprType::String: {
					vd.type = slxfmt::Type::String;
					_write(fs, vd);

					auto &s = static_pointer_cast<StringLiteralExprNode>(expr)->data;

					_write(fs, (uint32_t)s.length());
					_write(fs, s.data(), s.size());
					break;
				}
				case ExprType::Ref: {
					vd.type = slxfmt::Type::Ref;
					_write(fs, vd);

					compileRef(fs, static_pointer_cast<RefExprNode>(expr)->ref);
					break;
				}
				default:
					assert(false);
			}
			break;
		}
		default:
			assert(false);
	}
}

void slake::slkc::Compiler::compileGenericParam(std::ostream &fs, shared_ptr<GenericParamNode> genericParam) {
	slxfmt::GenericParamDesc gpd;

	gpd.lenName = (uint8_t)genericParam->name.size();
	gpd.hasBaseType = (bool)genericParam->baseType;
	gpd.nInterfaces = (uint8_t)genericParam->interfaceTypes.size();
	gpd.nTraits = (uint8_t)genericParam->traitTypes.size();

	_write(fs, gpd);

	fs.write(genericParam->name.c_str(), genericParam->name.size());

	if (genericParam->baseType)
		compileTypeName(fs, genericParam->baseType);

	for (auto i : genericParam->interfaceTypes)
		compileTypeName(fs, i);

	for (auto i : genericParam->traitTypes)
		compileTypeName(fs, i);
}
