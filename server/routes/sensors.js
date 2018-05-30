var express = require('express');
var router = express.Router();
var fs = require('fs');
var path = require('path');
var public_path = path.join(__dirname, '../public/');
var private_path = path.join(__dirname, '../private/');

router.get('/', function(req, res, next) {
	var configFile = fs.readFileSync(private_path +'config.json');
	var configData = JSON.parse(configFile);
	var hostname = configData.hostname;

	res.render('sensors', {
		hostname: hostname
	});
});

router.post('/savesensors', function (req, res) {
	var sensorJSON = req.body.json;
	fs.writeFile(public_path +'config/sensors.json', sensorJSON, function () {
		res.app.emit('recallsensors');
	});
});

router.post('/resetsensors', function (req, res) {
	res.app.emit('resetsensors');
	res.sendStatus(200);
});

router.post('/recallsensors', function (req, res) {
	res.app.emit('recallsensors');
	res.sendStatus(200);
});

module.exports = router;
