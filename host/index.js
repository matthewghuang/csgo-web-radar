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

let saved_maps = {};

app.get("/data/:dataId", (req, res) => {
    const map = req.params.dataId;

    fs.readFile(__dirname + "/data/" + map + ".txt", "utf8", (err, data) => {
        read_numbers = (str, start_idx) => {
            let numbers = "";
            let found_number = false;

            const subset = str.substr(start_idx);

            for (const c of subset) {
                if ("0123456789.-".includes(c)) { // check if we've hit a number
                    found_number = true;
                    numbers += c;   
                } else {
                    if (found_number)
                        return parseFloat(numbers);
                    else
                        continue;
                }
            }
        };

        if (saved_maps.hasOwnProperty(map))
            res.send({ x: saved_maps[map].x, y: saved_maps[map].y, scale: saved_maps[map].scale });
        else {
            const pos_x_start = data.indexOf("pos_x");
            const pos_x = read_numbers(data, pos_x_start);

            const pos_y_start = data.indexOf("pos_y");
            const pos_y = read_numbers(data, pos_y_start);

            const scale_start = data.indexOf("scale");
            const scale = read_numbers(data, scale_start);

            const obj = {
                x: pos_x,
                y: pos_y,
                scale: scale
            };

            saved_maps[map] = obj;

            res.send(obj);
        }
    });
});

app.listen(web_port, () => {
    console.log("CS:GO Web Radar listening on port %i", web_port);
});
