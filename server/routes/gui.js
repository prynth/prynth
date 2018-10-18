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
var private_path = path.join(__dirname, '../private/');
let public_path = path.join(__dirname, '../public/');


router.get('/', function(req,res,next){
    var configFile = fs.readFileSync(private_path +'config.json');
    var configData = JSON.parse(configFile);
    var hostname = configData.hostname;

	guifiles = fs.readdirSync(public_path + 'guifiles');
	guifiles = guifiles.filter(item => !(/(^|\/)\.[^\/\.]/g).test(item));

	res.render('gui', {
		guifiles: guifiles,
		hostname: hostname,
	});
});


router.post('/guifiles', function (req, res) {
	if(req.body.action === 'load'){
		let filetoload = guifiles[JSON.parse(req.body.fileindex)[0]];
		let currentGUIString = fs.readFileSync((public_path + 'guifiles/' + filetoload), 'utf8');
		let currentGUI = JSON.parse(currentGUIString);
		console.log(currentGUIString);
		res.io.emit('guidraw', currentGUI);
	};

    if(req.body.action === 'save'){
        let filename = req.body.filename;
        let gui = req.body.gui;
        fs.writeFile(public_path + 'guifiles/' + filename, gui, function (err) {
            if(err) {
                res.send('error saving file');
                return console.log(err);
            } else {
                var defaultGUIFile = req.body.filename;
                var configFile = fs.readFileSync(private_path +'config.json');
                var configData = JSON.parse(configFile);

                configData.defaultGUIFile = defaultGUIFile;
                var configJSON = JSON.stringify(configData, null, 2);

                fs.writeFileSync(private_path +'config.json', configJSON);

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

    if(req.body.action === 'loadlastGUI'){
        let configFile = fs.readFileSync(private_path +'config.json');
        let configData = JSON.parse(configFile);
        let defaultGUIFile = configData.defaultGUIFile;
        let currentGUIString = fs.readFileSync((public_path + 'guifiles/' + defaultGUIFile), 'utf8');
        let currentGUI = JSON.parse(currentGUIString);
        // console.log(currentGUIString);
        res.io.emit('guidraw', currentGUI);
    };
});

router.get('/refresh-files', function (req, res) {
	refreshFiles();
	res.io.emit('guifiles', guifiles);
	res.redirect('/');
});

function refreshFiles() {
	guifiles = fs.readdirSync(public_path + 'guifiles');
	guifiles = guifiles.filter(item => !(/(^|\/)\.[^\/\.]/g).test(item));
};


module.exports = router;