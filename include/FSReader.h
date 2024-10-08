#pragma once
#include <filesystem>
#include <fstream>
#include <vector>
template<typename T, typename U>
constexpr bool IS_DECAY_SAME = std::is_same_v<std::decay_t< T>, std::decay_t< U>>;

class FSReader
{
public:





    template <typename T = uint8_t>
    static auto ReadFileContents(const std::filesystem::path& path)
    {
        // Check if the file exists and is a regular file
        if (!std::filesystem::exists(path) || !std::filesystem::is_regular_file(path)) {
            throw std::runtime_error("Invalid file path: " + path.string());
        }

        // Open the file in binary mode, starting at the end to get the file size
        std::ifstream file(path, std::ios::ate | std::ios::binary);
        if (!file.is_open()) {
            throw std::runtime_error("Failed to open file: " + path.string());
        }

        const std::streamsize fileSize = file.tellg(); // Get the size of the file
        file.seekg(0, std::ios::beg); // Move back to the beginning of the file

        // Declare the buffer based on the type T
        if constexpr (std::is_arithmetic_v<T>) {
            using BufferType = std::conditional_t<
                IS_DECAY_SAME<T, uint8_t>, uint8_t,
                std::conditional_t<
                IS_DECAY_SAME<T, uint16_t>, uint16_t,
                std::conditional_t<
                IS_DECAY_SAME<T, uint32_t>, uint32_t,
                std::conditional_t<IS_DECAY_SAME<T, uint64_t>, uint64_t, void>>>>;

            if constexpr (!std::is_void_v<BufferType>) {
                std::vector<BufferType> buffer(fileSize / sizeof(BufferType));
                file.read(reinterpret_cast<char*>(buffer.data()), fileSize); // Single read operation
                file.close();
                return buffer;
            }
        } else if constexpr (IS_DECAY_SAME<T, std::string>) {
            std::string buffer(fileSize, '\0');
            file.read(buffer.data(), fileSize); // Single read for strings
            file.close();
            return buffer;
        } else {
            throw std::runtime_error("Unsupported type for reading file contents.");
        }
    }




	static std::string ReadTextFile(const std::filesystem::path& path);
	static std::vector<uint8_t> ReadBinaryFile(const std::filesystem::path& path);
	static std::vector<uint16_t> ReadBinaryFile16(const std::filesystem::path& path);
	static std::vector<uint32_t> ReadBinaryFile32(const std::filesystem::path& path);
};
