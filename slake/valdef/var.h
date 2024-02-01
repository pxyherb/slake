#ifndef _SLAKE_VALDEF_VAR_H_
#define _SLAKE_VALDEF_VAR_H_

#include "member.h"
#include <slake/except.h>
#include <slake/type.h>

namespace slake {
	using VarFlags = uint8_t;
	constexpr static VarFlags
		VAR_REG = 0x01;

	class VarValue final : public MemberValue {
	protected:
		mutable ValueRef<slake::Value, false> value;
		Type type = TypeId::ANY;

		friend class Runtime;

	public:
		VarFlags flags;

		VarValue(Runtime *rt, AccessModifier access, Type type, VarFlags flags = 0);
		virtual ~VarValue();

		virtual inline Type getType() const override { return TypeId::VAR; }
		inline Type getVarType() const { return type; }

		virtual Value *duplicate() const override;

		virtual inline MemberValue *getMember(std::string name) override {
			return value ? value->getMember(name) : nullptr;
		}
		virtual inline const MemberValue *getMember(std::string name) const override {
			return value ? value->getMember(name) : nullptr;
		}

		Value *getData() const { return value.get(); }
		void setData(Value *value) {
			if (value && !isCompatible(type, value->getType()))
				throw MismatchedTypeError("Mismatched types");
			this->value = value;
		}

		VarValue &operator=(const VarValue &x) {
			((MemberValue &)*this) = (MemberValue &)x;
			if (x.value)
				value = x.value->duplicate();
			type = x.type;
			return *this;
		}
		VarValue &operator=(VarValue &&) = delete;
	};
}

#endif
