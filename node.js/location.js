var gps = require('wifi-location');
var ttn = require('ttn');
var app = require('http').createServer(handler)
var io = require('socket.io')(app);
var fs = require('fs');
var sock = null;
app.listen(80);
var appId = '';      // INSERT TTN YOUR AppEUI
var accessKey = '';   // INSERT TTN accessKey
var client = new ttn.Client('eu', appId, accessKey);

function handler (req, res) {
  fs.readFile(__dirname + '/index.html',
  function (err, data) {
    if (err) {
      res.writeHead(500);
      return res.end('Error loading index.html');
    }
    res.writeHead(200);
    res.end(data);
  });
}

io.on('connection', function (socket) {
  socket.emit('news', { hello: 'world' });
  socket.on('my other event', function (data) {
    console.log(data);
  });
  setSocket(socket);
});

client.on('connect', function() {
  console.log('connected');
});

io.sockets.on('connection', function (socket) {
    socket.on('messageChange', function (data) {
      console.log(data);
      socket.emit('receive', data.message.split('').reverse().join('') );
    });
});

client.on('uplink', function (msg) {
  var raw = new Buffer(msg.fields.raw, 'base64');
  console.log();
  console.log("lengte " + raw.length);
  var mac = [];
  var rssi = [];
  var tmp = "";
  for(i=0; i < raw.length; i++) {
      if((i + 1) % 7 == 0) {
         tmp = tmp.slice(0,-1);
         rssi = parseInt(raw[i]);
         console.log("mac " + tmp + " rssi " + rssi);
         mac.push({ mac: tmp, ssid: '', signal_level: "-" + rssi });
         i++;
         tmp = "";
      }
      var hex = parseInt(raw[i]).toString(16);
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
