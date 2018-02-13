fork{
	SynthDef(\test, {
		var sig;
		sig = SinOsc.ar(In.kr(100).range(100, 500), mul: 0.1);
		Out.ar([0,1], sig);
	}).add;

	s.sync;

	Synth(\test);
};
