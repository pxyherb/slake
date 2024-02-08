#ifndef _SLKC_COMPILER_AST_REF_H_
#define _SLKC_COMPILER_AST_REF_H_

#include "astnode.h"

namespace slake {
	namespace slkc {
		class TypeNameNode;

		struct RefEntry {
			Location loc;
			string name;
			deque<shared_ptr<TypeNameNode>> genericArgs;

			inline RefEntry(Location loc, string name, deque<shared_ptr<TypeNameNode>> genericArgs = {})
				: loc(loc), name(name), genericArgs(genericArgs) {}
		};

		struct ModuleRefEntry {
			Location loc;
			string name;

			inline ModuleRefEntry(Location loc, string name)
				: loc(loc), name(name) {}

			inline bool operator<(const ModuleRefEntry&rhs) const {
				if(name < rhs.name)
					return true;
				if(name > rhs.name)
					return false;
				return loc < rhs.loc;
			}
		};

		using Ref = deque<RefEntry>;
		using ModuleRef = deque<ModuleRefEntry>;

		Ref toRegularRef(const ModuleRef &ref);
		ModuleRef toModuleRef(const Ref &ref);

		class ThisRefNode : public AstNode {
		public:
			inline ThisRefNode() = default;
			virtual ~ThisRefNode() = default;

			virtual inline Location getLocation() const override { throw std::logic_error("Should not get location of a this reference"); }

			virtual inline NodeType getNodeType() const override { return AST_THIS_REF; }
		};

		class BaseRefNode : public AstNode {
		public:
			inline BaseRefNode() = default;
			virtual ~BaseRefNode() = default;

			virtual inline Location getLocation() const override { throw std::logic_error("Should not get location of a this reference"); }

			virtual inline NodeType getNodeType() const override { return AST_BASE_REF; }
		};

		class Compiler;
	}
}

namespace std {
	string to_string(const slake::slkc::Ref &ref, slake::slkc::Compiler *compiler);
	string to_string(const slake::slkc::ModuleRef &ref);
}

// For module loading.
// We don't strictly compare them.
bool operator<(slake::slkc::ModuleRef lhs, slake::slkc::ModuleRef rhs);

#endif
