var express = require('express');
var router = express.Router();
var multer = require('multer');
var fs = require('fs');
var path = require('path');

var storage = multer.diskStorage({
	destination: function (req, file, callback) {callback(null, (public_path + 'guifiles'));},
	filename: function (req, file, callback) {callback(null, file.originalname);}
});

var guifiles = [''];
var private_path = path.join(__dirname, '../private/');
var public_path = path.join(__dirname, '../public/');


router.get('/', function(req,res){
   let configFile = fs.readFileSync(private_path +'config.json');
   let configData = JSON.parse(configFile);
   let hostname = configData.hostname;

   guifiles = fs.readdirSync(public_path + 'guifiles');
   guifiles = guifiles.filter(item => !(/(^|\/)\.[^\/\.]/g).test(item));

   res.render('gui', {
       guifiles: guifiles,
       hostname: hostname,
   });
});

router.post('/guifiles', function (req, res) {
	if(req.body.action === 'load'){
		let filename = guifiles[JSON.parse(req.body.fileindex)[0]];
		let guiBuffer = fs.readFileSync((public_path + 'guifiles/' + filename), 'utf8');
		res.io.emit('guidraw', JSON.parse(guiBuffer));
        res.sendStatus(200);
    };

    if(req.body.action === 'save'){
        let filename = req.body.filename;
        let gui = req.body.gui;
        fs.writeFile(public_path + 'guifiles/' + filename, gui, function (err) {
            if(err) {
                res.send('error saving file');
                return console.log(err);
            } else {
                let defaultGUIFile = req.body.filename;
                let configFile = fs.readFileSync(private_path +'config.json');
                let configData = JSON.parse(configFile);

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
        let guiBuffer = fs.readFileSync((public_path + 'guifiles/' + defaultGUIFile), 'utf8');
        res.io.emit('guidraw', JSON.parse(guiBuffer));
        res.sendStatus(200);
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