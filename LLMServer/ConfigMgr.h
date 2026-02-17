#pragma once

#include <boost/filesystem.hpp>
#include <boost/property_tree/ini_parser.hpp>
#include <boost/property_tree/ptree.hpp>

#include <iostream>
#include <map>
#include <string>

struct SectionInfo {
    std::map<std::string, std::string> section_data;

    std::string operator[](const std::string& key) const {
        auto it = section_data.find(key);
        if (it == section_data.end()) {
            return "";
        }
        return it->second;
    }
};

class ConfigMgr {
public:
    static ConfigMgr& Inst() {
        static ConfigMgr instance;
        return instance;
    }

    SectionInfo operator[](const std::string& section) const {
        auto it = config_map_.find(section);
        if (it == config_map_.end()) {
            return SectionInfo{};
        }
        return it->second;
    }

    std::string GetValue(const std::string& section, const std::string& key) const;

private:
    ConfigMgr();
    std::map<std::string, SectionInfo> config_map_;
};
