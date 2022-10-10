#pragma once

#include <fstream>
#include <stdarg.h>

#include <chrono>
#include <ctime>
#include <iomanip>

#define WDBGVAR( var ) { std::ostringstream ods_stream_; ods_stream_ << "(" << __LINE__ << ") " << #var << " = [" << (var) << "]"; ::OutputDebugStringA(ods_stream_.str().c_str()); }

/*
	Instantiation:
		
		std::ofstream Log::LOG("mylogfile.log");

			or

		std::ofstream Log::LOG;
		..
		Log().open("mylogfile.log");

	Usage:
		Log() << "Log entry";


	TODO:
		- Output to OutputDebugString(A/W)
		- Include optional timestamps
*/
class Log
{
public:
	
	Log() {}
	~Log()
	{
		if (LOG.is_open())
		{
			LOG << std::endl;
		}
	}

	template <typename T>
	Log& operator<<(const T& t)
	{
		if (LOG.is_open())
		{
			LOG << t;
		}
		return *this;
	}

	Log& open(std::string filename)
	{
		if (!LOG.is_open())
		{
			LOG.open(filename);
		}
		return *this;
	}

private:
	static std::ofstream LOG;
};
// std::chrono::format("%F_%T", now)
static std::ostream& operator<<(std::ostream& os, const wchar_t* wchr)
{
	/*
		auto now = std::chrono::system_clock::now();
		auto in_time_t = std::chrono::system_clock::to_time_t(now);
		auto time = std::put_time(std::localtime(&in_time_t), "%Y-%m-%d %X");
		os << time << " ";
	*/
	std::wstring ws(wchr);
	return os << std::string(ws.begin(), ws.end()).c_str();
}

static void logf(char* fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	auto size = vsnprintf(nullptr, 0, fmt, ap);
	std::string output(size + 1, '\0');
	vsprintf_s(&output[0], size + 1, fmt, ap);
	Log() << output.c_str();
	va_end(ap);
}

static void logf(wchar_t* fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
#pragma warning(suppress: 4996)
	auto size = _vsnwprintf(nullptr, 0, fmt, ap);
	std::wstring output(size + 1, '\0');
	vswprintf_s(&output[0], size + 1, fmt, ap);
	Log() << output.c_str();
	va_end(ap);
}

/*
static char* ByteArrStr(BYTE* byteArr, SIZE_T sz) {
	char buffer[(5 * 1024) + 1];

	for (unsigned int j = 0; j < sz; j++)
		sprintf(&buffer[3 * j], "%02X ", (unsigned char)byteArr[j]);

	return buffer;
}
*/