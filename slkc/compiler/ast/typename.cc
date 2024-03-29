#include "../compiler.h"

using namespace slake::slkc;

string std::to_string(shared_ptr<slake::slkc::TypeNameNode> typeName, slake::slkc::Compiler *compiler, bool asOperatorName) {
	string s = typeName->isConst ? "const " : "";
	switch (typeName->getTypeId()) {
		case Type::I8:
			return s + "i8";
		case Type::I16:
			return s + "i16";
		case Type::I32:
			return s + "i32";
		case Type::I64:
			return s + "i64";
		case Type::U8:
			return s + "u8";
		case Type::U16:
			return s + "u16";
		case Type::U32:
			return s + "u32";
		case Type::U64:
			return s + "u64";
		case Type::F32:
			return s + "f32";
		case Type::F64:
			return s + "f64";
		case Type::String:
			return s + "string";
		case Type::Bool:
			return s + "bool";
		case Type::Auto:
			return s + "auto";
		case Type::Void:
			return s + "void";
		case Type::Any:
			return s + "any";
		case Type::Array:
			return s + to_string(static_pointer_cast<ArrayTypeNameNode>(typeName)->elementType, compiler, asOperatorName) + "[]";
		case Type::Map: {
			auto t = static_pointer_cast<MapTypeNameNode>(typeName);
			return s + to_string(t->keyType, compiler, asOperatorName) + "[" + to_string(t->valueType, compiler, asOperatorName) + "]";
		}
		case Type::Fn: {
			auto t = static_pointer_cast<FnTypeNameNode>(typeName);
			s += to_string(t->returnType, compiler, asOperatorName) + " -> (";

			for (size_t i = 0; i < t->paramTypes.size(); ++i) {
				if (i)
					s += ", ";
				s += to_string(t->paramTypes[i], compiler, asOperatorName);
			}

			s += ")";
			return s;
		}
		case Type::Custom: {
			slake::slkc::Ref ref;
			compiler->_getFullName((MemberNode *)compiler->resolveCustomType((CustomTypeNameNode*)typeName.get()).get(), ref);
			return (asOperatorName ? "" : "@") + to_string(ref, compiler);
		}
		default:
			throw std::logic_error("Unrecognized type");
	}
}
