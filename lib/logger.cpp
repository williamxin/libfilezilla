#include "libfilezilla/logger.hpp"
#include "libfilezilla/time.hpp"
#include "libfilezilla/util.hpp"

#include <iostream>

namespace fz {

null_logger& get_null_logger()
{
	static null_logger log;
	return log;
}

void stdout_logger::do_log(logmsg::type t, std::wstring && msg)
{
	auto now = fz::datetime::now();
	std::cout << now.format("%Y-%m-%dT%H:%M:%S.", fz::datetime::utc) << fz::sprintf("%03d", now.get_milliseconds()) << "Z " << (1 + bitscan(static_cast<uint64_t>(t))) << " " << fz::to_string(msg) << std::endl;
}
}

