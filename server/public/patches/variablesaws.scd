fork{

	SynthDef(\cartopolmapper, {|inx, iny, outamp, outangle|
		var x, y, vectorSize, vectorAngle;
		x = LinLin.kr(In.kr(inx), 0, 1, -1, 1).round(0.1);
		y = LinLin.kr(In.kr(iny), 0, 1, -1, 1).round(0.1);
		vectorSize = hypot(x, y);
		vectorAngle = (atan2(y,x)/pi).linlin(-1, 1, 0, 1);
		Out.kr(outamp, vectorSize);
		Out.kr(outangle, vectorAngle);
	}).add;

	s.sync;

	5.do{|i|
		var xControl = 0;
		var yControl = 1;
		Synth(\cartopolmapper, [inx: 100+(i*10+xControl), iny: 100+(i*10+yControl), outamp: 200+(i*10+xControl), outangle: 200+(i*10+yControl)]);
	};

	s.sync;

	play{

		b = Array.fill(5, { |i|
			Buffer.alloc(s, 44100 * 2, 2);
		});

		s = Mix.ar(VarSaw.ar(
			freq: (In.kr(104).linlin(0,1,0,11).round(1) + [0,2,3,5,7] + (In.kr([105,115, 125, 135, 145]).linlin(0,1, 2, 8).round(4/12)*12)).midicps.lag(0.1),
			width: In.kr([201, 211, 221, 231, 241]).lag(10),
			mul: In.kr([200, 210, 220, 230, 240]).lag(1),
		) * 0.2 *
		In.kr([106, 116, 126,136,146]); //gain
		)!2;

		// 5.do { |i| t = DelayL.ar(RLPF.ar(t, Rand(100, 3000), 1), 1, In.kr(114).lag(0.1) / (2**i), 0.9, t * In.kr(124).linlin(0,1,0,2))};
		// 5.do { |i| t = PingPong.ar(b[i].bufnum, RLPF.ar(t, Rand(100, 3000), 0.2), In.kr(114).linlin(0,1,0,2), In.kr(124).linlin(0,1, 0, 0.1), rotate:0)} ;
		// 5.do { |i| t = PingPong.ar(b[i].bufnum, RLPF.ar(t[], Rand(100, 3000), 0.2), In.kr(114).linlin(0,1,0,2), 0.1, rotate:0)} ;

		5.do({ |i|
			(i*10+200).asBus.set(0);
		});

		d = Array.fill(5, { |i|
			PingPong.ar(b[i].bufnum, RLPF.ar(s, Rand(100, 3000), 0.02), In.kr(114) / (2**i), In.kr(124), 1);
		});


		s+d;
		// Splay.ar((s+d), 1);
		// Splay.ar(s, 1);

		//DelayL.ar( t, 3, In.kr(114).linlin(0,1,0.0001, 3).lag(0.1), 0.7, t * 0.4);

		// DelayL.ar( t, 3, 0, 0.7, t * In.kr(124).linlin(0,1,0,2));

		//FreeVerb.ar(y, 1, 0.95, 0.2);

	};

	"Saw drones executed!".postln;
};
