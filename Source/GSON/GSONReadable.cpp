#include "Precomp.h"
#include "GSON/GSONReadable.h"

void CE::ReadableGSONObject::SaveTo(std::ostream& ostream) const
{
	std::function<void(const ObjectType&, uint32)> save = [&](const ObjectType& current, uint32 numOfIndentations)
		{
			std::string indentation = "";

			const ObjectType* const parent = current.GetParent();

			if (parent != nullptr)
			{
				for (uint8_t i = 0; i < numOfIndentations; i++)
				{
					indentation += '\t';
				}

				ostream << indentation << current.GetName() << " {\n";
				++numOfIndentations;
			}

			for (const ObjectType& child : current.GetChildren())
			{
				save(child, numOfIndentations);
			}

			for (const MemberType& member : current.GetGSONMembers())
			{
				ostream << indentation << '\t' << member.GetName() << " = " << member.GetData() << '\n';
			}

			if (parent != nullptr)
			{
				ostream << indentation << "}\n";
			}
		};
	save(*this, 0);
}

void CE::ReadableGSONObject::LoadFrom(std::istream& istream)
{
	std::string line;

	const auto removeWhiteSpaceAndLineEndings = [](std::string& in)
		{
			in.erase(std::remove_if(in.begin(), in.end(),
				[](unsigned char x)
				{
					return x == '\t'
						|| x == '\n'
						|| x == '\r';
				}), in.end());
		};

	while (std::getline(istream, line))
	{
		const size_t lineSize = line.size();
		char ch;
		for (size_t i = 0; i < lineSize; i++)
		{
			ch = line[i];

			// If line contains {, consider it the start of a new GSONObject.
			if (ch == '{')
			{
				removeWhiteSpaceAndLineEndings(line);

				line.pop_back(); // Remove '{'
				line.pop_back(); // Remove ' '

				ObjectType& newChild = AddGSONObject(line);
				newChild.LoadFrom(istream);
				break;
			}

			if (ch == '=')
			{
				std::string memberName = line.substr(0, i - 1);

				removeWhiteSpaceAndLineEndings(memberName);

				std::string memberVal = line.substr(i + 2);
				removeWhiteSpaceAndLineEndings(memberVal);

				MemberType& newMember = AddGSONMember(memberName);
				newMember.mData = std::move(memberVal);
				break;
			}

			// If a line contains }, consider it the end of this location.
			if (ch == '}')
			{
				return;
			}
		}
	}
}
