fork{
	~group = Group.new;
	~fxbus = Bus.audio(s, 2);
	
	
	
	80.do{ | i | Bus.new('control', (200+i), 1);};
	// ~sounds = (path: PathName.new("/Users/if/Desktop/sounds"));

	s.sync;

	SynthDef(\granulator, { |outbus, density = 1, freq = 1, amp = 1,  bufnum=0, pos = 0.5, toogleff = 1, gain = 0|
		var source, env;// use mouse x to control panning
		var local, output;
		var toogle = ToggleFF.kr(Schmidt.kr(toogleff, 0.1, 0.7));

		source = GrainBuf.ar(
			2, //number of channels
			Impulse.kr(density+LFNoise0.kr().range(-0.01, 0.01)*2), //trigger
			1/(density/4), //dur
			bufnum, //buffer number
			//Gate.kr((LinLin.kr(In.kr(freq), 0, 1, 4, 10).floor * 12).midiratio, toogle), //playback rate
			freq, //playback rate
			// pos, //position in file
			(Gate.kr(pos, toogle) + LFNoise0.kr().range(-0.01, 0.01)).clip(0, 1),
			2, //interpolation mode
			LFNoise0.kr(LFNoise0.kr(10).range(1,10)).range(-0.4, 0.4), //pan
			-1, // envelope (default -1 uses Hanning)
			20
		);

		output = source * 0.1 * Gate.kr(amp, toogle) * gain;
		Out.ar(outbus, output);
	}).add;

	SynthDef(\fx, { |inbus|
		var input, processing, output, local;
		input = In.ar(inbus,2);
		// local = LocalIn.ar(2) + input; // read feedback, add to source
		// local = [DelayN.ar(local[0], 0.2, 0.2), DelayN.ar(local[1], 0.2, 0.15)]; // delay sound
		// LocalOut.ar(local.reverse * 0.5);	// reverse channels to give ping pong effect, apply decay factor + feedback ammount
		//output = FreeVerb2.ar(local[0],local[1], mix: 0, room: 0.3, damp: 0.8, mul: 0.9, add: 0);
		output = FreeVerb2.ar(input[0],input[1], mix: 0.5, room: 0.8, damp: 0.8, mul: 1, add: 0);
		Out.ar([0,1], output);
		//Out.ar([0,1], input);
	}).add;

	SynthDef(\linearmapper, {|in, out, minval, maxval, step|
		Out.kr(out, LinLin.kr(In.kr(in), 0, 1, minval, maxval).round(step));
	}).add;
	
	SynthDef(\exponentialmapper, {|in, out, minval, maxval, step|
		Out.kr(out, LinExp.kr(In.kr(in), 0, 1, minval, maxval).round(step));
	}).add;

	SynthDef(\cartopolmapper, {|inx, iny, outamp, outangle|
		var x, y, vectorSize, vectorAngle;
		x = LinLin.kr(In.kr(inx), 0, 1, -1, 1).round(0.1);
		y = LinLin.kr(In.kr(iny), 0, 1, -1, 1).round(0.1);
		vectorSize = hypot(x, y);
		vectorAngle = (atan2(y,x)/pi).linlin(-1, 1, 0, 1);
		Out.kr(outamp, vectorSize);
		Out.kr(outangle, vectorAngle);
	}).add;

	SynthDef(\buttonTrigger, { |businnum, buttonNumber|
		SendTrig.kr(Schmidt.kr(In.kr(businnum), 0.1, 0.7), buttonNumber, 42);
	}).add;

	s.sync;

	//load buffers
	~grainbuffers = Array.fill(5, { |i|
		Buffer.read(s, ~sounds.path.files[i].fullPath, 0, s.sampleRate * 4);
	});

	s.sync;

	//normalize buffers
	~grainbuffers.size.do({ |i|	~grainbuffers[i].normalize});
	s.sync;

	//create granulators
	~granulators= Array.fill(5, { |i|
		Synth(\granulator, [outbus: ~fxbus, bufnum: ~grainbuffers[i].bufnum]);
	});

	s.sync;

	// create fx
	~fx = Synth(\fx, [target: ~group, inbus: ~fxbus], ~group, addAction: 'addToTail');

	//scaling and mapping
	~granulators.size.do{|i|
		var xControl = 0;
		var yControl = 1;
		var toogleControl = 3;
		var impControl = 4;
		var freqControl = 5;
		var gainControl = 6;

		Synth(\cartopolmapper, [inx: 100+(i*10+xControl), iny: 100+(i*10+yControl), outamp: 200+(i*10+xControl), outangle: 200+(i*10+yControl)]);
		Synth(\linearmapper, [in: 100+(i*10+impControl), out: 200+(i*10+impControl), minval:0.1, maxval: 10, step: 0.01]);
		Synth(\linearmapper, [in: 100+(i*10+freqControl), out: 200+(i*10+freqControl), minval:0, maxval: 2, step: 0.01]);
		Synth(\linearmapper, [in: 100+(i*10+gainControl), out: 200+(i*10+gainControl), minval:0, maxval: 2, step: 0.01]);

		~granulators[i].map("amp", 200+(i*10+xControl));
		~granulators[i].map("pos", 200+(i*10+yControl));
		~granulators[i].map("toogleff",100+(i*10+toogleControl));
		~granulators[i].map("density", 200+(i*10+impControl));
		~granulators[i].map("freq", 200+(i*10+freqControl));
		~granulators[i].map("gain", 200+(i*10+gainControl));
	};

	//create button triggers
	~buttons = Array.fill(5, { |i|
		[
			Synth(\buttonTrigger, [businnum: 100 + (i*10+2), buttonNumber:  200 + (i*10+2)]),
			Synth(\buttonTrigger, [businnum: 100 + (i*10+3), buttonNumber:  200 + (i*10+3)]),
			Synth(\buttonTrigger, [businnum: 100 + (i*10+7), buttonNumber:  200 + (i*10+7)])
		]
	});

	//button callback functions
	OSCFunc({ arg msg, time;
		fork{
			var input = msg[2];
			var buttonNumber = -1;
			if((input - 200 % 10) == 2, {
				buttonNumber = (input-200/10).round;
				buttonNumber.post;
				~grainbuffers[buttonNumber].free;
				~newsound = ~sounds.path.files[~sounds.path.files.size.rand].fullPath;
				~newsound.postln;
				s.sync;
				~grainbuffers[buttonNumber] = Buffer.read(s,~newsound, 0, s.sampleRate * 4);
			});
		}
	},'/tr', s.addr);

	"granulators executed!".postln;
};
