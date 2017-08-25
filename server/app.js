var express = require('express');
var path = require('path');
var favicon = require('serve-favicon');
var logger = require('morgan');
var cookieParser = require('cookie-parser');
var bodyParser = require('body-parser');

let os = require('os');
let exec = require('child_process').exec;
let execSync = require('child_process').execSync;

let multer = require('multer');
let fs = require('fs');
let sc = require('supercolliderjs');

let sclang;
let jackd;
let serial2osc;

var index = require('./routes/index');
var users = require('./routes/users');
var system = require('./routes/system');

var app = express();

// create server here and set sockets
var server = require('http').Server(app);
var io = require('socket.io')(server);

//get config file
var config = require('./private/config.json');

// poll system info
var systemTick = 0;
var systemInfo;

//public path
let supercolliderfiles_path = path.join(__dirname, 'public/supercolliderfiles/');
let public_path = path.join(__dirname, '../public/');

// view engine setup
app.set('views', path.join(__dirname, 'views'));
app.set('view engine', 'ejs');

//socket.io setup
app.set('socketio', io);

// uncomment after placing your favicon in /public
//app.use(favicon(path.join(__dirname, 'public', 'favicon.ico')));
app.use(logger('dev'));
app.use(bodyParser.json());
app.use(bodyParser.urlencoded({ extended: false }));
app.use(cookieParser());
app.use(express.static(path.join(__dirname, 'public')));

//middleware for sockets
// add socket.io to res in event loop
app.use(function(req, res, next){
	res.io = io;
	next();
});

app.use('/', index);
app.use('/users', users);
app.use('/system', system);


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
//start jackd
if(jackd == null){
	let device = config.jack.device;
	let vectorSize = config.jack.vectorSize;
	let sampleRate = config.jack.sampleRate;

	let command = 'jackd -P75 -dalsa -dhw:'+device+' -p'+vectorSize+' -n3 -s -r'+sampleRate;
	// console.log('jack command: '+command);

	jackd = exec(command, function (error, stdout, stderr) {
		console.log(stdout);
	});

};

//start serial2osc
if(serial2osc == null){
	let target = path.join(__dirname, '../serial2osc/serial2osc')+' -'+config.sensorDataTarget;

	serial2osc = exec(target, function (error, stdout, stderr) {
		console.log(stdout);
	});
};

//start sclang


if(sclang == null) {
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
			io.sockets.emit('toconsole', JSON.stringify(text));
		});
	}).then(function () {
		sclang.executeFile(path.join(supercolliderfiles_path, config.defaultSCFile)).then(
			function (answer) {
				io.sockets.emit('toconsole', JSON.stringify(answer) + '\n');
			},
			function (error) {
				io.sockets.emit('toconsole', 'cannot run or find default file. Check your settings...\n');
				io.sockets.emit('toconsole', 'error type:' + JSON.stringify(error.type) + '\n');
			}
		)
		// console.log(path.join(supercolliderfiles_path, 'default.scd'));
	})
};



//interprets in supercolliderfiles (receives from post via socket and outputs to console via socket)
app.on('interpret', function (msg) {
	if(sclang !== null){
		sclang.interpret(msg, null, true, true, false)
				.then(function(result) {
					io.sockets.emit('toconsole', result);
				})
				.catch(function (error) {
					io.sockets.emit('toconsole', JSON.stringify(error));
				});
	};

});

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
						io.sockets.emit('toconsole', JSON.stringify(answer) + '\n');
					},
					function (error) {
						io.sockets.emit('toconsole', 'cannot run or find temp file.\n');
						io.sockets.emit('toconsole', 'error type:' + JSON.stringify(error.type) + '\n');
					}
				);
			}
			;
		});
	};

});

var getSystemInfo = function () {
	var wirelessIP, ethernetIP, cpu, hostname, totalmem, freemem;
	if (os.networkInterfaces().wlan0) {wirelessIP = os.networkInterfaces().wlan0[0].address;
	} else {wirelessIP = 'unavailable';}
	if (os.networkInterfaces().eth0) {ethernetIP = os.networkInterfaces().eth0[0].address;
	} else {ethernetIP = 'unavailable';}
	cpu = (Math.round(os.loadavg()[0]));
	hostname = os.hostname();
	totalmem = (Math.round(os.totalmem()/1000000));
	freemem = (Math.round(os.freemem()/1000000));

	return([hostname, ethernetIP, wirelessIP, cpu,  totalmem, freemem]);
};

setInterval(function () {
	systemInfo = getSystemInfo();
	systemTick = systemTick +1;
	io.sockets.emit('systeminfo', {tick: systemTick, hostname: systemInfo[0], ethernetip: systemInfo[1], wirelessip: systemInfo[2], cpu: systemInfo[3], totalmem: systemInfo[4], freemem: systemInfo[5]})
}, 1000);

///////////

module.exports = {app: app, server: server};