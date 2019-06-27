#include "json.h"
#include "offsets.h"
#include "process.h"
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

offsets offset;
netvars netvar;

int main(int argc, char *argv[]) {
    if (geteuid() != 0) { // check if user has root privileges
        std::cout << "run as root" << std::endl;
        exit(1);
    }

    using easywsclient::WebSocket;
    std::unique_ptr<WebSocket> ws(WebSocket::from_url("ws://localhost:6969"));

    process::proc_handle handle;

    bool found_process = process::find_process_by_name("csgo_linux64", &handle);

    if (!found_process) {
        std::cout << "unable to find csgo_linux64" << std::endl;
        exit(1);
    }

    std::cout << "found csgo_linux64, pid: " << std::to_string(handle.pid) << std::endl;

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

        json data;
        data["map_name"] = map_name;
        data["x_positions"] = json::array();
        data["y_positions"] = json::array();
        data["healths"] = json::array(); 
        data["last_places"] = json::array(); 
        data["simulation_times"] = json::array();

        for (int i = 1; i <= 64; ++i) {
            unsigned long entity_base;
            handle.read(client_base + offset.entity_list + i * 0x10, &entity_base, sizeof(entity_base));

            if (entity_base == (unsigned long) NULL || entity_base == local_base)
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

            int simulation_time;
            handle.read(entity_base + netvar.simulation_time, &simulation_time, sizeof(simulation_time));

            data["x_positions"].push_back(entity_origin[0]);
            data["y_positions"].push_back(entity_origin[1]);
            data["healths"].push_back(entity_health);
            data["last_places"].push_back(std::string(last_place));
            data["simulation_times"].push_back(simulation_time);
        }

        ws->send(data.dump());
        ws->poll();

        usleep(100 * 1000);
    }

    return 0;
}
