using System;
using System.ComponentModel;
using System.Reflection;

namespace ShaderCompiler
{
	class EnumException : Exception
	{
		public EnumException(string message) :
			base(message)
		{
		}
	}

	static class EnumUtils
	{
		public static string ToDescription<T>(T value) where T : struct
		{
			Type type = value.GetType();
			if (!type.IsEnum)
			{
				throw new EnumException("Value must be of Enum type");
			}

			// Tries to find a DescriptionAttribute for a potential friendly name
			// for the enum
			MemberInfo[] memberInfo = type.GetMember(value.ToString());
			if (memberInfo != null && memberInfo.Length > 0)
			{
				object[] attrs = memberInfo[0].GetCustomAttributes(typeof(DescriptionAttribute), false);

				if (attrs != null && attrs.Length > 0)
				{
					// Pull out the description value
					return ((DescriptionAttribute) attrs[0]).Description;
				}
			}

			// If we have no description attribute, just return the ToString of the enum
			return value.ToString();
		}

		public static T FromDescription<T>(string description) where T : struct
		{
			Type type = typeof(T);
			if (!type.IsEnum)
			{
				throw new EnumException("Template must be of Enum type");
			}

			foreach (var field in type.GetFields())
			{
				DescriptionAttribute attribute = Attribute.GetCustomAttribute(field, typeof(DescriptionAttribute)) as DescriptionAttribute;
				if (attribute != null)
				{
					if (attribute.Description.Equals(description, StringComparison.InvariantCultureIgnoreCase))
					{
						return (T) field.GetValue(null);
					}
				}
				else
				{
					if (field.Name.Equals(description, StringComparison.InvariantCultureIgnoreCase))
					{
						return (T) field.GetValue(null);
					}
				}
			}

			throw new EnumException("Invalid enum description \"" + description + "\"");
		}

		public static void ForEach<T>(Action<T> action)
		{
			foreach (T t in (T[]) Enum.GetValues(typeof(T)))
			{
				action(t);
			}
		}
	}
}