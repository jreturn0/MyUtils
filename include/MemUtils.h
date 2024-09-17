#pragma once


//bytes to kilobytes
constexpr size_t B_TO_KB = 1024ll;
//bytes to megabytes
constexpr size_t B_TO_MB = 1024ll * 1024;
//bytes to gigabytes
constexpr size_t B_TO_GB = 1024ll * 1024 * 1024;

constexpr size_t KB_TO_B = 1 / B_TO_KB;

constexpr size_t MB_TO_B = 1 / B_TO_MB;

constexpr size_t GB_TO_B = 1 / B_TO_GB;



//functions to convert bytes to kilobytes, megabytes, and gigabytes
constexpr size_t BytesToKb(const size_t bytes) { return bytes / B_TO_KB; }
constexpr size_t BytesToMb(const size_t bytes) { return bytes / B_TO_MB; }
constexpr size_t BytesToGb(const size_t bytes) { return bytes / B_TO_GB; }

//functions to convert kilobytes to bytes, megabytes, and gigabytes
constexpr size_t KbToBytes(const size_t kb) { return kb * B_TO_KB; }
constexpr size_t KbToMb(const size_t kb) { return kb / B_TO_KB; }
constexpr size_t KbToGb(const size_t kb) { return kb / B_TO_MB; }

//functions to convert megabytes to bytes, kilobytes, and gigabytes
constexpr size_t MbToBytes(const size_t mb) { return mb * B_TO_MB; }
constexpr size_t MbToKb(const size_t mb) { return mb * B_TO_KB; }
constexpr size_t MbToGb(const size_t mb) { return mb / B_TO_MB; }

//functions to convert gigabytes to bytes, kilobytes, and megabytes
constexpr size_t GbToBytes(const size_t gb) { return gb * B_TO_GB; }
constexpr size_t GbToKb(const size_t gb) { return gb * B_TO_MB; }
constexpr size_t GbToMb(const size_t gb) { return gb * B_TO_KB; }

