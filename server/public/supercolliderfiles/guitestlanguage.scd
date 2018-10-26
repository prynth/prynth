fork{
	SynthDef(\sine, { |freq = 220, amp = 0|
		var sig;
		sig = SinOsc.ar(freq, mul: 0.05*amp);
		Out.ar([0,1], sig);
	}).add;

	s.sync;

	~scale = [52, 55, 59, 62, 65, 69, 72, 76];

	~testsynth = Array.fill(8, {|i|
		Synth(\sine, [freq: ~scale[i].midicps]);
	});

	8.do{ |i|
		OSCdef('testsynth'++i, {|msg, time, addr, recvPort|
			~testsynth[i].set(\amp, msg[1]);
		}, '/raspberrypi/gui/slider'++i);
	};
};
