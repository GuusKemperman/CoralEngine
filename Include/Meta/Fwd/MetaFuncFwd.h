#pragma once
#include "MetaAnyFwd.h"
#include "MetaTypeTraitsFwd.h"
#include "MetaFuncIdFwd.h"

namespace CE
{
	class MetaType;
	class MetaProps;

	enum class OperatorType
	{
		constructor,
		assign = 3,
		destructor = 5,
		negate,						// !   logical negation
		complement,					// ~   complement
		indirect,					// *   indirection
		address_of,					// &   address of
		add,						// +   addition
		subtract,					// -   subtraction
		multiplies,					// *   multiplication
		divides,					// /   division
		modulus,					// %   modulo
		equal,						// ==  equality
		inequal,					// !=  inequality
		greater,					// >   greater than
		less,						// <   less than
		greater_equal,				// >=  greater or equal than
		less_equal,					// <=  less or equal than
		logical_and,				// &&  logical and
		logical_or,					// ||  logical or
		bitwise_and,				// &   bitwise and
		bitwise_or,					// |   bitwise inclusive or
		bitwise_xor,				// ^   bitwise exclusive or
		left_shift,					// <<  left shift
		right_shift,				// >>  right shift
		add_assign,					// +=  addition assignment
		subtract_assign,			// -=  subtractions assignment
		multiplies_assign,			// *=  multiplication assignment
		divides_assign,				// /=  division assignment
		modulus_assign,				// %=  modulo assignment
		right_shift_assign,			// >>= right shift assignment
		left_shift_assign,			// <<= left shift assignment
		bitwise_and_assign,			// &=  bitwise and assignment
		bitwise_xor_assign,			// ^=  bitwise exclusive or assignment
		bitwise_or_assign,			// |=  bitwise inclusive or assignment
		arrow_indirect,				// ->* pointer to field
		comma,						// ,   comma
		subscript,					// []  subscription
		arrow,						// ->  class field
		dot,						// .   class field
		dot_indirect,				// .*  pointer to field
		increment = 45,				// ++
		decrement,					// --
		none = 44,
	};

	std::string_view GetNameOfOperator(OperatorType type);

	struct MetaFuncNamedParam
	{
		MetaFuncNamedParam(TypeTraits typeTraits = {}, std::string_view name = {}) :
			mTypeTraits(typeTraits),
			mName(name) {}

		bool operator==(const MetaFuncNamedParam& other) const { return mTypeTraits == other.mTypeTraits && mName == other.mName; };
		bool operator!=(const MetaFuncNamedParam& other) const { return mTypeTraits != other.mTypeTraits || mName != other.mName; };

		TypeTraits mTypeTraits{};
		std::string mName{};
	};

	template<class T>
	concept StringLike = std::is_convertible_v<T, std::string_view>;

	/*
	FuncResult contains information about wether the function call succeeded. Examples of reasons
	why a call can fail are if one of the arguments is of the wrong type, if the number of arguments
	provided does not meet the number of arguments expected, or if you provide a null value for a non-nullable type.
	*/
	class FuncResult
	{
	public:
		template<typename... T>
		constexpr FuncResult(T&&... args) :
			mResult(std::forward<T>(args)...) {}

		/*
		Will Assert if the result does not have a return value.
		This can happen if the function returns void, or if the
		function failed to be invoked.

		Use HasReturnValue() to test if this function is safe to call.
		*/
		MetaAny& GetReturnValue();


		/*
		Will return false if the function call failed or if the function returned void.
		*/
		bool HasReturnValue() const;

		/*
		Check if the function was invoked succesfully. IF this returns true,
		you can obtain more information by calling FuncResult::Error
		*/
		bool HasError() const;

		// If the function was not invoked, this will return a string with the reason WHY it failed.
		std::string Error() const;

	private:
		// Basically used as std::expected
		std::variant<std::optional<MetaAny>, std::string> mResult{};
	};

	/*
	A wrapper around a function. Functions can be called from C++. If they have the Props::sIsScriptableTag property, they can also be called
	from scripts.

	Construct example:
		static bool DistributeMoney(int numOfPeople, const float& amountOfMoney, std::vector<float>* howMuchEachPersonReceived);

		// Any C++ function can be easily added, the return type and parameters are detected automatically.
		MetaFunc func1{&DistributeMoney, "Print"};

		// You optionally give names to the parameters and the return value.
		MetaFunc func7{&DistributeMoney, "Print", "numOfPeople", "amountOfMoney", "howMuchEachPersonReceived", "wasEvenlyDistributed" };

		// Lambdas without parameters can be trivially added as well
		MetaFunc func2{[]() { return 42; }, "ReturnTheAnswerToLifeTheUniverseAndEverything"};

		// For lambdas and other functors with parameters, the parameters have to be explicitely provided
		MetaFunc func3{[](int32 i, int32 j) { return i + j; }, "LambdaWithParams" };
		MetaFunc func4{std::equal<int32>(), "Functor" };

		// In order to distinquish between overloads, you may have to cast it to the correct signature first.
		// This is not specifically related to the MetaFunc class, but a restraint enforced by C++ itself.
		MetaFunc func5{static_cast<float(*)(glm::vec2, glm::vec2)>(&glm::dot), "Dot" };
	*/
	class MetaFunc
	{
	public:
		using DynamicArg = MetaAny;
		using DynamicArgs = std::span<DynamicArg>;

