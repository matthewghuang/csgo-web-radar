#include "process.h"

#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>
#include <dirent.h>

bool process::find_process_by_name(std::string target_process, proc_handle *target) {
    if (target_process.empty()) // must have a target
        return false;

    DIR *proc_dir = opendir("/proc/");
    struct dirent *dir_ent;

    if (proc_dir == nullptr)
        return false;

    while ((dir_ent = readdir(proc_dir))) {
        if (dir_ent == nullptr)
            return false;

        std::string pid = std::string(dir_ent->d_name);
        unsigned char type = dir_ent->d_type;

        if (type != DT_DIR)
            continue;

        std::string map_path = "/proc/" + pid + "/maps";

        struct stat info;

        if (stat(map_path.c_str(), &info) != 0) // make sure we can access memory
            continue;

        process::proc_handle handle(stoi(pid)); // temporary for validation

        std::string exe = handle.get_executable();

        if (!exe.compare(target_process)) { // found process
            *target = handle;

            return true;
        }
    }

    return false;
}

process::proc_handle::proc_handle(int target) {
    this->pid = target;
}

std::string process::proc_handle::get_executable() {
    std::ifstream istream("/proc/" + std::to_string(pid) + "/comm");

    std::string line; // returns the first lline of file
    std::getline(istream, line);

    return line;
}

bool process::proc_handle::read(unsigned long addr, void *buffer, size_t size) {
    struct iovec local[1];
    struct iovec remote[1];

    local[0].iov_base = buffer;
    local[0].iov_len = size;

    remote[0].iov_base = (void *) addr;
    remote[0].iov_len = size;

    ssize_t bytes_read = process_vm_readv(this->pid, local, 1, remote, 1, 0);

    return bytes_read == size;
}

unsigned long process::proc_handle::get_module_addr(std::string module_name) {
    std::ifstream istream("/proc/" + std::to_string(this->pid) + "/maps");

    std::string line;
    while (std::getline(istream, line)) {
        if (line.find(module_name) != std::string::npos) {
            size_t first_separator = line.find_first_of("-");

            std::string addr = line.substr(0, first_separator);

            return strtoul(addr.c_str(), nullptr, 16);
        }
    }

    return 0;
}
