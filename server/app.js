var express = require('express');
var path = require('path');
var favicon = require('serve-favicon');
var logger = require('morgan');
var cookieParser = require('cookie-parser');
var bodyParser = require('body-parser');
var OSC = require('osc-js');
// var serveIndex = require('serve-index');

var os = require('os');
var cp = require('child_process');
var fs = require('fs');
var sc = require('supercolliderjs');

var sclang;
var jackd;
var pbridge;

var index = require('./routes/index');
var users = require('./routes/users');
var system = require('./routes/system');
var sensors = require('./routes/sensors');
var gui = require('./routes/gui');

var app = express();

// create sockets
var server = require('http').Server(app);
var io = require('socket.io')(server);

//get config file
var config = require('./private/config.json');

// poll system info
var systemTick = 0;
var systemInfo;

//public path
var supercolliderfiles_path = path.join(__dirname, 'public/supercolliderfiles/');

// view engine setup
app.set('views', path.join(__dirname, 'views'));
app.set('view engine', 'ejs');

//socket.io setup
app.set('socketio', io);

app.use(favicon(path.join(__dirname, 'public/images', 'favicon.ico')));
app.use(logger('dev'));
app.use(bodyParser.json());
app.use(bodyParser.urlencoded({ extended: false }));
app.use(cookieParser());



//middleware for sockets
app.use(function(req, res, next){
	res.io = io;
	next();
});

app.use('/', index);
app.use('/users', users);
app.use('/system', system);
app.use('/sensors', sensors);
app.use('/gui',gui);
app.use(express.static(path.join(__dirname, 'public')));


// catch 404 and forward to error handler
app.use(function(req, res, next) {
	var err = new Error('Not Found')
	err.status = 404;
	next(err);
});

// error handler
app.use(function(err, req, res, next) {
	// set locals, only providing error in development
	res.locals.message = err.message;
	res.locals.error = req.app.get('env') === 'development' ? err : {};

	// render the error page
	res.status(err.status || 500);
	res.render('error');
});


////////////


function startJack() {
		var device = config.jack.device;
		var vectorSize = config.jack.vectorSize;
		var sampleRate = config.jack.sampleRate;
		var deviceParam = '-dhw:'+device;
		var vectorSizeParam = '-p'+vectorSize;
		var sampleRateParam = '-r'+sampleRate;

		jackd = cp.spawn('jackd', ['-P95', '-dalsa', deviceParam, vectorSizeParam, '-n3', '-s', sampleRateParam]);
}

function startPbridge() {
		var target = path.join(__dirname, '../pbridge/pbridge');
		var param = '-'+config.sensorDataTarget;

		pbridge = cp.spawn(target, [param]);
}

//start sclang
function startSclang() {
		sc.lang.boot({stdin: false, echo: false, debug: false}).then(function (lang) {
			sclang = lang;
			sclang.on('stdout', function (text) {
				io.sockets.emit('toconsole', text);
			});
			sclang.on('state', function (text) {
				io.sockets.emit('toconsole', JSON.stringify(text));
			});
			sclang.on('stderror', function (text) {
				io.sockets.emit('toconsole', JSON.stringify(text));
			});
			sclang.on('error', function (text) {
				io.sockets.emit('toconsole', JSON.stringify(text.error.errorString));
			});
		}).then(function () {
			fs.access(supercolliderfiles_path + config.defaultSCFile, function (err) {
				if (err){
					io.sockets.emit('toconsole', 'cannot find default file. Check your settings...\n');
				} else {
					sclang.executeFile(path.join(supercolliderfiles_path, config.defaultSCFile)).then(
						function (answer) {
							io.sockets.emit('toconsole', JSON.stringify(answer) + '\n');
						},
						function (error) {
							io.sockets.emit('toconsole', 'error type:' + JSON.stringify(error.type) + '\n');
						}
					)
				};
			});
		})
}

function start() {
	startPbridge();
	startJack();
	startSclang();
	setTimeout(function(){app.emit('recallsensors');}, 4);
}

function getCpuUsage(cmd) {
    try {
        return cp.execSync(cmd).toString();
    }
    catch (error) {
        error.status;  // Might be 127 in your example.
        error.message; // Holds the message you typically want.
        error.stderr;  // Holds the stderr output. Use `.toString()`.
        error.stdout;  // Holds the stdout output. Use `.toString()`.
    }
};



function getSystemInfo() {
	var wirelessIP, ethernetIP, hostname, freemem, cpuSclang, cpuScsynth;

    if (os.networkInterfaces().wlan0) {wirelessIP = os.networkInterfaces().wlan0[0].address;
	} else {wirelessIP = 'unavailable';}
	if (os.networkInterfaces().eth0) {ethernetIP = os.networkInterfaces().eth0[0].address;
	} else {ethernetIP = 'unavailable';}
    cpuSclang = getCpuUsage('ps --no-headers -C sclang -o %cpu');
    cpuScsynth = getCpuUsage('ps --no-headers -C scsynth -o %cpu');
	hostname = os.hostname();
	freemem = (Math.round(os.freemem()/1000000));

	return([hostname, ethernetIP, wirelessIP, cpuSclang, cpuScsynth, freemem]);

};

setInterval(function () {
	systemInfo = getSystemInfo();
	systemTick = systemTick +1;
	io.sockets.emit('systeminfo', {
		tick: systemTick,
		hostname: systemInfo[0],
		ethernetip: systemInfo[1],
		wirelessip: systemInfo[2],
		cpusclang: systemInfo[3],
        cpuscsynth: systemInfo[4],
		freemem: systemInfo[5]
	})
}, 1000);




