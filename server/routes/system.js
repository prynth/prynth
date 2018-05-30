var express = require('express');
var router = express.Router();
var exec = require('child_process').exec;
var fs = require('fs');
var path = require('path');
var private_path = path.join(__dirname, '../private/');

router.get('/', function(req, res, next) {
	var configFile = fs.readFileSync(private_path +'config.json');
	var configData = JSON.parse(configFile);

	var defaultSCFile = configData.defaultSCFile;
	var defaultssid = configData.wifinetworks[0];
	var hostname = configData.hostname;
	var sensordatatarget = configData.sensorDataTarget;
	var audiodevice = configData.jack.device;
	var vectorsize = configData.jack.vectorSize;
	var samplerate = configData.jack.sampleRate;
	var usb1 = configData.jack.usb1;

	res.render('system', {
		defaultSCFile: defaultSCFile,
		defaultssid: defaultssid,
		hostname: hostname,
		sensordatatarget: sensordatatarget,
		audiodevice: audiodevice,
		vectorsize: vectorsize,
		samplerate: samplerate,
		usb1: usb1
	});
});

router.post('/setwifi', function (req, res) {
	var networkname = req.body.networkname;
	var networkpass = req.body.networkpass;

	var configFile = fs.readFileSync(private_path +'config.json');
	var configData = JSON.parse(configFile);

	configData.wifinetworks = ([networkname, networkpass]);
	var configJSON = JSON.stringify(configData, null, 2);

	fs.writeFileSync(private_path +'config.json', configJSON);

	var command =
	'sudo ip link set wlan0 down && '+
	'sudo wpa_cli -i wlan0 add_network 0 && sudo wpa_cli -i wlan0 set_network 0 ssid '
	+'\'\"'+networkname+'\"\''+
	' && sudo wpa_cli -i wlan0 set_network 0 psk '
	+'\'\"'+networkpass+'\"\''
	+' && sudo wpa_cli -i wlan0 enable_network 0 && sudo wpa_cli -i wlan0 save_config '
	+'&& sudo ip link set wlan0 up'
	;

	var child = exec(command, function (error, stdout, stderr) {
		console.log(stdout);
		res.redirect('/system');
	});

})


router.post('/sethostname', function (req, res) {
	var hostname = req.body.hostname;

	var configFile = fs.readFileSync(private_path +'config.json');
	var configData = JSON.parse(configFile);

	configData.hostname = hostname;
	var configJSON = JSON.stringify(configData, null, 2);

	fs.writeFileSync(private_path +'config.json', configJSON);

	var command1 = 'sudo sed -i "s/\\(127.0.1.1 *\\).*/\\1\\t'+hostname+'/" /etc/hosts';
	var command2 = 'sudo sed -i "s/^.*/'+hostname+'/" /etc/hostname';

	var child1 =  exec(command1, function (error, stdout, stderr) {
		var child2 = exec(command2, function (error, stdout, stderr) {
			res.redirect('/system');
		});
	});

});

router.post('/setdefaultscfile', function (req, res) {
	var defaultSCFile= req.body.defaultSCFile;

	var configFile = fs.readFileSync(private_path +'config.json');
	var configData = JSON.parse(configFile);

	configData.defaultSCFile = defaultSCFile;
	var configJSON = JSON.stringify(configData, null, 2);

	fs.writeFileSync(private_path +'config.json', configJSON);
});

router.post('/setsensordatatarget', function (req, res) {
	var sensordatatarget= req.body.sensorDataTarget;

	var configFile = fs.readFileSync(private_path +'config.json');
	var configData = JSON.parse(configFile);

	configData.sensorDataTarget= sensordatatarget;
	var configJSON = JSON.stringify(configData, null, 2);

	fs.writeFileSync(private_path +'config.json', configJSON);

	res.redirect('/system');

});

router.post('/setjack', function (req, res) {
	var jackDevice= req.body.device;
	var jackVectorSize = req.body.vectorSize;
	var jackSampleRate = req.body.sampleRate;
	var usb1 = req.body.usb1;
	var command;


	var configFile = fs.readFileSync(private_path +'config.json');
	var configData = JSON.parse(configFile);

	configData.jack = {"device": jackDevice, "vectorSize": jackVectorSize, "sampleRate": jackSampleRate, "usb1": usb1};

	var configJSON = JSON.stringify(configData, null, 2);
	fs.writeFileSync(private_path +'config.json', configJSON);

	if (usb1 == 'true'){
		command = 'echo "dwc_otg.speed=1 dwc_otg.lpm_enable=0 console=tty1 root=/dev/mmcblk0p2 rootfstype=ext4 elevator=deadline fsck.repair=yes rootwait" |sudo tee /boot/cmdline.txt';
		} else
		{
		command = 'echo "dwc_otg.lpm_enable=0 console=tty1 root=/dev/mmcblk0p2 rootfstype=ext4 elevator=deadline fsck.repair=yes rootwait" |sudo tee /boot/cmdline.txt';
		};

	exec(command, function (error, stdout, stderr) {
		console.log(stdout);
	});

});

module.exports = router;
