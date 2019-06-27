#include <string>
#include <sys/uio.h>

namespace process {
    class proc_handle;

    bool find_process_by_name(std::string target_process, proc_handle *target);
}

class process::proc_handle {
public:
    proc_handle() {};
    proc_handle(int target);

    std::string get_executable();
    unsigned long get_module_addr(std::string module_name);
    bool read(unsigned long addr, void *buffer, size_t size);

    pid_t pid;
};
