fork{
	~group = Group.new;
	~fxbus = Bus.audio(s, 2);

	80.do{ | i | Bus.new('control', (200+i), 1);};


	// ~sounds = (path: PathName.new("/Users/if/Desktop/sounds"));

	s.sync;

	SynthDef(\fm, { |outbus, note = 0,  octave = 0.5, modfreq = 100,  modindex= 2, amp = 0, toogleff = 1, gain = 0.1|
		var source, env;
		var local, output;
		var toogle = ToggleFF.kr(Schmidt.kr(toogleff, 0.1, 0.7));

		env = EnvGen.kr(Env.adsr(), Schmidt.kr(amp, 0.1, 0.2), doneAction: 0);
		source = PMOsc.ar(((12 + Lag.kr(note) ) * octave.linlin(0,1,2,6).round).midicps, modfreq, modindex) *  env;

		// output = source * 0.1 * Gate.kr(amp, toogle) * gain;
		output = source ! 2 * gain;
		Out.ar(outbus, output);
	}).add;

	SynthDef(\fx, { |inbus|
		var input, processing, output, local;
		input = In.ar(inbus,2);
		// FX here
		Out.ar([0,1], input);
	}).add;

	SynthDef(\linearmapper, {|in, out, minval, maxval, step|
		Out.kr(out, LinLin.kr(In.kr(in), 0, 1, minval, maxval).round(step));
	}).add;

	SynthDef(\exponentialmapper, {|in, out, minval, maxval, step|
		Out.kr(out, LinExp.kr(In.kr(in), 0, 1, minval, maxval).round(step));
	}).add;

	SynthDef(\modulator, {|in, out, minval, maxval, step, select = 0, amp = 0.3, frequency = 0.1, phase = 0|
		var sig;
		sig = Select.kr(select, [
			SinOsc.kr(frequency, phase: phase, mul: amp),
			LFNoise0.kr(frequency, mul: amp),
			Lag.kr(LFNoise0.kr(frequency, mul: amp), 0.3),
		]);
		Out.kr(out, LinExp.kr(In.kr(in) + sig, 0, 1, minval, maxval).round(step));
	}).add;

	SynthDef(\cartopolmapper, {|inx, iny, outsize, outangle|
		var x, y, vectorSize, vectorAngle;
		x = LinLin.kr(In.kr(inx), 0, 1, -1, 1).round(0.1);
		y = LinLin.kr(In.kr(iny), 0, 1, -1, 1).round(0.1);
		vectorSize = hypot(x, y);
		vectorAngle = (atan2(y,x)/pi).linlin(-1, 1, 0, 1);
		Out.kr(outsize, vectorSize);
		Out.kr(outangle, vectorAngle);
	}).add;

	SynthDef(\buttonTrigger, { |businnum, buttonNumber|
		SendTrig.kr(Schmidt.kr(In.kr(businnum), 0.1, 0.7), buttonNumber, 42);
	}).add;

	s.sync;

	//create FMs
	~fm = Array.fill(5, { |i|
		Synth(\fm, [note: Scale.minor.degrees[i], outbus: ~fxbus]);
	});

	s.sync;

	// create fx
	~fx = Synth(\fx, [target: ~group, inbus: ~fxbus], ~group, addAction: 'addToTail');

	//scaling and mapping
	~controllers = Array.fill(5, { |i|
		[
			Synth(\cartopolmapper, [inx: 100+(i*10+0), iny: 100+(i*10+1), outsize: 200+(i*10+0), outangle: 200+(i*10+1)]),
			Synth(\modulator, [in: 100+(i*10+4), out: 200+(i*10+4), minval:1, maxval: 1000, step: 0.01, phase: [0,1,2,3].choose]),
			Synth(\linearmapper, [in: 100+(i*10+5), out: 200+(i*10+5), minval:0, maxval: 10, step: 0.01]),
			Synth(\linearmapper, [in: 100+(i*10+6), out: 200+(i*10+6), minval:0, maxval: 1, step: 0.01]),
		];
	});

	~fm.size.do{|i|
		~fm[i].map("amp", 200+(i*10+0));
		~fm[i].map("octave", 200+(i*10+1));

		// ~granulators[i].map("pos", 200+(i*10+1));
		// ~granulators[i].map("toogleff",100+(i*10+3));
		~fm[i].map("modfreq", 200+(i*10+4));
		~fm[i].map("modindex", 200+(i*10+5));
		// ~granulators[i].map("gain", 200+(i*10+6));

		~controllers[i][1].map("amp", 200+(i*10+6));
	};



	//create button triggers
	~buttons = Array.fill(5, { |i|
		[
			Synth(\buttonTrigger, [businnum: 100 + (i*10+2), buttonNumber:  200 + (i*10+2)]),
			Synth(\buttonTrigger, [businnum: 100 + (i*10+3), buttonNumber:  200 + (i*10+3)]),
			Synth(\buttonTrigger, [businnum: 100 + (i*10+7), buttonNumber:  200 + (i*10+7)])
		]
	});


	//current value of waveform for modulator
	~waveformSelection = [0,0,0,0,0];



	//button callback functions
	OSCFunc({ arg msg, time;
		fork{
			var input = msg[2];
			var buttonNumber = -1;

			if((input - 200 % 10) == 2, {
				buttonNumber = (input-200/10).round;
				buttonNumber.post;

				if (~waveformSelection[buttonNumber] < 2, {
					~waveformSelection[buttonNumber] = 	~waveformSelection[buttonNumber] + 1;
				},{
					~waveformSelection[buttonNumber] = 0;
				});

				~waveformSelection[buttonNumber].postln;

				//~controllers[buttonNumber][1].set(\select, ~waveformSelection[buttonNumber]);
				~controllers[buttonNumber][1].set(\select, 3.rand);
				~controllers[buttonNumber][1].set(\frequency, [0.1, 1, 10, 1000].choose);
				~controllers[buttonNumber][1].set(\phase, [0,1,2,3].choose);

			});

		}
	},'/tr', s.addr);

	"FMs executed!".postln;
};
