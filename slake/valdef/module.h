#ifndef _SLAKE_VALDEF_MODULE_H_
#define _SLAKE_VALDEF_MODULE_H_

#include "member.h"
#include <unordered_map>
#include <map>

namespace slake {
	class ModuleValue : public MemberValue {
	public:
		std::unordered_map<std::string, RefValue *> imports;

		ModuleValue(Runtime *rt, AccessModifier access);
		virtual ~ModuleValue();

		virtual Type getType() const override;

		virtual Value *duplicate() const override;

		inline ModuleValue &operator=(const ModuleValue &x) {
			((MemberValue &)*this) = (MemberValue &)x;

			return *this;
		}
		ModuleValue &operator=(ModuleValue &&) = delete;
	};
}

#endif
