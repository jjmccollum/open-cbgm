#ifndef OPEN_CBGM_JSON_STRING_H
#define OPEN_CBGM_JSON_STRING_H

#include <ostream>
#include <string>

namespace open_cbgm {

inline void write_json_string(std::ostream & out, const std::string & value) {
	const char hex[] = "0123456789abcdef";
	out.put('"');
	for (char ch : value) {
		switch (ch) {
		case '"': out << "\\\""; break;
		case '\\': out << "\\\\"; break;
		case '\b': out << "\\b"; break;
		case '\f': out << "\\f"; break;
		case '\n': out << "\\n"; break;
		case '\r': out << "\\r"; break;
		case '\t': out << "\\t"; break;
		default:
			const auto byte = static_cast<unsigned char>(ch);
			if (byte < 0x20) {
				out << "\\u00" << hex[byte >> 4] << hex[byte & 0x0f];
			} else {
				out.put(ch);
			}
		}
	}
	out.put('"');
}

}

#endif
