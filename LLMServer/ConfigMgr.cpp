#include "ConfigMgr.h"

ConfigMgr::ConfigMgr() {
    boost::filesystem::path current = boost::filesystem::current_path();
    boost::filesystem::path config_path = current / "config.ini";

    std::cout << "[LLMServer] config path: " << config_path << std::endl;

    boost::property_tree::ptree tree;
    boost::property_tree::read_ini(config_path.string(), tree);

    for (const auto& section_pair : tree) {
        const std::string& section_name = section_pair.first;
        const auto& section_tree = section_pair.second;

        SectionInfo section;
        for (const auto& key_value : section_tree) {
            section.section_data[key_value.first] = key_value.second.get_value<std::string>();
        }

        config_map_[section_name] = section;
    }
}

std::string ConfigMgr::GetValue(const std::string& section, const std::string& key) const {
    auto it = config_map_.find(section);
    if (it == config_map_.end()) {
        return "";
    }
    return it->second[key];
}
