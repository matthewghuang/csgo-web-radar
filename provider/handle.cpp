#include "handle.h"

#include <iostream>
#include <sys/uio.h>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <linux/limits.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>

proc_handle::proc_handle(std::string target) {
    if (std::all_of(target.begin(), target.end(), isdigit)) {
        std::istringstream iss(target);
        iss >> pid;

        pid_str = target;
    } else {
        pid = -1;

        pid_str.clear();
    }
}

std::string proc_handle::get_executable() {
    std::ifstream istream("/proc/" + pid_str + "/comm");

    std::string line;
    std::getline(istream, line);

    return line;
}

bool proc_handle::read(unsigned long addr, void *buffer, size_t size) {
    struct iovec local[1];
    struct iovec remote[1];

    local[0].iov_base = buffer;
    local[0].iov_len = size;

    remote[0].iov_base = (void *) addr;
    remote[0].iov_len = size;

    ssize_t bytes_read = process_vm_readv(this->pid, local, 1, remote, 1, 0);

    if (bytes_read == -1)
        std::cout << std::to_string(errno) << std::endl;
    
    return bytes_read == size;
}

unsigned long proc_handle::get_module_addr(std::string module_name) {
    std::ifstream istream("/proc/" + this->pid_str + "/maps");

    std::string line;
    while (std::getline(istream, line)) {
        if (line.find(module_name) != std::string::npos) {
            size_t first_separator = line.find_first_of("-");

            std::string addr = line.substr(0, first_separator);

            return strtoul(addr.c_str(), NULL, 16);
        }
    }

    return 0; 
}
