const PORT = 9876;
const HOST = '127.0.0.1';

const dgram = require('dgram');
const client = dgram.createSocket('udp4');

//接收消息
let receiveNum = -1;
client.on('message', message => {
    console.log("收到回复：’"+message.toString()+ "‘");
    receiveNum++;
    if (receiveNum == 50) {
      client.close();
    }
});


for(let i = 0; i <= 50; i++){
  const sendBuffer = i.toString();
  const sendLen = sendBuffer.length;
  client.send(sendBuffer, 0, sendLen, PORT, HOST)
}

