
#include "FSNavigator.h"

#include <fstream>

namespace fs = std::filesystem;

FSNavigator::FSNavigator(std::filesystem::path path) : currentPath(std::move(path))
{
	update();
}

void FSNavigator::update()
{
	fs::path fullPath = currentPath;
	if (!currentPath.is_absolute()) {
		fullPath = fs::current_path() / currentPath;
	}

	if (!fs::exists(fullPath) || !fs::is_directory(fullPath)) {
		throw std::runtime_error("Invalid directory path: " + fullPath.string());
	}
	files.clear();
	folders.clear();

	for (auto&& entry : fs::directory_iterator(currentPath)) {
		if (entry.is_directory()) {
			folders.insert(entry.path().filename());
		} else {
			files.insert(entry.path().filename());
		}
	}
}

bool FSNavigator::goToParent()
{
	if (currentPath.has_parent_path()) {
		currentPath = currentPath.parent_path();
		update();
		return true;
	}
	return false;
}
bool FSNavigator::goToFolder(const std::filesystem::path& folder)
{
	if (containsFolder(folder)) {
		currentPath /= folder;
		update();
		return true;
	};
	return false;
}

bool FSNavigator::goToPath(const std::filesystem::path& path)
{
	if (std::filesystem::is_directory(path)) {
		currentPath = path;
		update();
		return true;
	}
	return false;
}




bool FSNavigator::setSelectedFile(const std::filesystem::path& file)
{
	if (files.contains(file)) {
		selectedFile = currentPath / file.filename();
		return true;
	}
	selectedFile.reset();
	return false;
}


