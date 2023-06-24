#ifndef _SLKBCC_SCOPE_H_
#define _SLKBCC_SCOPE_H_

#include "var.h"
#include "class.h"
#include "interface.h"
#include "fn.h"

namespace Slake {
	namespace Assembler {
		class Scope {
		public:
			unordered_map<string, shared_ptr<Var>> vars;
			unordered_map<string, shared_ptr<Class>> classes;
			unordered_map<string, shared_ptr<Interface>> interfaces;
			unordered_map<string, shared_ptr<Fn>> funcs;

			Scope() = default;
			virtual ~Scope() = default;
		};

		extern shared_ptr<Scope> rootScope, curScope;
		extern shared_ptr<Ref> moduleName;
	}
}

#endif
