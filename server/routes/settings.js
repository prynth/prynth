let express = require('express');
let router = express.Router();
// let exec = require('child_process').exec;
let fs = require('fs');
let path = require('path');
let private_path = path.join(__dirname, '../private/');

// let jsonfile = require('jsonfile');

// let configfile = '/Users/if/Desktop/prynthtemp/server/private/config.json';
// let configbuffer = require(configfile);

router.get('/', function(req, res, next) {
	var configFile = fs.readFileSync(private_path +'config.json');
	var configData = JSON.parse(configFile);
	var defaultSCFile = configData.defaultSCFile;
	// var hostname = configData.hostname;

	res.render('settings', {defaultSCFile: defaultSCFile});
});

router.post('/setwifi', function (req, res) {
	var networkname = req.body.networkname;
	var networkpass = req.body.networkpass;

	var buffer = "";

	//CHANGE TO RELATIVE PATHS!!!
	var configFile = fs.readFileSync(private_path +'config.json');
	var configData = JSON.parse(configFile);

	// console.log(configData);
	configData.wifinetworks.push([networkname, networkpass]);
	var configJSON = JSON.stringify(configData, null, 2);

	// console.log(configJSON);
	fs.writeFileSync(private_path +'config.json', configJSON);

	buffer += "country=GB\n";
	buffer += "ctrl_interface=DIR=/var/run/wpa_supplicant GROUP=netdev\n";
	buffer += "update_config=1\n\n";
	buffer += "network={\n";
	buffer += '\tssid="' + networkname +'"\n';
	buffer += '\tpassword="' + networkpass+'"\n';
	buffer += "}";

	// fs.writeFile("/Users/if/Desktop/myconf.conf", buffer, function (err) {
	// 	if(err) {
	// 		res.send('error saving file');
	// 		return console.log(err);
	// 	}
	// 	else {
	// 		// var child = exec('atom /Users/if/Desktop/myconf.conf', function (error, stdout, stderr) {
	// 		// 	console.log(stdout);
	// 		// });
	//
	// 		res.redirect('/settings');
	// 	}
	// });
})

router.post('/setdefaultscfile', function (req, res) {
	var defaultSCFile= req.body.defaultSCFile;
	console.log(defaultSCFile);

	//CHANGE TO RELATIVE PATHS!!!
	var configFile = fs.readFileSync(private_path +'config.json');
	var configData = JSON.parse(configFile);

	configData.defaultSCFile = defaultSCFile;
	var configJSON = JSON.stringify(configData, null, 2);

	fs.writeFileSync(private_path +'config.json', configJSON);
	//
	// buffer += "country=GB\n";
	// buffer += "ctrl_interface=DIR=/var/run/wpa_supplicant GROUP=netdev\n";
	// buffer += "update_config=1\n\n";
	// buffer += "network={\n";
	// buffer += '\tssid="' + networkname +'"\n';
	// buffer += '\tpassword="' + networkpass+'"\n';
	// buffer += "}";
	//
	// fs.writeFile("/Users/if/Desktop/myconf.conf", buffer, function (err) {
	// 	if(err) {
	// 		res.send('error saving file');
	// 		return console.log(err);
	// 	}
	// 	else {
	// 		// var child = exec('atom /Users/if/Desktop/myconf.conf', function (error, stdout, stderr) {
	// 		// 	console.log(stdout);
	// 		// });
	//
	// 		res.redirect('/settings');
	// 	}
	// });
})


module.exports = router;
