module.exports = function (io) {


	var express = require('express');
	var router = express.Router();
	var os = require('os');
	var exec = require('child_process').exec;
// var execSync = require('child_process').execSync;
	var multer = require('multer');
	var fs = require('fs');



//Sample pool variables
	var storage = multer.diskStorage({
		destination: function (req, file, callback) {callback(null, 'public/sounds');},
		filename: function (req, file, callback) {callback(null, file.originalname);}
	});
	var upload = multer({ storage : storage}).single('filename');
	var samplesFilenameArray = ['OK'];
	var patchesFilenameArray = ['OK'];

	var systemInfo;
	var codeBuffer = 'OK';

	/* GET home page. */
	router.get('/', function(req, res, next) {

		systemInfo = getSystemInfo();
		samplesFilenameArray = fs.readdirSync('public/sounds');
		patchesFilenameArray = fs.readdirSync('public/patches');
		codeBuffer= fs.readFileSync('../sc/user.scd', 'utf8');

		res.render('index', {
			hostname: systemInfo[0],
			ethernetIP: systemInfo[1],
			wirelessIP: systemInfo[2],
			cpu: systemInfo[3],
			totalmem: systemInfo[4],
			freemem: systemInfo[5],
			samples: samplesFilenameArray,
			patches: patchesFilenameArray,
			code: codeBuffer
		});
	});

	/* POSTS*/
	router.post('/start_sc', function (req, res) {
		var codeBuffer = req.body.code;
		var child1 = exec('../bin/stopsc.sh');
		
		child1.on('close', function () {
			fs.writeFile("../sc/user.scd", codeBuffer, function (err) {
				if(err) {
					res.send('error saving file');
					return console.log(err);
				} else {
					// res.send('file saved');
					io.sockets.emit('stdout', 'file saved!');

					var child2 = exec('../bin/startsc.sh');
					child2.stdout.on('data', function (data) {
						io.sockets.emit('stdout', data);

					});
				}
			});
		res.redirect('/');
		});

	});

	router.post('/stop_sc', function (req, res) {
		var child = exec('../bin/stopsc.sh');
		// res.send('killed!');

		child.on('close', function (data) {
			// res.send(data);
			// res.app.locals.stdoutBuffer = data;
			io.sockets.emit('stdout', 'finished!');
			res.redirect('/');
		});
		// child.stdout.on('data', function (data) {
		// 	// res.send(data);
		// res.app.set('stdoutBuffer', data);
		// });

	});

	router.post('/save_code', function (req, res) {
		var codeBuffer = req.body.code;

		fs.writeFile("../sc/user.scd", codeBuffer, function (err) {
			if(err) {
				res.send('error saving file');
				return console.log(err);
			} else {
				// res.send('file saved');
				io.sockets.emit('stdout', 'file saved!');
				res.redirect('/');
			}
		});

	});

	router.post('/sample', function (req, res) {
		if(req.body.action === 'Delete'){
			// console.log('delete ' + req.body.filename);
			var samplesToDelete = req.body.sample;

			if (Array.isArray(samplesToDelete)) {
				for (var i in samplesToDelete) {
					fs.unlinkSync('public/sounds/' + samplesToDelete[i]);
				}
				// console.log('theres too many of them');
			} else {
				fs.unlinkSync('public/sounds/' + samplesToDelete);
				// console.log('alone in this world');
			}
		}
		res.redirect('/')

	});

	router.post('/patch', function (req, res) {
		if(req.body.action === 'Load'){

			var patchesToLoad = req.body.patch;

			if (Array.isArray(patchesToLoad) == false) {
				codeBuffer = fs.readFileSync(('public/patches/' + patchesToLoad), 'utf8');
				fs.writeFileSync("../sc/user.scd", codeBuffer);
			}
			res.redirect('/');
		}

		if(req.body.action === 'Delete'){
			var patchesToDelete = req.body.patch;

			if (Array.isArray(patchesToDelete)) {
				for (var i in patchesToDelete) {
					fs.unlinkSync('public/patches/' + patchesToDelete[i]);
				}
			} else {
				fs.unlinkSync('public/patches/' + patchesToDelete);
				// console.log('alone in this world');
			}
			res.redirect('/');
		}
	});

	router.post('/new_patch', function (req, res) {
		fs.writeFileSync('public/patches/' + req.body.name, req.body.code);
		patchesFilenameArray = fs.readdirSync('public/patches');
		console.log(patchesFilenameArray);

		res.send({redirect: '/'})

	});



	router.post('/upload_sample', function (req, res) {
		upload(req,res,function(err) {
			if (err) {
				return res.end("Error uploading file.");
			}
		});
		res.redirect('/')
	});

//main system routes
	router.post('/shutdown', function (req, res) {
		exec('sudo shutdown -h now');
		// var randomNumber = Math.random();
		// console.log('shutdown!');
		// console.log(randomNumber);
		// set stdoutBuffer which is pooled in app.js
		// res.app.set('stdoutBuffer', randomNumber);
		// io.sockets.emit('stdout', randomNumber);
		res.redirect('/');

	});

	router.post('/reboot', function (req, res) {
		exec('sudo reboot');
		// console.log('reboot!');
		res.redirect('/');
	});


//functions
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

	// module.exports = router;

	return router;


}
