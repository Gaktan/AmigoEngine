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

	// TODO: Shoundn't be in String.h but oh well
	// https://stackoverflow.com/questions/6089231/getting-std-ifstream-to-handle-lf-cr-and-crlf
	std::istream& GetLine(std::istream& inStream, std::string& outLine)
	{
		outLine.clear();

		// The characters in the stream are read one-by-one using a std::streambuf.
		// That is faster than reading them one-by-one using the std::istream.
		// Code that uses streambuf this way must be guarded by a sentry object.
		// The sentry object performs various tasks,
		// such as thread synchronization and updating the stream state.

		std::istream::sentry se(inStream, true);
		std::streambuf* stream_buffer = inStream.rdbuf();

		// Go through each character one by one to find and handle delimiters
		for (;;)
		{
			int c = stream_buffer->sbumpc();
			switch (c)
			{
			case '\n':
				return inStream;
			case '\r':
				if (stream_buffer->sgetc() == '\n')
					stream_buffer->sbumpc();
				return inStream;
			case std::streambuf::traits_type::eof():
				// Also handle the case when the last line has no line ending
				if (outLine.empty())
					inStream.setstate(std::ios::eofbit);
				return inStream;
			default:
				outLine += (char) c;
			}
		}
	}
}