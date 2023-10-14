///
/// @file scope.h
/// @brief Header file of the scope module.
///
/// @copyright Copyright (c) 2023 Slake contributors
///
///
#ifndef _SLKC_COMPILER_AST_SCOPE_H_
#define _SLKC_COMPILER_AST_SCOPE_H_

#include "astnode.h"

namespace slake {
	namespace slkc {
		class MemberNode;

		class Scope {
		public:
			weak_ptr<AstNode> owner;	  // Owner of this scope
			weak_ptr<AstNode> parent;  // Parent of the owner
			unordered_map<string, shared_ptr<MemberNode>> members;
		};
	}
}

#endif
