fork{
	SynthDef(\sine, { |freq = 220, amp = 0|
		var sig;
		sig = SinOsc.ar(freq, mul: 0.1*amp);

		Out.ar([0,1], sig);
	}).add;

	s.sync;

	~scale = (Scale.major.semitones + (12*4)) ++ (Scale.major.semitones + (12*5));

	~testsynth = Array.fill(6, {|i|
		Synth(\sine, [freq:~scale[i*2].midicps])
	});

	6.do{ |i|
		OSCdef('testsynth'++i, {|msg, time, addr, recvPort|
			~testsynth[i].set(\amp, msg[1]);
		}, '/raspberrypi/gui/slider'++i, n);

	};
};


