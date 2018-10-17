var express = require('express');
var router = express.Router();
var multer = require('multer');
var fs = require('fs');
var path = require('path');
var exec = require('child_process').exec;

var storage = multer.diskStorage({
	destination: function (req, file, callback) {callback(null, (public_path + 'soundfiles'));},
	filename: function (req, file, callback) {callback(null, file.originalname);}
});

var upload = multer({ storage : storage}).array('filename', 10);
var soundfiles = [''];
var supercolliderfiles = [''];
var public_path = path.join(__dirname, '../public/');
var tempcode;


router.get('/', function(req, res, next) {
	refreshFiles();
	res.render('index', {supercolliderfiles: supercolliderfiles, soundfiles: soundfiles, tempcode: tempcode});
});

router.post('/interpret', function (req, res) {
	res.app.emit('interpret', req.body.code);
	res.sendStatus(200);
});

router.post('/runtemp', function (req, res) {
	tempcode = req.body.code;
	res.app.emit('runtemp', tempcode);
	res.sendStatus(200);
});

router.get('/refresh-files', function (req, res) {
	refreshFiles();
	res.io.emit('tosupercolliderfiles', supercolliderfiles);
	res.io.emit('tosoundfiles', soundfiles);
	res.redirect('/');
})

router.post('/shutdown', function (req, res) {
	exec('sudo shutdown -h now', function (error, stdout, stderr) {
		console.log(stdout);
	});
})

router.post('/reboot', function (req, res) {
	exec('sudo reboot', function (error, stdout, stderr) {
		console.log(stdout);
	});
})

router.post('/restartsclang', function (req, res) {
	res.app.emit('restartSclang');
	res.sendStatus(200);
});


router.post('/supercolliderfiles', function (req, res) {

	if(req.body.action === 'load'){
		var filetoload = supercolliderfiles[JSON.parse(req.body.fileindex)[0]];
		var buffer = fs.readFileSync((public_path + 'supercolliderfiles/' + filetoload), 'utf8');
		tempcode = buffer;
        res.io.emit('toprompt', filetoload);
        res.io.emit('toconsole', filetoload);
        res.io.emit('toeditor', buffer);
		res.sendStatus(200);
	};

	if(req.body.action === 'save'){
		tempcode = req.body.code;
		fs.writeFile(public_path + 'supercolliderfiles/' + req.body.filename, tempcode, function (err) {
			if(err) {
				res.send('error saving file');
				return console.log(err);
			} else {
				res.io.emit('toconsole', '\nfile saved');
                res.io.emit('toprompt', req.body.filename);
                res.redirect('refresh-files');
			}
		});
	};

	if(req.body.action === 'delete') {
		var filestodelete = JSON.parse(req.body.fileindex);
		for (i in filestodelete) {
			var fullpath = public_path + 'supercolliderfiles/' + supercolliderfiles[filestodelete[i]];
			fs.unlinkSync(fullpath);
		}
		res.redirect('/refresh-files');
	}
})

router.post('/soundfiles', function (req, res) {

	if(req.body.action === 'upload'){
		upload(req,res,function(err) {
			if (err) {
				return res.end("Error uploading file.");
			}
		});
		res.redirect('/refresh-files');
	};

	if(req.body.action === 'delete'){
		var filestodelete = JSON.parse(req.body.fileindex);
		for (i in filestodelete) {
			var fullpath = public_path + 'soundfiles/' + soundfiles[filestodelete[i]];
			fs.unlinkSync(fullpath);
		}
		res.redirect('/refresh-files');
	};
});

router.post('/soundfileupload', function (req, res) {
	upload(req,res,function(err) {
		if (err) {
			return res.end("Error uploading file.");
		}
	});
	res.redirect('/refresh-files');
})



function refreshFiles() {
	soundfiles = fs.readdirSync(public_path + 'soundfiles');
	soundfiles = soundfiles.filter(item => !(/(^|\/)\.[^\/\.]/g).test(item));

	supercolliderfiles = fs.readdirSync(public_path + 'supercolliderfiles');
	supercolliderfiles = supercolliderfiles.filter(item => !(/(^|\/)\.[^\/\.]/g).test(item));
};

module.exports = router;
