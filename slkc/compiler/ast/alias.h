#ifndef _SLKC_COMPILER_AST_ALIAS_H_
#define _SLKC_COMPILER_AST_ALIAS_H_

#include "typename.h"
#include "member.h"
#include <variant>

namespace slake {
	namespace slkc {
		class AliasNode : public MemberNode {
		private:
			Location _loc;

		public:
			string name;
			Ref target;

			inline AliasNode(Location loc, Compiler *compiler, string name, Ref target)
				: MemberNode(compiler, ACCESS_PUB), _loc(loc), name(name), target(target) {}
			virtual ~AliasNode() = default;

			virtual inline Location getLocation() const override { return _loc; }

			virtual inline NodeType getNodeType() const override { return NodeType::Alias; }

			virtual inline RefEntry getName() const { return { _loc, name, genericArgs }; }
		};
	}
}

#endif
