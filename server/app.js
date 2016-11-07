var express = require('express');
var path = require('path');
var favicon = require('serve-favicon');
var logger = require('morgan');
var cookieParser = require('cookie-parser');
var bodyParser = require('body-parser');

var app = express();

var server = require('http').Server(app);
var io = require('socket.io')(server);

var routes = require('./routes/index')(io); //index expects an impit
var users = require('./routes/users');

// view engine setup
app.set('views', path.join(__dirname, 'views'));
app.set('view engine', 'ejs');
app.locals.stdoutBuffer = 'foo';

// uncomment after placing your favicon in /public
//app.use(favicon(path.join(__dirname, 'public', 'favicon.ico')));
app.use(logger('dev'));
app.use(bodyParser.json());
app.use(bodyParser.urlencoded({extended: false}));
app.use(cookieParser());
app.use(express.static(path.join(__dirname, 'public')));

app.use('/', routes);
app.use('/users', users);

// add sockets to res
// app.use(function(req, res, next){
// 	res.io = io;
// 	next();
// });

// catch 404 and forward to error handler
app.use(function (req, res, next) {
	var err = new Error('Not Found');
	err.status = 404;
	next(err);
});

// error handlers

// development error handler
// will print stacktrace
if (app.get('env') === 'development') {
	app.use(function (err, req, res, next) {
		res.status(err.status || 500);
		res.render('error', {
			message: err.message,
			error: err
		});
	});
}

// production error handler
// no stacktraces leaked to user
app.use(function (err, req, res, next) {
	res.status(err.status || 500);
	res.render('error', {
		message: err.message,
		error: {}
	});
});


// send constant tick to client at interval
var tick = 0;
setInterval(function () {

	// console.log('ticking '+ tick);
	io.sockets.emit('tick from server', 'Time: '+ tick+ ' s');
	tick = tick + 1;

	//pooling app.locals example
	//log stdoutBuffer if it's anything other than empty string, not efficient depends on pooling
// 	var currentstdoutBuffer = app.locals.stdoutBuffer;
//
// 	if (currentstdoutBuffer != ''){
// 		io.sockets.emit('stdout', currentstdoutBuffer);
// 		app.locals.stdoutBuffer = '';
// 	};
//


}, 1000);

//chat example
//responder from action triggered by client
// io.sockets.on('connection', function (socket) {
// 	socket.on('request', function (data) {
// 		io.sockets.emit('response', 'coco ranheta: '+ data);
// 	})
// })

module.exports = {app: app, server: server};