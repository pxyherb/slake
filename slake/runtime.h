#ifndef _SLAKE_RUNTIME_H_
#define _SLAKE_RUNTIME_H_

#include <sstream>
#include <thread>
#include <unordered_set>
#include <set>
#include <memory>
#include <slake/slxfmt.h>

#include "except.h"
#include "generated/config.h"
#include "util/debug.h"
#include "value.h"

namespace slake {
	/// @brief Minor frames which are created by ENTER instructions and
	/// destroyed by LEAVE instructions.
	struct MinorFrame final {
		std::deque<uint32_t> exceptHandlers;  // Exception handlers
		uint32_t exitOff;					  // Offset of instruction to jump when leaving unexpectedly
		uint32_t stackBase;
	};

	/// @brief Major frames which represent a single calling frame.
	struct MajorFrame final {
		ValueRef<> scopeValue;					   // Scope value.
		ValueRef<const FnValue> curFn;			   // Current function
		uint32_t curIns = 0;					   // Offset of current instruction in function body
		std::deque<ValueRef<>> argStack;		   // Argument stack
		std::deque<ValueRef<>> nextArgStack;	   // Next Argument stack
		std::deque<ValueRef<>> dataStack;		   // Data stack
		std::deque<ValueRef<VarValue>> localVars;  // Local variables
		ValueRef<> thisObject;					   // `this' object
		ValueRef<> returnValue;					   // Return value
		std::deque<MinorFrame> minorFrames;		   // Minor frames

		inline ValueRef<> lload(uint32_t off) {
			if (off >= localVars.size())
				throw InvalidLocalVarIndexError("Invalid local variable index", off);

			return localVars.at(off);
		}

		inline void push(ValueRef<> v) {
			if (dataStack.size() > SLAKE_STACK_MAX)
				throw StackOverflowError("Stack overflowed");
			dataStack.push_back(*v);
		}

		inline ValueRef<> pop() {
			if (!dataStack.size())
				throw FrameBoundaryExceededError("Frame bottom exceeded");
			ValueRef<> v = dataStack.back();
			dataStack.pop_back();
			return v;
		}

		inline void expand(uint32_t n) {
			if ((n += (uint32_t)dataStack.size()) > SLAKE_STACK_MAX)
				throw StackOverflowError("Stack overflowed");
			dataStack.resize(n);
		}

		inline void shrink(uint32_t n) {
			if (n > ((uint32_t)dataStack.size()))
				throw StackOverflowError("Stack overflowed");
			dataStack.resize(n);
		}
	};

	struct Context final {
		std::deque<MajorFrame> majorFrames;	 // Major frames, aka calling frames

		inline MajorFrame &getCurFrame() {
			return majorFrames.back();
		}
	};

	using RuntimeFlags = uint32_t;
	constexpr static RuntimeFlags
		// No JIT, do not set unless you want to debug the JIT engine.
		RT_NOJIT = 0x0000001,
		// Enable Debugging, set for module debugging.
		RT_DEBUG = 0x0000002,
		// GC Debugging, do not set unless you want to debug the garbage collector.
		RT_GCDBG = 0x0000004,
		// The runtime is in a GC cycle.
		_RT_INGC = 0x40000000,
		// The runtime is deleting by user.
		_RT_DELETING = 0x80000000;

	using ModuleLocatorFn = std::function<ValueRef<ModuleValue>(Runtime *rt, RefValue *ref)>;

	using LoadModuleFlags = uint8_t;
	constexpr LoadModuleFlags
		LMOD_NOMODNAME = 0x01;

	class Runtime final {
	private:
		/// @brief Root value of the runtime.
		RootValue *_rootValue;

		/// @brief Contains all created values.
		std::unordered_set<Value *> _createdValues;

		/// @brief Extra target values for GC, all contained values will be released by the
		/// garbage collector every GC cycle and this container will be cleared if the cycle
		/// was completed.
		std::unordered_set<Value *> _extraGcTargets;

		struct GenericLookupEntry {
			Value *originalValue;
			GenericArgList genericArgs;
		};
		std::map<Value *, GenericLookupEntry> _genericCacheLookupTable;

		using GenericCacheTable =
			std::map<
				GenericArgList,	 // Generic arguments.
				Value *,		 // Cached instantiated value.
				GenericArgListComparator>;

