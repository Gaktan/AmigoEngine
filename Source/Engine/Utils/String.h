#pragma once

#include <string>
#include <vector>

namespace String
{
	// Convert string to float
	float ToFloat(const std::string& inStr)
	{
		return std::stof(inStr);
	}

	// Convert string to int
	int ToInt(const std::string& inStr)
	{
		return std::stoi(inStr);
	}

	// Split string into multiple strings using delimiter
	std::vector<std::string> Split(const std::string& inStr, const std::string& inDelimiter)
	{
		size_t pos_start	= 0;
		size_t pos_end		= 0;
		size_t delim_len	= inDelimiter.length();

		std::string token;
		std::vector<std::string> result;

		while ((pos_end = inStr.find(inDelimiter, pos_start)) != std::string::npos)
		{
			token = inStr.substr(pos_start, pos_end - pos_start);
			pos_start = pos_end + delim_len;
			result.push_back(token);
		}

		result.push_back(inStr.substr(pos_start));
		return result;
	}
}