'use strict';

const http = require('http');
const WebSocketServer = require('websocket').server;

class ProxyServer {
  constructor() {
    this.called = false;
    this.lastReq = null;
    this.httpServer = http.createServer((req, res) => {
      this.called = true;
      this.lastReq = req;
      res.writeHead(200, { 'Content-Type': 'text/plain' });
      res.end('okay');
    });
    this.httpServer.listen(3001);

    let wsServer = new WebSocketServer({
      httpServer: this.httpServer,
      autoAcceptConnections: true,
    });

    let websocketEvents = this.websocketEvents = [];
    wsServer.on('connect', connection => {
      websocketEvents.push('connect');

      connection.on('message', message => {
        websocketEvents.push(`message: ${message.utf8Data}`);
        connection.sendUTF(message.utf8Data);
      });

      connection.on('error', error => {
        websocketEvents.push(`error: ${error}`);
      });

      connection.on('close', () => {
        websocketEvents.push(`close`);
      });
    });
  }
}

module.exports = ProxyServer;
