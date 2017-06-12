var express = require('express');
var app = express();
var server = require('http').Server(app);
var io = require('socket.io')(server);
var mysql = require('mysql');
var gps = require('wifi-location');
var ttn = require('ttn');
var sock = null;
var appId = 'tishcatrack';      // INSERT TTN YOUR AppEUI
var accessKey = 'ttn-account-v2.NuxDXbvuSyi0SJlXY0N5IOcZYyEgiyRS-btAukllQWI';   // INSERT TTN accessKey
var client = new ttn.Client('eu', appId, accessKey);
//------------------ ( TTN ) ---------------------------
client.on('connect', function() {
  console.log('Conectado a TTN');
});
client.on('message', function (deviceId, msg) {
  var formattedData = JSON.parse(JSON.stringify(msg, null, 2))
  formattedData.payload_raw = formattedData.payload_raw.data;
  console.info(formattedData.payload_raw);
  console.log();
  console.log("length " + formattedData.payload_raw.length);
  var mac = [];
  var rssi = [];
  var tmp = "";
  for(i=0; i < formattedData.payload_raw.length; i++) {
      if((i + 1) % 7 == 0) {
         tmp = tmp.slice(0,-1);
         rssi = parseInt(formattedData.payload_raw[i]);
         console.log("mac " + tmp + " rssi " + rssi);
         mac.push({ mac: tmp, ssid: '', signal_level: "-" + rssi });
         i++;
         tmp = "";
      }
      var hex = parseInt(formattedData.payload_raw[i]).toString(16);
      tmp  = tmp + hex + ":";
  }
  getLocation(mac);
});

client.on('activation', function (evt) {
  console.log("activation");
});

client.on('error', function (err) {
  console.error('[ERROR]', err.message);
});

function setSocket(s) {
  sock = s;
  console.log("socket");
  console.log(s);
}

function getLocation(plop) {
  console.log("get location");
  console.log(plop);
    gps.getLocation(plop, function(err, loc){
      console.log("location: " + JSON.stringify(loc));
      console.log("https://www.google.com/maps/place/" + loc.latitude + "," + loc.longitude);
      console.log(loc.latitude + ", " + loc.longitude);
      console.log("error: " + err);
      if(sock != null) {
        io.sockets.emit('location', loc.latitude + "," + loc.longitude);
      }
    });
}

//------------------------------------------------------
app.use(express.static('public'));

//--------------------------------------------
//        sql
var connection = mysql.createConnection({
    host: 'localhost',
    user: 'root',
    password: '',
    database: '',
    port: 3306
});

connection.connect(function(error) {
    if (error) {
        throw error;
    } else {
        console.log('Conexion correcta a base de datos.');
    }
});

//-------------------------( SOCKETS)------------------------------------
io.sockets.on('connection', function (socket) {
  console.log("Usuario Conectado por sockets");
    socket.on('messageChange', function (data) {
      console.log(data);
      socket.emit('receive', data.message.split('').reverse().join('') );
    });
});

//---------------------(SERVER)-----------------------


server.listen(80, function() {
    console.log("Servidor corriendo en http://localhost:8080");
});
