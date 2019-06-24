const WebSocket = require("ws");
const fs = require("fs");

const express = require("express");
const app = express();
const web_port = 23417;

const wss = new WebSocket.Server({
    port: 6969
});

wss.on("connection", (ws) => {
    console.log("new connection");

    ws.on("message", (message) => { // when we recieve data from provider send to all other clients
        wss.clients.forEach((client) => {
            client.send(message); 
        });
    });
});

app.get("/", (req, res) => {
    res.sendFile(__dirname + "/index.html");
});

app.get("/data/images/:imgId", (req, res) => {
    res.sendFile(__dirname + "/data/images/" + req.params.imgId);
});

let stored_maps = {};

const base_dir = __dirname + "/data/";

// parse map data files to find position offsets and scale of the maps
fs.readdir(base_dir, (err, filenames) => {
    if (err)
        return;

    for (const filename of filenames) {
        fs.readFile(base_dir + filename, "utf-8", (err, content) => {
            if (err)
                return;

            read_value = (key) => {
                const start_idx = content.indexOf(key);

                let numbers = "";
                let found_number = false;

                const subset = content.substr(start_idx);

                for (const c of subset) {
                    if (c.match(/[0-9.-]/)) {
                        if (!found_number)
                            found_number = true;

                        numbers += c;
                    } else {
                        if (found_number)
                            return parseFloat(numbers);
                        else
                            continue;
                    }
                }
            }

            const pos_x = read_value("pos_x");
            const pos_y = read_value("pos_y");
            const scale = read_value("scale");

            const map_name_end = filename.indexOf(".");
            const map_name = filename.substr(0, map_name_end);

            stored_maps[map_name] = {
                pos_x: pos_x,
                pos_y: pos_y,
                scale: scale
            };
        });
    }
});

app.get("/data/:dataId", (req, res) => {
    const map = req.params.dataId;

    if (stored_maps[map])
        res.send({ pos_x: stored_maps[map].pos_x, pos_y: stored_maps[map].pos_y, scale: stored_maps[map].scale });
});

app.listen(web_port, () => {
    console.log("CS:GO Web Radar listening on port %i", web_port);
});
