#include "handle.h"
#include "json.h"
#include "offsets.h"
#include "easywsclient/easywsclient.hpp"

#include <regex>
#include <memory>
#include <vector>
#include <string>
#include <ios>
#include <iostream>
#include <sys/types.h>
#include <dirent.h>
#include <unistd.h>

using json = nlohmann::json;

offsets offset = offsets{};
netvars netvar = netvars{};

bool find_process_by_name(std::string target, proc_handle *out_handle) {
    if (out_handle == nullptr || target.empty())
        return false;

    DIR *proc_dir = opendir("/proc/");

    struct dirent *dir_ent; // store dirs

    if (proc_dir != nullptr) {
        while ((dir_ent = readdir(proc_dir)) != nullptr) {
            char *name = dir_ent->d_name; // pid
            unsigned char type = dir_ent->d_type; if (type != DT_DIR)
                continue;

            std::string map_path = "/proc/" + std::string(name) + "/maps";
           
            proc_handle handle(name); // create a temporary object to validate process

            std::string exe_name = handle.get_executable();

            if (!exe_name.compare(target)) {
                *out_handle = handle;    

                return true;
            }
        }
    }

    return false;
}

int main(int argc, char *argv[]) {
    if (geteuid() != 0) { // check if user has root privileges
        std::cout << "run as root" << std::endl;
        exit(1);
    }

    using easywsclient::WebSocket;
    std::unique_ptr<WebSocket> ws(WebSocket::from_url("ws://localhost:6969"));

    proc_handle handle;

    bool found_process = find_process_by_name("csgo_linux64", &handle);

    if (!found_process) {
        std::cout << "unable to find csgo_linux64" << std::endl;
        exit(1);
    }

    std::cout << "found csgo_linux64, pid: " << handle.pid_str << std::endl;

    unsigned long engine_base = handle.get_module_addr("engine_client.so");
    unsigned long client_base = handle.get_module_addr("client_panorama_client.so");

    std::cout << "engine_base: " << std::hex << engine_base << std::endl;
    std::cout << "client_panorama_client: " << std::hex << client_base << std::endl;

    while (true) {
        unsigned long client_state_base;
        handle.read(engine_base + offset.client_state, &client_state_base, sizeof(client_state_base));

        char map_name_buffer[32];
        handle.read(client_state_base + offset.client_state_map, &map_name_buffer, sizeof(map_name_buffer));

        std::string map_name;

        bool first_letter = false;

        for (int i = 0; i < sizeof(map_name_buffer) - 1; ++i) {
            char c = map_name_buffer[i];

            // reading map name from memory leads to some weird characters
            // make sure we're only reading the map name
            if (isalpha(c) || isdigit(c) || c == '_') {
                if (!first_letter)
                    first_letter = true;

                map_name += c;
            } else {
                if (first_letter)
                    break;
            }
        }

        unsigned long local_base;
        handle.read(client_base + offset.local_player, &local_base, sizeof(local_base));

        int local_team;
        handle.read(local_base + netvar.team, &local_team, sizeof(local_team));

        std::vector<int> ids;
        std::vector<float> x_positions;
        std::vector<float> y_positions;
        std::vector<int> healths;
        std::vector<std::string> last_places;

        for (int i = 1; i <= 64; ++i) {
            unsigned long entity_base;
            handle.read(client_base + offset.entity_list + i * 0x10, &entity_base, sizeof(entity_base));

            if (entity_base == NULL || entity_base == local_base)
                continue;

            int entity_health;
            handle.read(entity_base + netvar.health, &entity_health, sizeof(entity_health));

            if (entity_health <= 0 || entity_health > 100) // check if entity is alive
                continue;

            int entity_team;
            handle.read(entity_base + netvar.team, &entity_team, sizeof(entity_team));

            // TODO: show both teams on radar?
            if (local_team == entity_team)
                continue;

            float entity_origin[2];
            handle.read(entity_base + netvar.origin, &entity_origin, sizeof(entity_origin));

            char last_place[32];
            handle.read(entity_base + netvar.last_place, &last_place, sizeof(last_place));

            ids.push_back(i);
            x_positions.push_back(entity_origin[0]);
            y_positions.push_back(entity_origin[1]);
            healths.push_back(entity_health);
            last_places.push_back(std::string(last_place));
        }

        json data;
        data["ids"] = ids;
        data["map_name"] = map_name;
        data["x_positions"] = x_positions;
        data["y_positions"] = y_positions;
        data["healths"] = healths;
        data["last_places"] = last_places;

        ws->send(data.dump());
        ws->poll();

        usleep(100 * 1000);
    }

    return 0;
}