		using GenericCacheDirectory = std::map<
			const Value *,	// Original generic value.
			GenericCacheTable>;

		/// @brief Cached generic instances for generic values.
		mutable GenericCacheDirectory _genericCacheDir;

		inline void invalidateGenericCache(Value* i) {
			if (_genericCacheLookupTable.count(i)) {
				// Remove the value from generic cache if it is unreachable.
				auto &lookupEntry = _genericCacheLookupTable.at(i);

				auto &table = _genericCacheDir.at(lookupEntry.originalValue);
				table.erase(lookupEntry.genericArgs);

				if(!table.size())
					_genericCacheLookupTable.erase(lookupEntry.originalValue);

				_genericCacheLookupTable.erase(i);
			}
		}

		/// @brief Runtime flags.
		RuntimeFlags _flags = 0;

		/// @brief Size of memory allocated for values.
		size_t _szMemInUse = 0;
		/// @brief Size of memory allocated for values after last GC cycle.
		size_t _szMemUsedAfterLastGc = 0;

		/// @brief Module locator for importing.
		ModuleLocatorFn _moduleLocator;

		RefValue *_loadRef(std::istream &fs);
		Value *_loadValue(std::istream &fs);
		Type _loadType(std::istream &fs, slxfmt::Type vt);
		GenericParam _loadGenericParam(std::istream &fs);
		void _loadScope(ModuleValue *mod, std::istream &fs);

		/// @brief Execute a single instruction.
		/// @param context Context for execution.
		/// @param ins Instruction to be executed.
		///
		/// @note Opcode-callback map was not introduced because designated initialization
		/// was not supported in ISO C++17.
		void _execIns(Context *context, Instruction &ins);

		void _gcWalk(Type &type);
		void _gcWalk(Value *i);

		void _instantiateGenericValue(Type &type, const GenericArgList &genericArgs) const;
		void _instantiateGenericValue(Value *v, const GenericArgList &genericArgs) const;

		ObjectValue *_newClassInstance(ClassValue *cls);
		ObjectValue *_newGenericClassInstance(ClassValue *cls, GenericArgList &genericArgs);

		void _callFn(Context *context, FnValue *fn);
		VarValue *_addLocalVar(MajorFrame &frame, Type type);

		friend class Value;
		friend class FnValue;
		friend class ObjectValue;
		friend class MemberValue;
		friend class ModuleValue;
		friend class ValueRef<ObjectValue>;

	public:
		/// @brief Active context on threads.
		std::map<std::thread::id, std::shared_ptr<Context>> activeContexts;

		/// @brief Thread IDs of threads which are executing destructors.
		std::unordered_set<std::thread::id> destructingThreads;

		Runtime(Runtime &) = delete;
		Runtime(Runtime &&) = delete;
		Runtime &operator=(Runtime &) = delete;
		Runtime &operator=(Runtime &&) = delete;

		Runtime(RuntimeFlags flags = 0);
		virtual ~Runtime();

		Value *instantiateGenericValue(const Value *v, const std::deque<Type> &genericArgs) const;

		/// @brief Resolve a reference to referred value.
		/// @param ref Reference to be resolved.
		/// @param scopeValue Scope value for resolving.
		/// @return Resolved value which is referred by the reference.
		Value *resolveRef(ValueRef<RefValue> ref, Value *scopeValue = nullptr) const;

		ValueRef<ModuleValue> loadModule(std::istream &fs);
		ValueRef<ModuleValue> loadModule(const void *buf, size_t size);

		inline RootValue *getRootValue() { return _rootValue; }

		inline void setModuleLocator(ModuleLocatorFn locator) { _moduleLocator = locator; }
		inline ModuleLocatorFn getModuleLocator() { return _moduleLocator; }

		std::string resolveName(const MemberValue *v) const;

		/// @brief Get active context on specified thread.
		/// @param id ID of specified thread.
		/// @return Active context on specified thread.
		inline std::shared_ptr<Context> getActiveContext(std::thread::id id = std::this_thread::get_id()) {
			return activeContexts.at(id);
		}

		void gc();

		std::string mangleName(
			std::string name,
			std::deque<Type> params,
			GenericArgList genericArgs = {});
	};
}

#endif
