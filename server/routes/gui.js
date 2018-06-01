var express = require('express');
var router = express.Router();
let multer = require('multer');
let fs = require('fs');
let path = require('path');

let storage = multer.diskStorage({
	destination: function (req, file, callback) {callback(null, (public_path + 'guifiles'));},
	filename: function (req, file, callback) {callback(null, file.originalname);}
});

let upload = multer({ storage : storage}).array('filename', 10);
var guifiles = [''];
let public_path = path.join(__dirname, '../public/');
var currentGUI = {"objects":[
	{
		"type":"slider",
		"name":"slider0",
		"posx":30,"posy":30,
		"value":0,
		"color":"orange",
		"height":130,
		"width":30
	}
	]};

// var currentGUIString = '{"objects":[]}';
// var currentGUIString = '{"objects":[{"type":"slider","name":"slider0","posx":30,"posy":30,"value":0,"color":"orange","height":130,"width":30}]}';


router.get('/', function(req,res,next){
	// refreshFiles();
	guifiles = fs.readdirSync(public_path + 'guifiles');
	guifiles = guifiles.filter(item => !(/(^|\/)\.[^\/\.]/g).test(item));

	console.log(currentGUI);

	res.render('gui', {
		guifiles: guifiles,
		sentguijson:JSON.stringify(currentGUI)
		// currentGUI
	});

	// res.render('gui', {guifiles: guifiles});

});



router.post('/guifiles', function (req, res) {
	if(req.body.action === 'load'){
		let filetoload = guifiles[JSON.parse(req.body.fileindex)[0]];
		let currentGUIString = fs.readFileSync((public_path + 'guifiles/' + filetoload), 'utf8');
		currentGUI = JSON.parse(currentGUIString);
		res.io.emit('guidraw', currentGUI);
		// res.redirect('/');
	};

	if(req.body.action === 'save'){
		let filename = req.body.filename;
		let gui = req.body.gui;
		fs.writeFile(public_path + 'guifiles/' + filename, gui, function (err) {
			if(err) {
				res.send('error saving file');
				return console.log(err);
			} else {
				res.redirect('refresh-files');
			}
		});
	};

	if(req.body.action === 'delete') {
		let filestodelete = JSON.parse(req.body.fileindex);
		for (i in filestodelete) {
			let fullpath = public_path + 'guifiles/' + guifiles[filestodelete[i]];
			fs.unlinkSync(fullpath);
		}
		res.redirect('refresh-files');
	};
});

router.get('/refresh-files', function (req, res) {
	refreshFiles();
	// guifiles = fs.readdirSync(public_path + 'guifiles');
	// guifiles = guifiles.filter(item => !(/(^|\/)\.[^\/\.]/g).test(item));
	//
	// console.log('files have been refreshed');
	res.io.emit('guifiles', guifiles);
	// res.io.emit('tosoundfiles', soundfiles);
	res.redirect('/');
});

function refreshFiles() {
	guifiles = fs.readdirSync(public_path + 'guifiles');
	guifiles = guifiles.filter(item => !(/(^|\/)\.[^\/\.]/g).test(item));
};


module.exports = router;