		// The address at which the return value will be allocated.
		// Can reduce the amount of mallocs/frees. Is completely optional.
		// When writing your own Invoke function, you may ignore this argument
		// if you are returning void or a reference/pointer.
		using RVOBuffer = void*;
		using InvokeT = std::function<FuncResult(DynamicArgs, RVOBuffer)>;
		using NameOrType = std::variant<std::string, OperatorType>;
		using NameOrTypeInit = std::variant<std::string_view, OperatorType>;
		using Param = MetaFuncNamedParam;
		using Params = std::vector<MetaFuncNamedParam>;
		using Return = MetaFuncNamedParam;

		MetaFunc(const InvokeT& func,
			NameOrTypeInit nameOrType,
			const Return& returnType,
			const Params& parameters);

	private:
		struct ExplicitlyUseThisConstructor{};
		explicit MetaFunc(InvokeT&& func,
			NameOrType&& nameOrOp,
			std::initializer_list<TypeTraits> paramTypes,
			std::initializer_list<std::string_view> paramNames,
			FuncId funcId,
			ExplicitlyUseThisConstructor ctor);

		MetaFunc(InvokeT&& func,
			OperatorType op,
			std::initializer_list<TypeTraits> paramTypes,
			std::initializer_list<std::string_view> paramNames,
			FuncId funcId);

		MetaFunc(InvokeT&& func,
			std::string_view name,
			std::initializer_list<TypeTraits> paramTypes,
			std::initializer_list<std::string_view> paramNames,
			FuncId funcId);

	public:
		template<typename Ret, typename... ParamsT, typename NameOrOperator, StringLike... ParamAndRetNames>
		MetaFunc(std::function<Ret(ParamsT...)>&& func,
			NameOrOperator&& nameOrOp,
			ParamAndRetNames&&... paramAndRetNames);

		template<typename Functor, typename NameOrOperator, StringLike... ParamAndRetNames>
		MetaFunc(Functor&& functor, 
			NameOrOperator&& nameOrType,
			ParamAndRetNames&&... paramAndRetNames);

		// Mutable member function
		template<typename Ret, typename Obj, typename... ParamsT, typename NameOrOperator, StringLike... ParamAndRetNames>
		MetaFunc(Ret(Obj::* func)(ParamsT...), NameOrOperator&& nameOrOp, ParamAndRetNames&&... paramAndRetNames);

		// Const member function
		template<typename Ret, typename Obj, typename... ParamsT, typename NameOrOperator, StringLike... ParamAndRetNames>
		MetaFunc(Ret(Obj::* func)(ParamsT...) const, NameOrOperator&& nameOrOp, ParamAndRetNames&&... paramAndRetNames);

		// Static function
		template<typename Ret, typename... ParamsT, typename NameOrOperator, StringLike... ParamAndRetNames>
		MetaFunc(Ret(*func)(ParamsT...), NameOrOperator&& typeOrName, ParamAndRetNames&&... paramAndRetNames);

		MetaFunc(MetaFunc&& other) noexcept;
		MetaFunc(const MetaFunc&) = delete;

		~MetaFunc();

		MetaFunc& operator=(MetaFunc&& other) noexcept;
		MetaFunc& operator=(const MetaFunc&) = delete;

		uint32 GetFuncId() const { return mFuncId; }

		const MetaFuncNamedParam& GetReturnType() const { return mReturn; }
		const std::vector<MetaFuncNamedParam>& GetParameters() const { return mParams; }

		// If this is an operator, this will return the name of the operator
		std::string_view GetDesignerFriendlyName() const;

		// If this is an operator, this will return the name of the operator
		static std::string_view GetDesignerFriendlyName(const NameOrType& nameOrType);

		const NameOrType& GetNameOrType() const { return mNameOrType; }

