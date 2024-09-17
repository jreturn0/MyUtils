#include "FSReader.h"

#include <fstream>

namespace fs = std::filesystem;

std::string FSReader::ReadTextFile(const std::filesystem::path& path)
{
	if (!fs::exists(path) || !fs::is_regular_file(path)) {
		throw std::runtime_error("Invalid file path" + path.string());
	}

	std::ifstream file(path);
	if (!file.is_open()) {
		throw std::runtime_error("Failed to open file: " + path.string());
	}

	const std::streamsize fileSize = file.tellg();
	std::string content(fileSize, '\0');

	// Read the entire file content into the string
	file.seekg(0, std::ios::beg); // Move back to the start
	if (!file.read(content.data(), fileSize)) {
		throw std::runtime_error("Failed to read the file completely: " + path.string());
	}
	file.close();
	return content;
}

std::vector<uint8_t> FSReader::ReadBinaryFile(const std::filesystem::path& path)
{

	if (!fs::exists(path) || !fs::is_regular_file(path)) {
		throw std::runtime_error("Invalid file path: " + path.string());
	}
	std::ifstream file(path, std::ios::ate | std::ios::binary);
	if (!file.is_open()) {
		throw std::runtime_error("Failed to open file: " + path.string());
	}
	const std::streamsize fileSize = file.tellg();
	std::vector<uint8_t> buffer(fileSize);

	file.seekg(0, std::ios::beg);
	if (!file.read(reinterpret_cast<char*>(buffer.data()), fileSize)) {
		throw std::runtime_error("Failed to read the file completely: " + path.string());
	}
	file.close();
	return buffer;

}

std::vector<uint16_t> FSReader::ReadBinaryFile16(const std::filesystem::path& path)
{
	if (!fs::exists(path) || !fs::is_regular_file(path)) {
		throw std::runtime_error("Invalid file path: " + path.string());
	}
	std::ifstream file(path, std::ios::ate | std::ios::binary);
	if (!file.is_open()) {
		throw std::runtime_error("Failed to open file: " + path.string());
	}
	const std::streamsize fileSize = file.tellg();
	std::vector<uint16_t> buffer(fileSize);

	file.seekg(0, std::ios::beg);
	if (!file.read(reinterpret_cast<char*>(buffer.data()), fileSize)) {
		throw std::runtime_error("Failed to read the file completely: " + path.string());
	}
	file.close();
	return buffer;

}

std::vector<uint32_t> FSReader::ReadBinaryFile32(const std::filesystem::path& path)
{
	if (!fs::exists(path) || !fs::is_regular_file(path)) {
		throw std::runtime_error("Invalid file path: " + path.string());
	}
	std::ifstream file(path, std::ios::ate | std::ios::binary);
	if (!file.is_open()) {
		throw std::runtime_error("Failed to open file: " + path.string());
	}
	const std::streamsize fileSize = file.tellg();
	std::vector<uint32_t> buffer(fileSize);

	file.seekg(0, std::ios::beg);
	if (!file.read(reinterpret_cast<char*>(buffer.data()), fileSize)) {
		throw std::runtime_error("Failed to read the file completely: " + path.string());
	}
	file.close();
	return buffer;
}
