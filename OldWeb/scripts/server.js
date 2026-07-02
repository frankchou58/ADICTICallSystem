//import express 和 ws 套件
const WebSocket = require('ws');
 
const wss = new WebSocket.Server({
  port: 3000
});

wss.on('connection', function connection(ws) {
  console.log('server connection')
  ws.on('message', function incoming(message) {
    console.log('received: %s', message);
  });
  ws.send('something');
});