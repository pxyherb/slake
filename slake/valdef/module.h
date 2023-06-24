#ifndef _SLAKE_VALDEF_MODULE_H_
#define _SLAKE_VALDEF_MODULE_H_

#include "member.h"
#include <unordered_map>

namespace Slake {
	class ModuleValue : public MemberValue {
	protected:
		std::unordered_map<std::string, MemberValue *> _members;

		friend class Runtime;

	public:
		inline ModuleValue(Runtime *rt, AccessModifier access, Value *parent = nullptr, std::string name = "") : MemberValue(rt, access, parent, name) {
			reportSizeToRuntime(sizeof(*this));
		}
		virtual inline ~ModuleValue() {
			if (!getRefCount())
				for (auto &i : _members)
					i.second->decRefCount();
		}

		virtual inline MemberValue *getMember(std::string name) override { return _members.count(name) ? _members.at(name) : nullptr; }
		virtual inline const MemberValue *getMember(std::string name) const override { return _members.count(name) ? _members.at(name) : nullptr; }
		virtual inline Type getType() const override { return ValueType::MOD; }

		virtual inline void addMember(std::string name, MemberValue *value) {
			if (_members.count(name)) {
				_members.at(name)->unbind();
				_members.at(name)->decRefCount();
			}
			_members[name] = value;
			value->incRefCount();
			value->bind(this, name);
		}

		virtual inline std::string toString() const override {
			std::string s = MemberValue::toString() + ",\"members\":{";

			for (auto i = _members.begin(); i != _members.end(); ++i) {
				s += (i != _members.begin() ? ",\"" : "\"") + i->first + "\":" + std::to_string((uintptr_t)i->second);
			}

			s += "}";

			return s;
		}

		decltype(_members)::iterator begin() { return _members.begin(); }
		decltype(_members)::iterator end() { return _members.end(); }
		decltype(_members)::const_iterator begin() const { return _members.begin(); }
		decltype(_members)::const_iterator end() const { return _members.end(); }

		ModuleValue &operator=(const ModuleValue &) = delete;
		ModuleValue &operator=(const ModuleValue &&) = delete;
	};
}

#endif
