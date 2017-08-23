let express = require('express');
let router = express.Router();
let multer = require('multer');
let fs = require('fs');
let path = require('path');

let storage = multer.diskStorage({
	destination: function (req, file, callback) {callback(null, (public_path + 'soundfiles'));},
	filename: function (req, file, callback) {callback(null, file.originalname);}
});

let upload = multer({ storage : storage}).array('filename', 10);
let soundfiles = [''];
let supercolliderfiles = [''];
let public_path = path.join(__dirname, '../public/');
let tempcode;

refreshFiles();

router.get('/', function(req, res, next) {
	res.render('index', {supercolliderfiles: supercolliderfiles, soundfiles: soundfiles, tempcode: tempcode});
});

router.post('/interpret', function (req, res) {
	res.app.emit('interpret', req.body.code);
	res.sendStatus(200);
});

router.post('/runtemp', function (req, res) {
	res.app.emit('runtemp', tempcode);
	res.sendStatus(200);
});

router.get('/refresh-files', function (req, res) {

	refreshFiles();
	res.io.emit('tosupercolliderfiles', supercolliderfiles);
	res.io.emit('tosoundfiles', soundfiles);

	res.redirect('/');
})

router.post('/supercolliderfiles', function (req, res) {

	if(req.body.action === 'load'){
		let filetoload = supercolliderfiles[JSON.parse(req.body.fileindex)[0]];
		let buffer = fs.readFileSync((public_path + 'supercolliderfiles/' + filetoload), 'utf8');
		tempcode = buffer;
		res.io.emit('toeditor', buffer);
		res.sendStatus(200);
	};

	if(req.body.action === 'save'){
		console.log(req.body.filename);
		console.log(req.body.code);
		fs.writeFile(public_path + 'supercolliderfiles/' + req.body.filename, req.body.code, function (err) {
			if(err) {
				res.send('error saving file');
				return console.log(err);
			} else {
				res.io.emit('toconsole', '\nfile saved');
				// io.sockets.emit('stdout', 'file saved!');
				res.redirect('refresh-files');
			}
		});

	};

	if(req.body.action === 'delete') {
		let filestodelete = JSON.parse(req.body.fileindex);
		for (i in filestodelete) {
			let fullpath = public_path + 'supercolliderfiles/' + supercolliderfiles[filestodelete[i]];
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
		let filestodelete = JSON.parse(req.body.fileindex);
		for (i in filestodelete) {
			let fullpath = public_path + 'soundfiles/' + soundfiles[filestodelete[i]];
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
