#include "handle.h"
#include "json.h"
#include "easywsclient/easywsclient.hpp"

#include <memory>
#include <vector>
#include <string>
#include <ios>
#include <iostream>
#include <sys/types.h>
#include <dirent.h>
#include <unistd.h>

using json = nlohmann::json;

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

            if (access(map_path.c_str(), F_OK) == -1) // if we can't access the path for whatever reason (not root?)
                continue;
           
            proc_handle handle(name); // create a temporary object to check if we have the right file

            if (!handle.is_valid() || !handle.is_running())
                continue;

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
    if (geteuid() != 0) {
        std::cout << "run as root" << std::endl;
        exit(1);
    }

    if (argc < 2) {
        std::cout << "please pass in map name" << std::endl;
        exit(1);
    }

    std::string map_name = argv[1];

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

    unsigned long local_player_offset = 0x214af10;
    unsigned long entity_list_offset = 0x217b068;
    unsigned long client_state_offset = 0xe20d08;

    unsigned long entity_health_offset = 0x138;
    unsigned long entity_team_num_offset = 0x12c;
    unsigned long entity_vec_origin_offset = 0x170;
    unsigned long entity_last_place_offset = 0x3df0;

    while (true) {
        unsigned long local_base;
        handle.read(client_base + local_player_offset, &local_base, sizeof(local_base));

        if (local_base == NULL)
            continue;

        int local_team;
        handle.read(local_base + entity_team_num_offset, &local_team, sizeof(local_team));

        std::vector<float> x_coords;
        std::vector<float> y_coords;
        std::vector<int> healths;
        std::vector<std::string> last_places;

        for (int i = 1; i <= 64; ++i) {
            unsigned long player_base;
            handle.read(client_base + entity_list_offset + (i * 0x10), &player_base, sizeof(player_base));

            if (player_base == NULL)
                continue;

            int player_health;
            handle.read(player_base + entity_health_offset, &player_health, sizeof(player_health));

            if (player_health <= 0 || player_health > 100)
                continue;

            int player_team;
            handle.read(player_base + entity_team_num_offset, &player_team, sizeof(player_team));

            if (local_team == player_team)
                continue;

            float pos[3];
            handle.read(player_base + entity_vec_origin_offset, &pos, sizeof(pos));

            char last_place[32];
            handle.read(player_base + entity_last_place_offset, &last_place, sizeof(last_place));

            x_coords.push_back(pos[0]);
            y_coords.push_back(pos[1]);
            healths.push_back(player_health);
            last_places.push_back(std::string(last_place));
        }

        json data;
        data["map_name"] = map_name;
        data["x"] = x_coords;
        data["y"] = y_coords;
        data["health"] = healths;
        data["last_place"] = last_places;

        //std::cout << "sending" << std::endl;
        //std::cout << coords.dump() << std::endl;
        ws->send(data.dump());
        ws->poll();

        usleep(10 * 1000);
    }

    return 0;
}
