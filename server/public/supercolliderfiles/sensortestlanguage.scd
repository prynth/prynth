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
		var value = msg[1];
		~testsynth.set(\freq, value.linlin(0, 1, 100, 500));
	}, '/raspberrypi/0/0', n);
};
