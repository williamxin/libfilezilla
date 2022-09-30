#include "libfilezilla/buffer.hpp"
#include "libfilezilla/encode.hpp"
#include "libfilezilla/json.hpp"

#include "string.h"

namespace fz {
json::json(json_type t)
{
	set_type(t);
}

void json::set_type(json_type t)
{
	if (type() == t) {
		return;
	}

	switch (t) {
	case json_type::none:
		value_.emplace<std::size_t(json_type::none)>();
		break;
	case json_type::null:
		value_.emplace<std::size_t(json_type::null)>();
		break;
	case json_type::object:
		value_.emplace<std::size_t(json_type::object)>();
		break;
	case json_type::array:
		value_.emplace<std::size_t(json_type::array)>();
		break;
	case json_type::string:
		value_.emplace<std::size_t(json_type::string)>();
		break;
	case json_type::number:
		value_.emplace<std::size_t(json_type::number)>();
		break;
	case json_type::boolean:
		value_.emplace<std::size_t(json_type::boolean)>();
		break;
	}
}

bool json::check_type(json_type t)
{
	if (type() == t) {
		return true;
	}
	if (type() == json_type::none) {
		set_type(t);
		return true;
	}

	return false;
}

void json::erase(std::string const& name)
{
	if (auto *m = std::get_if<std::size_t(json_type::object)>(&value_)) {
		m->erase(name);
	}
}

json const& json::operator[](std::string const& name) const
{
	static json const nil;

	if (auto *m = std::get_if<std::size_t(json_type::object)>(&value_)) {
		auto it = m->find(name);
		if (it != m->end()) {
			return it->second;
		}
	}

	return nil;
}

json& json::operator[](std::string const& name)
{
	if (type() == json_type::none) {
		return value_.emplace<std::size_t(json_type::object)>()[name];
	}

	if (auto *m = std::get_if<std::size_t(json_type::object)>(&value_)) {
		return (*m)[name];
	}

	static thread_local json nil;
	return nil;
}

json const& json::operator[](size_t i) const
{
	static json const nil;

	if (auto *a = std::get_if<std::size_t(json_type::array)>(&value_)) {
		if (i < a->size()) {
			return (*a)[i];
		}
	}

	return nil;
}

json& json::operator[](size_t i)
{
	if (type() == json_type::none) {
		return value_.emplace<std::size_t(json_type::array)>(i + 1)[i];
	}

	if (auto *a = std::get_if<std::size_t(json_type::array)>(&value_)) {
		if (a->size() <= i) {
			a->resize(i + 1);
		}
		return (*a)[i];
	}

	static thread_local json nil;
	return nil;
}

size_t json::children() const
{
	if (auto *v = std::get_if<std::size_t(json_type::array)>(&value_)) {
		return v->size();
	}
	else
	if (auto *v = std::get_if<std::size_t(json_type::object)>(&value_)) {
		return v->size();
	}
	return 0;
}

void json::clear()
{
	value_ = {};
}

namespace {
void json_append_escaped(std::string & out, std::string const& s)
{
	for (auto & c : s) {
		switch (c) {
		case '\r':
			out += "\\r";
			break;
		case '"':
			out += "\\\"";
			break;
		case '\\':
			out += "\\\\";
			break;
		case '\n':
			out += "\\n";
			break;
		case '\t':
			out += "\\t";
			break;
		case '\b':
			out += "\\b";
			break;
		case '\f':
			out += "\\f";
			break;
		default:
			out += c;
		}
	}
}
}

std::string json::to_string(bool pretty, size_t depth) const
{	std::string ret;
	to_string(ret, pretty, depth);
	return ret;
}

void json::to_string(std::string & ret, bool pretty, size_t depth) const
{
	switch (type()) {
	case json_type::object: {
		ret += '{';
		if (pretty) {
			ret += '\n';
			ret.append(depth * 2 + 2, ' ');
		}
		bool first{true};
		for (auto const& c : *std::get_if<std::size_t(json_type::object)>(&value_)) {
			if (!c.second) {
				continue;
			}
			if (first) {
				first = false;
			}
			else {
				ret += ',';
				if (pretty) {
					ret += '\n';
					ret.append(depth * 2 + 2, ' ');
				}
			}
			ret += '"';
			json_append_escaped(ret, c.first);
			ret += "\":";
			if (pretty) {
				ret += ' ';
			}
			c.second.to_string(ret, pretty, depth + 1);
		}
		if (pretty) {
			ret += '\n';
			ret.append(depth * 2, ' ');
		}
		ret += '}';
		break;
	}
	case json_type::array: {
		ret += '[';
		if (pretty) {
			ret += '\n';
			ret.append(depth * 2 + 2, ' ');
		}
		bool first = true;
		for (auto const& c : *std::get_if<std::size_t(json_type::array)>(&value_)) {
			if (first) {
				first = false;
			}
			else {
				ret += ',';
				if (pretty) {
					ret += '\n';
					ret.append(depth * 2 + 2, ' ');
				}
			}
			if (!c) {
				ret += "null";
			}
			else {
				c.to_string(ret, pretty, depth + 1);
			}
		}
		if (pretty) {
			ret += '\n';
			ret.append(depth * 2, ' ');
		}
		ret += ']';
		break;
	}
	case json_type::boolean:
		ret += *std::get_if<std::size_t(json_type::boolean)>(&value_) ? "true" : "false";
		break;
	case json_type::number:
		ret += *std::get_if<std::size_t(json_type::number)>(&value_);
		break;
	case json_type::null:
		ret += "null";
		break;
	case json_type::string:
		ret += '"';
		json_append_escaped(ret, *std::get_if<std::size_t(json_type::string)>(&value_));
		ret += '"';
		break;
	case json_type::none:
		break;
	}
}

json json::parse(std::string_view const& s, size_t max_depth)
{
	if (s.empty()) {
		return {};
	}

	auto p = s.data();
	return parse(p, s.data() + s.size(), max_depth);
}

json json::parse(buffer const& b, size_t max_depth)
{
	return parse(b.to_view(), max_depth);
}

namespace {
void skip_ws(char const*& p, char const* end)
{
	while (p < end) {
		char c = *p;
		switch (c) {
		case ' ':
		case '\r':
		case '\n':
		case '\t':
			++p;
			break;
		default:
			return;
		}
	}
}

// Leading " has already been consumed
// Consumes trailing "
std::pair<std::string, bool> json_unescape_string(char const*& p, char const* end, bool allow_null)
{
	std::string ret;

	bool in_escape{};
	while (p < end) {
		char c = *(p++);
		if (in_escape) {
			in_escape = false;
			switch (c) {
				case '"':
					ret += '"';
					break;
				case '\\':
					ret += '\\';
					break;
				case '/':
					ret += '/';
					break;
				case 'b':
					ret += '\b';
					break;
				case 'f':
					ret += '\f';
					break;
				case 'n':
					ret += '\n';
					break;
				case 'r':
					ret += '\r';
					break;
				case 't':
					ret += '\t';
					break;
				case 'u': {
					uint32_t u{};
					if (end - p < 4) {
						return {};
					}
					for (size_t i = 0; i < 4; ++i) {
						int h = hex_char_to_int(*(p++));
						if (h == -1) {
							return {};
						}
						u <<= 4;
						u += static_cast<uint32_t>(h);
					}
					if (u >= 0xd800u && u <= 0xdbffu) {
						// High Surrogate, look for partner
						if (end - p < 6) {
							return {};
						}
						else if (*(p++) != '\\') {
							return {};
						}
						else if (*(p++) != 'u') {
							return {};
						}
						uint32_t low{};
						for (size_t i = 0; i < 4; ++i) {
							int h = hex_char_to_int(*(p++));
							if (h == -1) {
								return {};
							}
							low <<= 4;
							low += static_cast<uint32_t>(h);
						}
						if (low < 0xdc00u || low > 0xdfffu) {
							// Not a Low Surrogate
							return {};
						}
						u = (u & 0x3ffu) << 10;
						u += low & 0x3ffu;
						u += 0x10000u;
						if (u > 0x10ffffu) {
							// Too large
							return {};
						}
					}
					else if (u >= 0xdc00u && u <= 0xdfffu) {
						// Stand-alone Low Surrogate, forbidden.
						return {};
					}
					if (!u && !allow_null) {
						return {};
					}

					if (u <= 0x7f) {
						ret += static_cast<char>(u);
					}
					else if (u <= 0x7ff) {
						ret += static_cast<char>(0xc0u | ((u >> 6u) & 0x1fu));
						ret += static_cast<char>(0x80u | (u & 0x3fu));
					}
					else if (u <= 0xffff) {
						ret += static_cast<char>(0xe0u | ((u >> 12u) & 0x0fu));
						ret += static_cast<char>(0x80u | ((u >> 6u) & 0x3fu));
						ret += static_cast<char>(0x80u | (u & 0x3fu));
					}
					else {
						ret += static_cast<char>(0xf0u | ((u >> 18u) & 0x07u));
						ret += static_cast<char>(0x80u | ((u >> 12u) & 0x3fu));
						ret += static_cast<char>(0x80u | ((u >> 6u) & 0x3fu));
						ret += static_cast<char>(0x80u | (u & 0x3fu));
					}
					break;
				}
				default:
					return {};
			}
		}
		else if (c == '"') {
			return {ret, true};
		}
		else if (c == '\\') {
			in_escape = true;
		}
		else if (!c && !allow_null) {
			return {};
		}
		else {
			ret += c;
		}
	}

	return {};
}
}

json json::parse(char const*& p, char const* end, size_t max_depth)
{
	if (!max_depth) {
		return {};
	}

	skip_ws(p, end);
	if (p == end) {
		return {};
	}

	json j;
	if (*p == '"') {
		++p;
		auto [s, r] = json_unescape_string(p, end, false);
		if (!r) {
			return {};
		}

		j.value_.emplace<std::size_t(json_type::string)>(std::move(s));
	}
	else if (*p == '{') {
		++p;

		std::map<std::string, json, std::less<>> children;
		while (true) {
			skip_ws(p, end);
			if (p == end) {
				return {};
			}
			if (*p == '}') {
				++p;
				break;
			}

			if (!children.empty()) {
				if (*(p++) != ',') {
					return {};
				}
				skip_ws(p, end);
				if (p == end) {
					return {};
				}
				if (*p == '}') {
					++p;
					break;
				}
			}

			if (*(p++) != '"') {
				return {};
			}
			auto [name, r] = json_unescape_string(p, end, false);
			if (!r) {
				return {};
			}

			skip_ws(p, end);
			if (p == end || *(p++) != ':') {
				return {};
			}

			auto v = parse(p, end, max_depth - 1);
			if (!v) {
				return {};
			}
			if (!children.emplace(std::move(name), std::move(v)).second) {
				return {};
			}
		}

		j.value_ = std::move(children);
	}
	else if (*p == '[') {
		++p;

		std::vector<json> children;
		while (true) {
			skip_ws(p, end);
			if (p == end) {
				return {};
			}
			if (*p == ']') {
				++p;
				break;
			}

			if (!children.empty()) {
				if (*(p++) != ',') {
					return {};
				}
				skip_ws(p, end);
				if (p == end) {
					return {};
				}
				if (*p == ']') {
					++p;
					break;
				}
			}

			auto v = parse(p, end, max_depth - 1);
			if (!v) {
				return {};
			}
			children.emplace_back(std::move(v));
		}

		j.value_ = std::move(children);
	}
	else if ((*p >= '0' && *p <= '9') || *p == '-') {
		std::string v;
		v = *(p++);
		while (p < end && *p >= '0' && *p <= '9') {
			v += *(p++);
		}
		if (p < end && *p == '.') {
			if (v.empty() || v.back() < '0' || v.back() > '9') {
				return {};
			}
			v += *(p++);
			while (p < end && *p >= '0' && *p <= '9') {
				v += *(p++);
			}
		}
		if (p < end && (*p == 'e' || *p == 'E')) {
			if (v.empty() || v.back() < '0' || v.back() > '9') {
				return {};
			}
			v += *(p++);
			if (p < end && (*p == '+' || *p == '-')) {
				v += *(p++);
			}
			while (p < end && *p >= '0' && *p <= '9') {
				v += *(p++);
			}
		}
		if (v.empty() || v.back() < '0' || v.back() > '9') {
			return {};
		}

		j.value_.emplace<std::size_t(json_type::number)>(std::move(v));
	}
	else if (end - p >= 4 && !memcmp(p, "null", 4)) {
		j.value_ = nullptr;
		p += 4;
	}
	else if (end - p >= 4 && !memcmp(p, "true", 4)) {
		j.value_ = true;
		p += 4;
	}
	else if (end - p >= 5 && !memcmp(p, "false", 5)) {
		j.value_ = false;
		p += 5;
	}

	return j;
}

json& json::operator=(std::string_view const& v)
{
	// see comment in operator=(json const&)
	value_.emplace<std::size_t(json_type::string)>(v);
	return *this;
}

namespace {
char get_radix() {
	static char const radix = []{
		// Hackish to the max. Sadly nl_langinfo isn't available everywhere
		char buf[20];
		double d = 0.1;
		snprintf(buf, 19, "%f", d);
		char* p = buf;
		while (*p) {
			if (*p < '0' || *p > '9') {
				return *p;
			}
			++p;
		}
		return '.';
	}();
	return radix;
}
}

double json::number_value_double() const
{
	std::string const *s{};

	if (auto *v = std::get_if<std::size_t(json_type::number)>(&value_)) {
		s = v;
	}
	else if (auto *v = std::get_if<std::size_t(json_type::string)>(&value_)) {
		s = v;
	}
	else {
		return {};
	}

	std::string v = *s;

	size_t pos = v.find('.');
	if (pos != std::string::npos) {
		v[pos] = get_radix();
	}

	char* res{};
	double d = strtod(v.c_str(), &res);
	if (res && *res != 0) {
		return {};
	}

	return d;
}

uint64_t json::number_value_integer() const
{
	std::string const *s{};

	if (auto *v = std::get_if<std::size_t(json_type::number)>(&value_)) {
		s = v;
	}
	else if (auto *v = std::get_if<std::size_t(json_type::string)>(&value_)) {
		s = v;
	}
	else {
		return {};
	}


	std::string const& v = *s;
	bool floating{};

	size_t i = 0;
	if (!v.empty() && v[0] == '-') {
		++i;
	}
	for (; i < v.size(); ++i) {
		if (v[i] < '0' || v[i] > '9') {
			floating = true;
		}
	}

	// Only go through floating point if needed, so that large 64bit integers stay exact
	if (floating) {
		return static_cast<uint64_t>(number_value_double());
	}
	else {
		return to_integral<uint64_t>(v);
	}
}

bool json::bool_value() const
{
	if (auto *v = std::get_if<std::size_t(json_type::boolean)>(&value_)) {
		return *v;
	}
	if (auto *v = std::get_if<std::size_t(json_type::string)>(&value_)) {
		return *v == "true";
	}
	return false;
}

std::string json::string_value() const
{
	if (auto *v = std::get_if<std::size_t(json_type::string)>(&value_)) {
		return *v;
	}
	if (auto *v = std::get_if<std::size_t(json_type::number)>(&value_)) {
		return *v;
	}
	if (auto *v = std::get_if<std::size_t(json_type::boolean)>(&value_)) {
		return *v ? "true" : "false";
	}
	return {};
}

json& json::operator=(json const& j)
{
	if (&j != this) {
		// First make a copy, then destroy own value, as the argument may depend on our value:
		// fz::json j;
		// j["child"] = 1;
		// fz::json const& ref = j;
		// j = ref["child"];
		auto v = j.value_;
		value_ = std::move(v);
	}
	return *this;
}

json& json::operator=(json && j) noexcept
{
	if (&j != this) {
		auto v = std::move(j.value_);
		value_ = std::move(v);
	}
	return *this;
}

}
