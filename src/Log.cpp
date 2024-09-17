#include "Log.h"

Debug::Log::Log(const char* fmt, const Log::Type t, const std::source_location& l)
	: type(t), message(fmt), source(l)
{}

Debug::Log::Log(std::string fmt, const Type t, const std::source_location& l) 
	: type(t), message(std::move(fmt)), source(l)
{}
