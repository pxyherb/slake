#ifndef _SLAKE_LIB_STD_H_
#define _SLAKE_LIB_STD_H_

#include <slake/runtime.h>

namespace Slake {
	namespace StdLib {
		extern ModuleValue *modStd;
		void load(Runtime *rt);
	}
}

#endif
