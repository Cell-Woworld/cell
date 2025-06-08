#ifndef __cell_utils_h__
#define __cell_utils_h__

#include "IBiomolecule.h"
#include "nlohmann/json.hpp"
#include <iomanip>

BIO_BEGIN_NAMESPACE

String EscapeJSONString(const String& in, bool escape_quote)
{
	std::ostringstream o;
	for (auto c = in.cbegin(); c != in.cend(); c++) {
		switch (*c) {
		case '"': {
			if (escape_quote)
				o << "\\\"";
			break;
		}
		case '\b': o << "\\b"; break;
		case '\f': o << "\\f"; break;
		case '\n': 
			o << "\\r\\n"; 
			break;
		case '\r': {
			if (c + 1 == in.cend() || *(c + 1) != '\n')
				o << "\\r\\n";
			break;
		}
		case '\t': o << "\\t"; break;
		case '\\': {
			if (c + 1 != in.cend())
			{
				switch (*(c + 1))
				{
				case '\\':
					if (escape_quote)
					{
						c++;
						o << "\\\\\\\\";
					}
					else
					{
						c++;
						o << "\\" << *c;
					}
					break;
				case '"':
					if (escape_quote)
					{
						c++;
						o << "\\\\\\\"";
					}
					else
					{
						c++;
						o << "\\" << *c;
					}
					break;
				case 'f':
				case 'n':
				case 'r':
				case 't':
					if (escape_quote)
					{
						c++;
						o << "\\\\" << *c;
					}
					else
					{
						c++;
						o << "\\" << *c;
					}
					break;
				case 'u':
				{
					o << "\\u";
					c += 2;
					for (int i = 0; i < 4 && c != in.cend(); i++, c++)
					{
						o << *c;
					};
					break;
				}
				default:
					if (escape_quote)
					{
						o << "\\\\";
					}
					else
					{
						o << "\\";
					}
					break;
				}
			}
			else
			{
				if (escape_quote)
				{
					o << "\\\\";
				}
				else
				{
					o << "\\";
				}
			}
			break;
		}
		default:
			if ('\x00' <= *c && *c <= '\x1f') {
				o << "\\u"
					<< std::hex << std::setw(4) << std::setfill('0') << (int)*c;
			}
			else {
				o << *c;
			}
		}
	}
	return o.str();
}

void EscapeJSON(const String& in, String& out)
{
	out = "";
	size_t _start_pos = in.find_first_of('\"');
	if (_start_pos != String::npos && _start_pos > 0)
		out += in.substr(0, _start_pos);
	size_t _end_pos = String::npos;
	while (_start_pos != String::npos && (_end_pos = in.find_first_of('\"', _start_pos + 1)) != String::npos)
	{
		if (_end_pos - _start_pos > 1)
			out += "\"" + EscapeJSONString(in.substr(_start_pos + 1, _end_pos - _start_pos - 1), false) + "\"";
		else
			out += "\"\"";
		_start_pos = in.find_first_of('\"', _end_pos + 1);
		out += in.substr(_end_pos + 1, _start_pos - _end_pos - 1);
	}
	if (_start_pos != String::npos && _start_pos > 0)
		out += in.substr(_start_pos);
}

static String decToHexa(int n)
{
	// char array to store hexadecimal number
	char hexaDeciNum[100];

	// counter for hexadecimal number array
	int i = 0;
	while (n != 0) {
		// temporary variable to store remainder
		int temp = 0;

		// storing remainder in temp variable.
		temp = n % 16;

		// check if temp < 10
		if (temp < 10) {
			hexaDeciNum[i] = temp + 48;
			i++;
		}
		else {
			hexaDeciNum[i] = temp + 55;
			i++;
		}

		n = n / 16;
	}

	String ans = "";

	// printing hexadecimal number array in reverse order
	for (int j = i - 1; j >= 0; j--)
		ans += hexaDeciNum[j];

	return ans;
}

// Function to convert ASCII to HEX
String ASCIItoHex(const String& ascii)
{
	// Initialize final String
	String hex = "";

	// Make a loop to iterate through
	// every character of ascii string
	for (int i = 0; i < ascii.length(); i++) {
		// Take a char from
		// position i of string
		char ch = ascii[i];

		// Cast char to integer and
		// find its ascii value
		int tmp = (int)ch;

		// Change this ascii value
		// integer to hexadecimal value
		String part = decToHexa(tmp);

		// Add this hexadecimal value
		// to final string.
		hex += part;
	}

	// Return the final
	// string hex
	return hex;
}

String HextoASCII(const String& hex)
{
	// initialize the ASCII code string as empty.
	String ascii = "";
	for (size_t i = 0; i < hex.length(); i += 2)
	{
		// extract two characters from hex string
		String part = hex.substr(i, 2);

		// change it into base 16 and
		// typecast as the character
		char ch = (char)stoul(part, nullptr, 16);

		// add this char to final ASCII string
		ascii += ch;
	}
	return ascii;
}

namespace strutil
{
	template <typename ITR>
	static inline void SplitStringToIteratorUsing(const String& full, const char* delim, ITR& result)
	{
		// Optimize the common case where delim is a single character.
		if (delim[0] != '\0' && delim[1] == '\0') {
			char c = delim[0];
			const char* p = full.data();
			const char* end = p + full.size();
			while (p != end) {
				if (*p == c) {
					++p;
				}
				else {
					const char* start = p;
					while (++p != end && *p != c);
					*result++ = String(start, p - start);
				}
			}
			return;
		}

		String::size_type begin_index, end_index;
		begin_index = full.find_first_not_of(delim);
		while (begin_index != String::npos) {
			end_index = full.find_first_of(delim, begin_index);
			if (end_index == String::npos) {
				*result++ = full.substr(begin_index);
				return;
			}
			*result++ = full.substr(begin_index, (end_index - begin_index));
			begin_index = full.find_first_not_of(delim, end_index);
		}
	}

	static inline void SplitStringUsing(const String& full,
		const char* delim,
		Array<String>* result) {
		std::back_insert_iterator< Array<String> > it(*result);
		SplitStringToIteratorUsing(full, delim, it);
	}

	template <typename StringType, typename ITR>
	static inline void SplitStringToIteratorAllowEmpty(const StringType& full, const char* delim, int pieces, ITR& result)
	{
		String::size_type begin_index, end_index;
		begin_index = 0;

		for (int i = 0; (i < pieces - 1) || (pieces == 0); i++) {
			end_index = full.find_first_of(delim, begin_index);
			if (end_index == String::npos) {
				*result++ = full.substr(begin_index);
				return;
			}
			*result++ = full.substr(begin_index, (end_index - begin_index));
			begin_index = end_index + 1;
		}
		*result++ = full.substr(begin_index);
	}

	static inline void SplitStringAllowEmpty(const String& full, const char* delim, Array<String>* result)
	{
		std::back_insert_iterator<Array<String> > it(*result);
		SplitStringToIteratorAllowEmpty(full, delim, 0, it);
	}
};

Array<String> split(const String& full, const char* delim, bool skip_empty = true)
{
	Array<String> result;
	if (skip_empty) {
		strutil::SplitStringUsing(full, delim, &result);
	}
	else {
		strutil::SplitStringAllowEmpty(full, delim, &result);
	}
	return result;
}

BIO_END_NAMESPACE
#endif
