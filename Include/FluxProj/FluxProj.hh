#ifndef FLUXPROJ_HH
#define FLUXPROJ_HH
#include <string>
#include <filesystem>
#include <unordered_map>
#include <vector>
#include <map>

#define FLUXPROJ_VERSION 1

namespace FluxProj
{
    struct ProjPath
    {
        std::filesystem::path input;
        std::filesystem::path output;
        bool active = false;
    };

    extern std::unordered_map<std::string, bool(*)(std::filesystem::path, std::filesystem::path)> extensions;

    /* Register a handler for loading a file type. Takes the extension, including the . */
    inline bool addExtension(std::string ext, bool(*function)(std::filesystem::path, std::filesystem::path))
    {
        extensions[ext] = function;
        return true;
    }

    struct CacheEntry
    {
        uint64_t last_modified;
    };

    class Project
    {
    public:
        Project(std::string file, bool force_new = false);

        // For placeholder project
        Project();

        /** Add a file to the project. It will be automatically loaded if there's an apropriate loader */
        ProjPath addFile(std::string filename);

        /** Remove a file from the project. Takes input filename. Note: This does not remove any actual files */
        void removeFile(std::string filename);

        /** Imports new assets, and recreates the project file.
        If clean is true, it will re-import every asset.
        If release is true, it will build them in release mode */
        void runBuild(bool clean = false, bool release = false);

        /** Get a vector of paths in this project. Thay are all relative to the base_dir */
        std::vector<ProjPath> getPaths() const
        {
            return paths;
        }

        /** Get a path to the project folder */
        std::filesystem::path getFolder() const
        {
            return base_dir;
        }

        static std::map<std::string, int> thing;
    private:
        std::filesystem::path base_dir;
        std::filesystem::path project_file;

        std::vector<ProjPath> paths;
    };
}

#endif