#pragma once

#include <string>
#include <vector>
#include <optional>
#include <filesystem>
#include <set>
#include <stdexcept>

//namespace std::filesystem = std::filesystem;


class FSNavigator
{
private:

	std::filesystem::path currentPath;
	std::set<std::filesystem::path> files;
	std::set<std::filesystem::path> folders;

	std::optional<std::filesystem::path> selectedFile;

public:
	explicit FSNavigator(std::filesystem::path path = std::filesystem::current_path().string());

	void update();

	[[nodiscard]] const std::set<std::filesystem::path>& getFiles() const { return files; }
	[[nodiscard]] const std::set<std::filesystem::path>& getDirectories() const { return folders; }
	[[nodiscard]] std::filesystem::path getCurrentPath() const { return currentPath; }
	[[nodiscard]] std::filesystem::path getFilePath(const std::filesystem::path& file) const { return files.contains(file) ? currentPath / file : ""; }
	[[nodiscard]] std::filesystem::path getFolderPath(const std::filesystem::path& folder) const { return folders.contains(folder) ? currentPath / folder : ""; }
	[[nodiscard]] const std::optional<std::filesystem::path>& getSelectedFile() const { return selectedFile; }

	bool setSelectedFile(const std::filesystem::path& file);

	[[nodiscard]] bool containsFile(const std::filesystem::path& file) const { return files.contains(file) || files.contains(file.filename()); }
	[[nodiscard]] bool containsFolder(const std::filesystem::path& folder) const { return folders.contains(folder) || folders.contains(folder.filename()); }
	
	bool goToParent();
	bool goToFolder(const std::filesystem::path& folder);
	bool goToPath(const std::filesystem::path& path);







};
