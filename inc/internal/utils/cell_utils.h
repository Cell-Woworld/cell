#ifndef __cell_utils_h__
#define __cell_utils_h__

#include "IBiomolecule.h"
#include "internal/utils/nlohmann/json.hpp"
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

BIO_END_NAMESPACE
#endif