//interprets in supercollider code (receives from post via socket and outputs to console via socket)
app.on('interpret', function (msg) {
	if(sclang !== null){
		sclang.interpret(msg, null, true, true, false)
				.then(function(result) {
					io.sockets.emit('toconsole', result);
				})
				.catch(function (error) {
					var errorStringArray = JSON.stringify(error.error, null, ' ');
					io.sockets.emit('toconsole', errorStringArray + '\n\n\n');

				});
	};
});

//saves and runs temporary supercollider file
app.on('runtemp', function (msg) {
	if(sclang !== null){
		fs.writeFile(supercolliderfiles_path + '.temp.scd', msg, function (err) {
			if (err) {
				res.send('error saving file');
				return console.log(err);
			} else {
				io.sockets.emit('toconsole', '\ntemp file saved');

				sclang.executeFile(path.join(supercolliderfiles_path, '.temp.scd')).then(
					function (answer) {
						io.sockets.emit('toconsole', JSON.stringify(answer.string) + '\n');
					},
					function (error) {
						io.sockets.emit('toconsole',
							'cannot run or find temp file.\n'
							+ 'error type:' + JSON.stringify(error.type) + '\n'
							+ JSON.stringify(error.error, null, ' ') + '\n'
						);
					}
				);
			}
			;
		});
	};

});


// restart jack, supercollider and pbridge
app.on('restartSclang', function () {
	if(sclang !== null){
		sclang.interpret('0.exit', null, true, true, false)
			.then(function(result) {
				io.sockets.emit('toconsole', 'sclang quitting...');
				sclang = null;
				setTimeout(function(){startSclang()}, 4);
			})
			.catch(function (error) {
				io.sockets.emit('toconsole', JSON.stringify(error));
			});
	};
});


//OSC from client
var osc = new OSC({
	plugin: new OSC.DatagramPlugin({ send: { port: 57100, host: '127.0.0.1' }, open: { port: 57101, host:'127.0.0.1'}})
});

const oscSC = new OSC({
	plugin: new OSC.DatagramPlugin({ send: { port: 57120, host: '127.0.0.1' }, open: { port: 57142, host:'127.0.0.1'}})
});

osc.open();

//set sensor parameters via OSC from client socket message
io.sockets.on('connection', function(client){

	client.on('OSCSensorConfig', function(data){
		var message;
		switch (data.type) {
			case '/sensorFilter':
				message = new OSC.Message(
					data.type,
					data.address,
					data.param.filtertype,
					data.param.filterparam1,
					data.param.filterparam2,
					data.param.filterparam3
				);
				break;
			default:
				message = new OSC.Message(data.type, data.address, data.param);
		};
		osc.send(message);
	});

	client.on('sensorSampleRateConfig', function(data){
		var message = new OSC.Message(data.type, data.param);
		osc.send(message);
	});

	client.on('sensorselected', function(data){
		var message;
		if (data == -1) {
			var multiplexer = -1;
			var sensor = -1;
		} else {
			var multiplexer = Math.floor(data / 8);
			var sensor = (data % 8);
		};

		message = new OSC.Message('/sensorSelected', multiplexer, sensor);
		osc.send(message);
	});

	client.on('gui', function(data){
		// console.log(data);
		// console.log(data[0]);
		// console.log(data[1]);
		message = new OSC.Message(data[0], data[1]);
		oscSC.send(message);
	});
	
});

//receive OSC with sensor values that is being monitored from pbridge and relayed to client via socket
osc.on('/sensorMonitorValue', function (message) {
	io.sockets.emit('sensormonitorvalue', {data: message.args[0]});
});

osc.on('/sendSensorConfig', function (message) {
	// sendSensorConfigOSC('recall');
	app.emit('recallsensors');
});

// function to dump send sensor configuration to bridge via OSC
function sendSensorConfigOSC (json){
	var sensorsConfig = json;

	var message;
	var iterator= 0;
	var dumpSpeed = 10;

	//initialize sample rate
	message = new OSC.Message('/sampleRate', sensorsConfig.samplerate);
	osc.send(message);

	setInterval(function () {
		var multiplexer = Math.floor(iterator / 8);
		var sensor = (iterator % 8);
		message = new OSC.Message('/sensorActive', '/'+multiplexer+'/'+sensor, sensorsConfig.mask[iterator]);
		osc.send(message);
		message = new OSC.Message('/sensorResolution', '/'+multiplexer+'/'+sensor, sensorsConfig.resolution[iterator]);
		osc.send(message);
		message = new OSC.Message('/sensorFilter', '/'+multiplexer+'/'+sensor, sensorsConfig.filtertype[iterator], sensorsConfig.filterparam1[iterator], sensorsConfig.filterparam2[iterator], sensorsConfig.filterparam3[iterator]);
		osc.send(message);
		message = new OSC.Message('/sensorName', '/'+multiplexer+'/'+sensor, sensorsConfig.OSCaddress[iterator]);
		osc.send(message);
		iterator = iterator +1;
		if(iterator==128) {clearInterval(this)};
	}, dumpSpeed);
}



app.on('resetsensors', function () {
	fs.readFile(path.join(__dirname, 'public/config/sensors_reset.json'), 'utf8', function (err, data) {
		if (err) throw err; // we'll not consider error handling for now
		var temp = JSON.parse(data);
		sendSensorConfigOSC(temp);
	});
})

app.on('recallsensors', function () {
	fs.readFile(path.join(__dirname, 'public/config/sensors.json'), 'utf8', function (err, data) {
		if (err) throw err; // we'll not consider error handling for now
		var temp = JSON.parse(data);
		sendSensorConfigOSC(temp);
	});
});

start();

module.exports = {app: app, server: server};