		/*
		Call the function with the provided arguments.

		Example:
			// The function adds two integers and returns the result
			MetaFunc& add = ...;

			FuncResult result1 = add(1, 4);

			if (result1.HasError())
			{
				std::cout << result1.GetError() << std::endl;
				return;
			}

			Any& returnValue = result1.GetReturnValue();
			std::cout << *returnValue.As<int>() << std::endl; // prints 5

			FuncResult result2 = add(returnValue, -10);

			if (result2.HasError())
			{
				std::cout << result2.GetError() << std::endl;
				return;
			}

			std::cout << *result2.GetReturnValue().As<int>() << std::endl; // prints -5;
		*/
		template<typename... Arg>
		FuncResult operator()(Arg&&... args) const;

		/*
		Will call the function without checking if the arguments match the parameters.
		*/
		template<typename... Args>
		FuncResult InvokeCheckedUnpacked(Args&&... args) const;

		/*
		Will call the function without checking if the arguments match the parameters.

		If an RVOBuffer is provided, the return value will be allocated at that
		address. You are then responsible for calling the destructor and free.
		*/
		template<typename... Args>
		FuncResult InvokeCheckedUnpackedWithRVO(RVOBuffer rvoBuffer, Args&&... args) const;

		/*
		args.size() must be formOfArgs.size(). formOfArgs[N] represents how
		args[N] was passed in. This allows us to check if, for example, a const
		reference was passed to a function whose parameter is a mutable reference.

		If an RVOBuffer is provided, the return value will be allocated at that
		address. You are then responsible for calling the destructor and free.
		*/
		FuncResult InvokeChecked(DynamicArgs args, std::span<const TypeForm> formOfArgs, RVOBuffer rvoBuffer = nullptr) const;

		/*
		Will call the function without checking if the arguments match the parameters.
		*/
		template<typename... Args>
		FuncResult InvokeUncheckedUnpacked(Args&&... args) const;

		/*
		Will call the function without checking if the arguments match the parameters.

		If an RVOBuffer is provided, the return value will be allocated at that
		address. You are then responsible for calling the destructor and free.
		*/
		template<typename... Args>
		FuncResult InvokeUncheckedUnpackedWithRVO(RVOBuffer rvoBuffer, Args&&... args) const;

		/*
		Will call the function without checking if the arguments match the parameters.

		If an RVOBuffer is provided, the return value will be allocated at that
		address. You are then responsible for calling the destructor and free.
		*/
		FuncResult InvokeUnchecked(DynamicArgs args, std::span<const TypeForm> formOfArgs, RVOBuffer rvoBuffer = nullptr) const;

		/*
		Checks if the arguments can be passed to the parameters.

		args.size() must be formOfArgs.size(). formOfArgs[N] represents how
		args[N] was passed in. This allows us to check if, for example, a const
		reference was passed to a function whose parameter is a mutable reference.

		Returns the reason why we cannot pass these arguments to the parameters.
		*/
		static std::optional<std::string> CanArgBePassedIntoParam(std::span<const DynamicArg> args,
			std::span<const TypeForm> formOfArgs,
			const std::vector<MetaFuncNamedParam>& params);

		/*
		Returns the reason why the arg could not be passed into param.
		*/
		static std::optional<std::string> CanArgBePassedIntoParam(const DynamicArg& arg, TypeForm formOfArg, TypeTraits param);

		/*
		Returns the reason why the arg could not be passed into param, purely by looking at the typeid and form.

		Cannot check if the argument was null, so if you have the argument, the above function is preferred.
		*/
		static std::optional<std::string> CanArgBePassedIntoParam(TypeTraits arg, TypeTraits param);

		const MetaProps& GetProperties() const { ASSERT(mProperties != nullptr); return *mProperties; }
		MetaProps& GetProperties() { ASSERT(mProperties != nullptr); return *mProperties; }

		// Used in ScriptFunc::DefineFunc
		void RedirectFunction(InvokeT&& func) { mFuncToInvoke = std::move(func); }

		template<typename... Args>
		static std::pair<std::array<DynamicArg, sizeof...(Args)>, std::array<TypeForm, sizeof...(Args)>> Pack(Args&&... args);

		template<typename Arg>
		static DynamicArg PackSingle(Arg&& arg);

		template<>
		DynamicArg PackSingle(const MetaAny& other);

		template<>
		DynamicArg PackSingle(MetaAny& other);

	private:
		template<typename Ret, typename... ParamsT>
		static FuncResult DefaultInvoke(const std::function<Ret(ParamsT...)>& functionToInvoke, DynamicArgs runtimeArgs, RVOBuffer rvoBuffer);

		MetaFuncNamedParam mReturn;
		std::vector<MetaFuncNamedParam> mParams;

		NameOrType mNameOrType{};
		std::unique_ptr<MetaProps> mProperties;

		FuncId mFuncId{};
		InvokeT mFuncToInvoke{};
	};
}