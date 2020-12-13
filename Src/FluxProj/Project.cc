#include "Flux/Log.hh"
#include "FluxProj/FluxProj.hh"
#include <filesystem>
#include <nlohmann/json.hpp>
#include <fstream>
#include <stdexcept>
#include <string>
#include <chrono>
#include <iomanip>
#include <FluxArc/FluxArc.hh>

using namespace FluxProj;
using json = nlohmann::json;

// std::map<std::string, int> Project::thing = std::map<std::string, int>();
std::unordered_map<std::string, bool(*)(std::filesystem::path, std::filesystem::path)> FluxProj::extensions = std::unordered_map<std::string, bool(*)(std::filesystem::path, std::filesystem::path)>();

Project::Project()
{
    // Don't call functions on a project like this,
    // Everything will die
}

Project::Project(std::string filename, bool force_new)
{
    // Parse project file
    std::ifstream file(filename);

    project_file = std::filesystem::path(filename);
    base_dir = std::filesystem::path(filename).parent_path();

    if (!file.good() || force_new)
    {
        // File doesn't exist; create
        json basic_file = {
            {"version", FLUXPROJ_VERSION}
        };

        basic_file["files"] = json::array();

        std::ofstream new_file(filename);
        new_file << basic_file.dump(4);
        new_file.close();

        return;
    }

    std::string content = "";
    std::string tp;
    while(getline(file, tp)){  //read data from file object and put it into string.
        content += tp + "\n";
    }
    file.close();   //close the file object.

    // Parse
    json basic_file = json::parse(content);
    if (basic_file["version"] != FLUXPROJ_VERSION)
    {
        // TODO: Something bad
    }

    paths = std::vector<ProjPath>();
    for (auto i: basic_file["files"])
    {
        ProjPath p;
        p.input = i["input"].get<std::string>();
        p.output = i["output"].get<std::string>();
        p.active = true;
        paths.push_back(p);
    }
}

ProjPath Project::addFile(std::string filename)
{
    auto path = std::filesystem::path(filename);

    // I LOVE the file system library!
    auto relative_path = std::filesystem::relative(path, base_dir);

    // Make sure it doesn't already exist
    for (auto i : paths)
    {
        if (i.input == relative_path)
        {
            // Rebuild and exit
            runBuild();
            return ProjPath();
        }
    }

    ProjPath path_container;

    path_container.input = relative_path;
    path_container.output = relative_path.replace_extension(".farc");
    path_container.active = true;

    paths.push_back(path_container);

    runBuild();

    return path_container;
}

void Project::removeFile(std::string filename)
{
    int index = -1;
    std::filesystem::path fname(filename);
    for (int i = 0; i < paths.size(); i++)
    {
        if (paths[i].input == filename)
        {
            index = i;
            break;
        }
    }

    if (index == -1)
    {
        LOG_ERROR("... what just happened ...");
        return;
    }

    paths.erase(paths.begin() + index);

    runBuild();
}

void Project::runBuild(bool clean, bool release)
{
    LOG_INFO("Starting build:");

    // Check FluxCache
    FluxArc::Archive arc(base_dir / ".fluxproj_cache");

    if (arc.hasFile("--properties--"))
    {
        // Make sure it's the right version
        uint32_t version;
        auto file = arc.getBinaryFile("--properties--");
        if (!file.get(&version))
        {
            LOG_ERROR("Properties file is broken: Doing clean rebuild");
        }
        
        if (version != FLUXPROJ_VERSION)
        {
            clean = true;
            LOG_INFO("Cache out of date: Doing clean rebuild");

            // Set cache
            FluxArc::BinaryFile file;
            uint32_t vers = FLUXPROJ_VERSION;
            file.set(vers);

            arc.setFile("--properties--", file);
        }
    }
    else
    {
        LOG_INFO("Cache missing or broken: Doing clean rebuild");

        // Create cache
        FluxArc::BinaryFile file;
        uint32_t vers = FLUXPROJ_VERSION;
        file.set(vers);

        arc.setFile("--properties--", file);
    }

    for (auto i : paths)
    {
        bool rebuild = clean;
        if (arc.hasFile(i.input))
        {
            // Get cached last modified time
            CacheEntry c;
            auto bf = arc.getBinaryFile(i.input);
            bf.get(&c);

            // Get actual last modified time
            auto last_time = std::filesystem::last_write_time(base_dir / i.input);

            if (last_time.time_since_epoch().count() != c.last_modified)
            {
                rebuild = true;
            }
        }
        else
        {
            rebuild = true;
        }

        if (rebuild == true)
        {
            LOG_INFO("Building " + i.input.string() + " to " + i.output.string());
            if (extensions.find(i.input.extension()) == extensions.end())
            {
                LOG_ERROR("Invalid file type: " + i.input.extension().string());
                continue;
            }

            bool result = extensions[i.input.extension()](base_dir / i.input, base_dir / i.output);
            
            if (result)
            {
                LOG_SUCCESS("Built " + i.input.string() + " successfully");

                // Add to cache
                auto last_time = std::filesystem::last_write_time(base_dir / i.input);

                FluxArc::BinaryFile file;
                CacheEntry cache;
                cache.last_modified = last_time.time_since_epoch().count();
                file.set(cache);

                arc.setFile(i.input, file);
            }
            else
            {
                LOG_ERROR("File failed to build");
            }
        }
    }

    // Rebuild project file
    std::vector<json> paths_as_json;
    for (auto p : paths)
    {
        json j = {
            {"input", p.input.string()},
            {"output", p.output.string()}
        };
        paths_as_json.push_back(j);
    }

    // Make JSON
    json j;
    j["version"] = FLUXPROJ_VERSION;
    j["files"] = paths_as_json;

    std::ofstream new_file(project_file.string());
    new_file << j.dump(4);
    new_file.close();
}