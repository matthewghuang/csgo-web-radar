#include <string>
#include <cstring>
#include <sys/uio.h>

class proc_handle {
public:
    proc_handle() {};
    proc_handle(std::string target);

    bool is_running();
    std::string get_executable();
    unsigned long get_module_addr(std::string module_name);
    bool read(unsigned long, void *buffer, size_t size);

    pid_t pid;
    std::string pid_str;
};
