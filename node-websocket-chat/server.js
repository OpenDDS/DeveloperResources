const ChatClient = require('./chat')

const express = require('express');
const http = require('http');
const WebSocket = require('ws');

var my_args = process.argv.slice(2);
var port_index = my_args.indexOf("--port");
var PORT = (port_index == -1 || my_args.size <= port_index + 1) ? 7000 : my_args[port_index + 1];

const server = http.createServer(express);
const wss = new WebSocket.Server({ server })
const chatClient = new ChatClient();

const on_message = (userMessage) => {
    console.log("Received a message " + JSON.stringify(userMessage));
    wss.clients.forEach(function each(client) {
        if (client.readyState === WebSocket.OPEN) {
            client.send(userMessage.message);
        }
    })
};

// Split out DDS args
const ddsArgs = process.argv.slice(process.argv.indexOf(__filename) + 1);

chatClient.initializeDds(ddsArgs);

chatClient.subscribe_user_messages(on_message);

wss.on('connection', function connection(ws) {
    ws.on('message', function incoming(data) {
        const userMessage = {
            message: data,
        };
        chatClient.write_user_message(userMessage);
    })
})

server.listen(PORT, function() {
  console.log(`Server is listening on ${PORT}!`)
})
