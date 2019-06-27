struct offsets {
    const static unsigned long client_state = 0xe20d08;
    const static unsigned long client_state_map = 0x324;
    const static unsigned long client_state_max_player = 0x420;
    const static unsigned long entity_list = 0x217b068;
    const static unsigned long local_player = 0x214af10;
};

struct netvars {
    const static unsigned long health = 0x138;
    const static unsigned long origin = 0x170;
    const static unsigned long last_place = 0x3df0;
    const static unsigned long team = 0x12c;
    const static unsigned long simulation_time = 0x2a0;
};

extern offsets offset;
extern netvars netvar;
