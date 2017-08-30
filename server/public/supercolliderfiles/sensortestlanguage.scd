fork{
	SynthDef(\test, { |freq = 220|
		var sig;
		sig = SinOsc.ar(freq, mul: 0.1);
		Out.ar([0,1], sig);
	}).add;

	s.sync;

	~testsynth = Synth(\test);

	s.sync;

	OSCdef(\test, {|msg, time, addr, recvPort|
		var value;
		if (msg[1] == '/0/0', {value = msg[2]});
		~testsynth.set(\freq, value.linlin(0, 1, 100, 500));
	}, '/raspberrypi', n);
};
