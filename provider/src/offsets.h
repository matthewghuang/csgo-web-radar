struct offsets {
    const static unsigned long client_state = 0x589DCC;
    const static unsigned long client_state_map = 0x28C;
    const static unsigned long client_state_max_player = 0x388;
    const static unsigned long entity_list = 0x4D43AB4;
    const static unsigned long local_player = 0xD2FB84;
};

struct netvars {
    const static unsigned long health = 0x100;
    const static unsigned long origin = 0x138;
    const static unsigned long last_place = 0x35B4;
    const static unsigned long team = 0xF4;
    const static unsigned long simulation_time = 0x268;
};

extern offsets offset;
extern netvars netvar;
