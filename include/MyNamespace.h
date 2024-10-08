#pragma once
#define UTIL_NAMESPACE Utl

namespace Utl{}
#define 

#ifdef UTIL_NAMESPACE 
#define MY_UTILS_NAMESPACE_BEGIN namespace UTIL_NAMESPACE  {
#define MY_UTILS_NAMESPACE_END }
#else
#define MY_UTILS_NAMESPACE_BEGIN
#define MY_UTILS_NAMESPACE_END
#endif


MY_UTILS_NAMESPACE_BEGIN

namespace ansi
{
	constexpr const char* RED = "\033[31m";
}

class FSNavigator
{
	
};

class FSReader
{
public:
	static void ReadFile(const char* path)
	{

	}
};

class RNG
{
public:
	static RNG &Get()
	{
		static RNG rng;
		return rng;
	}
};

inline constexpr int Fnv1A32(const char* str) { return 151432; }


MY_UTILS_NAMESPACE_END

#define TEST_NAMESPACE UTIL_NAMESPACE::RNG::Get(); \
UTIL_NAMESPACE::FSNavigator fs; \
auto hash=UTIL_NAMESPACE::Fnv1A32("Hello"); \
auto red = UTIL_NAMESPACE::ansi::RED; \
UTIL_NAMESPACE::FSReader::ReadFile("Hello.txt"); 

void Test()
{
	//myutl
	myutl::RNG::Get();
	myutl::FSNavigator fs;
	auto hash = myutl::Fnv1A32("Hello");
	auto red = myutl::ansi::RED;
	myutl::FSReader::ReadFile("Hello.txt");




	//shutl
	shutl::RNG::Get();
	shutl::FSNavigator fs;
	auto hash = shutl::Fnv1A32("Hello");
	auto red = shutl::ansi::RED;
	shutl::FSReader::ReadFile("Hello.txt");

	//EMUtils
	Emu::RNG::Get();
	Emu::FSNavigator fs;
	auto hash = Emu::Fnv1A32("Hello");
	auto red = Emu::ansi::RED;
	Emu::FSReader::ReadFile("Hello.txt");

	//EMU
	EMU::RNG::Get();
	EMU::FSNavigator fs;
	auto hash = EMU::Fnv1A32("Hello");
	auto red = EMU::ansi::RED;
	EMU::FSReader::ReadFile("Hello.txt");

	//MU
	Mu::RNG::Get();
	Mu::FSNavigator fs;
	auto hash = Mu::Fnv1A32("Hello");
	auto red = Mu::ansi::RED;
	Mu::FSReader::ReadFile("Hello.txt");

	//EMUtls
	EMUtls::RNG::Get();
	EMUtls::FSNavigator fs;
	auto hash = EMUtls::Fnv1A32("Hello");
	auto red = EMUtls::ansi::RED;
	EMUtls::FSReader::ReadFile("Hello.txt");

	//EMUtl
	EMUtl::RNG::Get();
	EMUtl::FSNavigator fs;
	auto hash = EMUtl::Fnv1A32("Hello");
	auto red = EMUtl::ansi::RED;
	EMUtl::FSReader::ReadFile("Hello.txt");


	//Imp
	Imp::RNG::Get();
	Imp::FSNavigator fs;
	auto hash = Imp::Fnv1A32("Hello");
	auto red = Imp::ansi::RED;
	Imp::FSReader::ReadFile("Hello.txt");


}
