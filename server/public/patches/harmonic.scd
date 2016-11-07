fork{
	~group = Group.new;
	80.do{ | i | Bus.new('control', (200+i), 1);};

	s.sync;

	SynthDef(\drone, { |outbus, freq = 100, width, amp, gain, filterfreq, filterrq|

		var source, osc, out, sub;
		sub = SinOsc.ar(freq/2);
		// osc = Pulse.ar(freq, width, mul:0.9) * AmpComp.kr(freq, 48.midicps);
		osc = Pulse.ar(freq,  LinLin.kr(In.kr(width), -3.14, 3.14, 0, 0.5), mul:0.9);
		source = (sub + osc) * Lag2.kr(In.kr(amp)) * In.kr(gain);
		out = Resonz.ar(source, Lag2.kr(In.kr(filterfreq)), In.kr(filterrq));
		// Out.ar(outbus, out);
		Out.ar(outbus,Pan2.ar(out, LFNoise2.kr(1).range(-0.8, 0.8)));

	}).add;

	SynthDef(\fx, { |inbus|
		var input, processing, output, local;
		input = In.ar(inbus,2);
		local = LocalIn.ar(2) + input; // read feedback, add to source
		local = [DelayN.ar(local[0], 0.2, 0.2), DelayN.ar(local[1], 0.2, 0.15)]; // delay sound
		LocalOut.ar(local.reverse * 0.5);	// reverse channels to give ping pong effect, apply decay factor + feedback ammount
		output = FreeVerb2.ar(local[0],local[1], mix: 0, room: 0.3, damp: 0.8, mul: 0.9, add: 0);
		//Out.ar([0,1], output);
		Out.ar([0,1], input);

	}).add;

	SynthDef(\linearmapper, {|in, out, minval, maxval, step|
		Out.kr(out, LinLin.kr(In.kr(in), 0, 1, minval, maxval).round(step));
	}).add;

	SynthDef(\cartopolmapper, {|inx, iny, outamp, outangle|
		var x, y, vectorSize, vectorAngle;
		x = LinLin.kr(In.kr(inx), 0, 1, -1, 1).round(0.1);
		y = LinLin.kr(In.kr(iny), 0, 1, -1, 1).round(0.1);
		vectorSize = hypot(x, y);
		vectorAngle = atan2(y,x);
		Out.kr(outamp, vectorSize);
		Out.kr(outangle, vectorAngle);
	}).add;

	SynthDef(\buttonTrigger, { |businnum, buttonNumber|
		SendTrig.kr(Schmidt.kr(In.kr(businnum), 0.1, 0.7), buttonNumber, 42);
	}).add;

	s.sync;
	~scale = Scale.minor;
	~scale.degrees;
	~root = 12;

	// create harmonics
	~harmonics= Array.fill(5, { |i|
		Synth(\drone, [
			outbus: ~fxbus,
			freq: (~scale[i] + ~root +24).midicps,
			amp:200+(i*10+0),
			width:200+(i*10+1),
			toogleff: 100+(i*10+3),
			filterrq:  100+(i*10+4),
			filterfreq: 200+(i*10+5),
			gain: 200+(i*10+6),
		]);
	});

	// Synth(\drone, [
	// 	outbus: ~fxbus,
	// 	freq: (~scale[0] + ~root +24).midicps,
	// 	amp:200+(0*10+0),
	// 	width:200+(0*10+1),
	// 	// toogleff: 100+(i*10+3),
	// 	foo: 200+(0*10+4),
	// 	filterfreq: 200+(0*10+5),
	// 	gain: 200+(0*10+6),
	// ]);

	//create button triggers
	~buttons = Array.fill(5, { |i|
		[
			Synth(\buttonTrigger, [businnum: 100 + (i*10+2), buttonNumber:  200 + (i*10+2)]),
			Synth(\buttonTrigger, [businnum: 100 + (i*10+3), buttonNumber:  200 + (i*10+3)]),
			Synth(\buttonTrigger, [businnum: 100 + (i*10+7), buttonNumber:  200 + (i*10+7)])
		]
	});

	s.sync;

	// create fx
	~fx = Synth(\fx, [target: ~group, inbus: ~fxbus], ~group, addAction: 'addToTail');

	//mapping
	5.do{|i|
		var xControl = 0;
		var yControl = 1;
		var frqControl = 4;
		var ffreqControl = 5;
		var gainControl = 6;

		Synth(\cartopolmapper, [inx: 100+(i*10+xControl), iny: 100+(i*10+yControl), outamp: 200+(i*10+xControl), outangle: 200+(i*10+yControl)]);
		Synth(\linearmapper, [in: 100+(i*10+frqControl ), out: 200+(i*10+frqControl), minval:0, maxval: 1, step: 0.001]);
		Synth(\linearmapper, [in: 100+(i*10+ffreqControl), out: 200+(i*10+ffreqControl), minval:0, maxval: 1000, step: 0.001]);
		Synth(\linearmapper, [in: 100+(i*10+gainControl), out: 200+(i*10+gainControl), minval:0, maxval: 1, step: 0.001]);
	};

	//button callback functions
	OSCFunc({ arg msg, time;


	},'/tr', s.addr);

	"harmonic executed!".postln;
	//
};
