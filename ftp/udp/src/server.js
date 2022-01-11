const PORT = 9876;
const HOST = '0.0.0.0';

const dgram = require('dgram');
const server = dgram.createSocket('udp4');

let sequenceNumber = 0;

server.on('message', (message, remote) => {
  const servePrintMsg = "serve收到的第"+sequenceNumber.toString()+"条信息: " + message.toString();
  console.log(servePrintMsg);

  const sendMsg = sequenceNumber.toString() + " " + message.toString();
  const buffer = Buffer.from(sendMsg);
  server.send(buffer, 0, buffer.length, remote.port, remote.address);

  sequenceNumber++;
});

server.bind(PORT, HOST);
