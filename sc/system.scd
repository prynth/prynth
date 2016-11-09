fork{

~sounds = (
	path: PathName.new("/home/pi/prynth/server/public/sounds/")
);

80.do{ | i |
		// i.postln;
		Bus.new('control', (100+i), 1);
};

s.latency = 0.05;

};

