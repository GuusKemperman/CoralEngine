#include "Precomp.h"
#include "Scripting/Compiler/IRBuilder.h"

CE::IRBuilder& CE::IRBuilder::AddClass(std::string name)
{
	mClasses.emplace_back(std::move(name));
}

CE::IRBuilder& CE::IRBuilder::EndWhile()
{
	GoTo("UniqueWhileName");
	EndIf();

	bool condition;

	{
	myLabel:
		if (condition)
		{
			goto myLabel;
		}
	}

	for (int i = 0; i < 10; i++)
	{

	}

	{
		For()
			.DeclareVariable("int", "i", "0")
			.ForCondition()
			.DeclareVariable("bool", "ForCondition")
			.DeclareVariable("int", "Stop", "10")
			.Invoke("int", "less")
			.InvokeArgument("i")
			.InvokeArgument("Stop")
			.InvokeResult("ForCondition")
			.ForUpdate()
			.Invoke("int", "increment", { "i" }, {})
			.ForBody()
			.Invoke("string", "print", { "Hello world" }, {})
			.ForEnd();
	}
}

CE::IRBuilder& CE::IRBuilder::For()
{
	Scope();
}

CE::IRBuilder& CE::IRBuilder::ForCondition()
{
	Scope();
	DeclareVariable("bool", "ForCondition");
}

CE::IRBuilder& CE::IRBuilder::ForUpdate()
{
	If("ForCondition");
	GoTo("UniqueForBody");
	Else();
	GoTo("UniqueEndFor");
	EndIf();

	EndScope();
	Scope();
}

CE::IRBuilder& CE::IRBuilder::ForBody()
{
	Label("UniqueForBody");

	EndScope();
}

CE::IRBuilder& CE::IRBuilder::ForEnd()
{
	EndScope();
	Label("UniqueEndFor");
}
