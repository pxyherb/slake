#ifndef _SLAKE_EXCEPT_H_
#define _SLAKE_EXCEPT_H_

#include <stdexcept>
#include <slake/valdef/ref.h>

namespace slake {
	class RuntimeExecError : public std::runtime_error {
	public:
		inline RuntimeExecError(std::string msg) : runtime_error(msg){};
		virtual ~RuntimeExecError() = default;
	};

	class OutOfFnBodyError : public RuntimeExecError {
	public:
		inline OutOfFnBodyError(std::string msg) : RuntimeExecError(msg){};
		virtual ~OutOfFnBodyError() = default;
	};

	/// @brief Raises when mismatched types were detected.
	class MismatchedTypeError : public RuntimeExecError {
	public:
		inline MismatchedTypeError(std::string msg) : RuntimeExecError(msg){};
		virtual ~MismatchedTypeError() = default;
	};

	/// @brief Raises when incompatible types were detected.
	class IncompatibleTypeError : public RuntimeExecError {
	public:
		inline IncompatibleTypeError(std::string msg) : RuntimeExecError(msg){};
		virtual ~IncompatibleTypeError() = default;
	};

	/// @brief Raises when executing instructions with invalid opcode.
	class InvalidOpcodeError : public RuntimeExecError {
	public:
		inline InvalidOpcodeError(std::string msg) : RuntimeExecError(msg){};
		virtual ~InvalidOpcodeError() = default;
	};

	/// @brief Raises when executing instructions with invalid operand combination.
	class InvalidOperandsError : public RuntimeExecError {
	public:
		inline InvalidOperandsError(std::string msg) : RuntimeExecError(msg){};
		virtual ~InvalidOperandsError() = default;
	};

	class InvalidArgumentsError : public RuntimeExecError {
	public:
		inline InvalidArgumentsError(std::string msg = "Invalid arguments") : RuntimeExecError(msg){};
		virtual ~InvalidArgumentsError() = default;
	};

	class NotFoundError : public RuntimeExecError {
	public:
		ValueRef<RefValue> ref;

		NotFoundError(std::string msg, ValueRef<RefValue> ref);
		virtual ~NotFoundError() = default;
	};

	class AccessViolationError : public RuntimeExecError {
	public:
		inline AccessViolationError(std::string msg) : RuntimeExecError(msg){};
		virtual ~AccessViolationError() = default;
	};

	class UncaughtExceptionError : public RuntimeExecError {
	public:
		inline UncaughtExceptionError(std::string msg) : RuntimeExecError(msg){};
		virtual ~UncaughtExceptionError() = default;
	};

	class AbortedError : public RuntimeExecError {
	public:
		inline AbortedError(std::string msg) : RuntimeExecError(msg){};
		virtual ~AbortedError() = default;
	};

	class FrameBoundaryExceededError : public RuntimeExecError {
	public:
		inline FrameBoundaryExceededError(std::string msg) : RuntimeExecError(msg){};
		virtual ~FrameBoundaryExceededError() = default;
	};

	class InvalidSubscriptionError : public RuntimeExecError {
	public:
		inline InvalidSubscriptionError(std::string msg) : RuntimeExecError(msg){};
		virtual ~InvalidSubscriptionError() = default;
	};

	class FrameError : public RuntimeExecError {
	public:
		inline FrameError(std::string msg) : RuntimeExecError(msg){};
		virtual ~FrameError() = default;
	};

	class StackOverflowError : public RuntimeExecError {
	public:
		inline StackOverflowError(std::string msg) : RuntimeExecError(msg){};
		virtual ~StackOverflowError() = default;
	};

	class LoaderError : public RuntimeExecError {
	public:
		inline LoaderError(std::string msg) : RuntimeExecError(msg){};
		virtual ~LoaderError() = default;
	};

	class NullRefError : public RuntimeExecError {
	public:
		inline NullRefError(std::string msg = "Null reference detected") : RuntimeExecError(msg){};
		virtual ~NullRefError() = default;
	};
}

#endif
