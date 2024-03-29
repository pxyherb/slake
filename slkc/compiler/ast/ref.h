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

		using Ref = deque<RefEntry>;

		class ThisRefNode : public AstNode {
		public:
			inline ThisRefNode() = default;
			virtual ~ThisRefNode() = default;

			virtual inline Location getLocation() const override { throw std::logic_error("Should not get location of a this reference"); }

			virtual inline NodeType getNodeType() const override { return NodeType::ThisRef; }
		};

		class BaseRefNode : public AstNode {
		public:
			inline BaseRefNode() = default;
			virtual ~BaseRefNode() = default;

			virtual inline Location getLocation() const override { throw std::logic_error("Should not get location of a this reference"); }

			virtual inline NodeType getNodeType() const override { return NodeType::BaseRef; }
		};

		class Compiler;
	}
}

namespace std {
	string to_string(const slake::slkc::Ref &ref, slake::slkc::Compiler *compiler);
}

#endif
