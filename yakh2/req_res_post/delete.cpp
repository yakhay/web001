#include "delete.hpp"
#include <algorithm>

void    delete_path(std::string cpath)
{
    struct stat     path_info;
    struct dirent   *entry;
    DIR             *dir;

    if (stat(cpath.c_str(), &path_info) != 0)
        throw 404;
    if (S_ISDIR(path_info.st_mode))
    {
        if (path_info.st_mode & S_IXUSR)//permission to delete
        {
            dir = opendir(cpath.c_str());
            if (!dir)
                throw (403);
            while ((entry = readdir(dir)) != nullptr)
            {
                if (std::strcmp(entry->d_name, ".") && std::strcmp(entry->d_name, ".."))
                    delete_path(cpath + "/" + entry->d_name);
            }
            closedir(dir);
            rmdir(cpath.c_str());
        }
        else
            throw (403);
    }
    else if (S_ISREG(path_info.st_mode))
    {
        if (path_info.st_mode & S_IWUSR)//permission to delete
            remove(cpath.c_str());
        else
            throw (403);
    }
}

void    delete_request(const one_server& server, const request& req)
{
    Location    location;
    std::string url;
    std::string cpath;

    url = req.get_path();
    location = server.get_location(url.substr(0, url.find("?")));
    if (find(location._allow_methods.cbegin(), location._allow_methods.cend(), "DELETE") == location._allow_methods.cend())
        throw (405);
    cpath = server.get_path(url.substr(0, url.find("?")));
    delete_path(cpath);
    throw (204);
}