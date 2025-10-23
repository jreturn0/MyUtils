#pragma once
#include "ConfigFile.h"




namespace utl {

    class ConfigSystem {
    public:
        static ConfigSystem& instance() {
            static ConfigSystem instance;
            return instance;
        }
        ConfigSystem(const ConfigSystem&) = delete;
        ConfigSystem& operator=(const ConfigSystem&) = delete;
        ConfigSystem(ConfigSystem&&) = delete;
        ConfigSystem& operator=(ConfigSystem&&) = delete;

        ConfigFile& createFile(std::string_view name) {
            auto it = m_configFiles.find(StringHash(name));
            if (it != m_configFiles.end()) {
                return *(it->second);
            }
            // Create new ConfigFile
            auto cfg = std::make_shared<ConfigFile>(name);
            m_configFiles[StringHash(name)] = cfg;
            return *cfg;
        }

        bool hasFile(StringHash name) const noexcept {
            return m_configFiles.contains(name);
        }

        ConfigFile* getFile(StringHash name) noexcept {
            auto it = m_configFiles.find(name);
            return it != m_configFiles.end() ? it->second.get() : nullptr;
        }


        bool getFile(StringHash name, ConfigFile*& out) {
            auto it = m_configFiles.find(name);
            if (it != m_configFiles.end()) {
                out = it->second.get();
                return true;
            }
            out = nullptr;
            return false;
        }

        std::weak_ptr<ConfigFile> getFileWeak(StringHash name) noexcept {
            auto it = m_configFiles.find(name);
            return it != m_configFiles.end() ? std::weak_ptr<ConfigFile>(it->second) : std::weak_ptr<ConfigFile>{};
        }

        std::shared_ptr<ConfigFile> getFileShared(StringHash name) noexcept {
            auto it = m_configFiles.find(name);
            return it != m_configFiles.end() ? it->second : nullptr;
        }

        std::weak_ptr<ConfigFile> getGlobalConfigFileWeak() noexcept {
            return getFileWeak(StringHash(m_globalConfigFileName));
        }

        ConfigFile& getGlobalConfigFile() {
            return createFile(m_globalConfigFileName);
        }



        std::string_view getGlobalConfigFileName() const noexcept { return m_globalConfigFileName; }




    private:


        ConfigSystem() {
            createFile(m_globalConfigFileName);
            getGlobalConfigFile().load();
        }
        ~ConfigSystem() {
            getGlobalConfigFile().save();
        }


        std::string m_globalConfigFileName = "globalcfg.ini";
        std::unordered_map<uint64_t, std::shared_ptr<ConfigFile>> m_configFiles;
    };

    template<details::ConfigValuable T>
    class ConfigValueRef {
    public:
        using ValType = std::conditional_t< std::is_same_v<T, bool>, bool,
            std::conditional_t<  std::is_integral_v<T>, int64_t,
            std::conditional_t< std::is_floating_point_v<T>, double, std::string
            >>>;
        ConfigValueRef(std::string_view name, const T& defaultValue, ConfigFlags flags = details::g_defaultConfigFlags) :
            filePtr(ConfigSystem::instance().getGlobalConfigFileWeak()), index(0)
        {
            if (auto sp = filePtr.lock()) {
                index = sp->createValue<ValType>(name, defaultValue, flags);
            }
            else {
                throw std::runtime_error("ConfigValueRef: Failed to get global config file");
            }
        }

        const ValType& get() const {
            if (auto sp = filePtr.lock()) {
                if (auto ptr = sp->getValuePtr<ValType>(index)) {
                    return *ptr;
                }
            }
            throw std::runtime_error("ConfigValueRef: Failed to get value, invalid reference");

        }
        ValType getCopy() const {
            if (auto sp = filePtr.lock()) {
                if (auto ptr = sp->getValuePtr<ValType>(index)) {
                    return *ptr;
                }
            }
            throw std::runtime_error("ConfigValueRef: Failed to get value, invalid reference");
        }

        ValType* getPtr() const {
            if (auto sp = filePtr.lock()) {
                return sp->getValuePtr<ValType>(index);
            }
            return nullptr;
        }

        bool tryGet(ValType& out) const {
            if (auto sp = filePtr.lock()) {
                return sp->getValue<ValType>(index, out);
            }
            return false;
        }

        std::string toString() const {
            if (auto sp = filePtr.lock()) {
                ConfigValue val;
                if (sp->getValue(index, val)) { // fixed: use stored index instead of invalid hash
                    return details::toString(val); // generic string conversion
                }
            }
            return "<invalid>";
        }

        bool set(const ValType& v) {
            if (auto sp = filePtr.lock()) {
                if (sp->setValue<ValType>(index, v)) {
                    return true;
                }
            }
            return false;
        }
    private:
        friend class ConfigSystem;
        std::weak_ptr<ConfigFile> filePtr;
        size_t index{ 0 };
    };

} // namespace utl




