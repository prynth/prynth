fork {

	// play{Out.kr(112, SinOsc.kr(1, mul:0.5, add: 0.5))};
	// Stethoscope.new(s, 1, 112, rate: 'control');
	// Stethoscope.new(s, 1, 212, rate: 'control');

	SynthDef(\buttonTrigger, { |businnum, buttonNumber|
		SendTrig.kr(Schmidt.kr(In.kr(businnum), 0.1, 0.7), buttonNumber, 42);
	}).add;
	s.sync;

	//create button triggers
	~buttons = Array.fill(5, { |i|
		[
			Synth(\buttonTrigger, [businnum: 100 + (i*10+2), buttonNumber:  200 + (i*10+2)]),
			Synth(\buttonTrigger, [businnum: 100 + (i*10+3), buttonNumber:  200 + (i*10+3)]),
			Synth(\buttonTrigger, [businnum: 100 + (i*10+7), buttonNumber:  200 + (i*10+7)])
		]
	});

	//callback functions
	o = OSCFunc({ arg msg, time;
		// msg[2].postln;
		if (msg[2] == 202){(note: 12, dur: 0.5).play};
	},'/tr', s.addr);
}

// //Detect changes

// {
// 	var changingSignal = LFNoise0.ar(1000);
// 	var changed = Changed.ar(changingSignal);
// 	[changingSignal, changed]
// }.plot
//
// //coco xixi fafa
// //Schmidt trigger
// { Schmidt.kr(MouseX.kr(0, 1), 0.2, 0.8)}.scope; // try it with the cursor
//
//
// // Toogle Flip Flop
// (
// play({
// 	SinOsc.ar((ToggleFF.ar(Dust.ar(XLine.kr(1,1000,60))) * 400) + 800, 0, 0.1)
// }))
//
//
// //send trigger from server to client
// (
// SynthDef("help-SendTrig",{
// 	SendTrig.kr(Dust.kr(1.0),0,0.9);
// }).send(s);
//
// // register to receive this message
// o = OSCFunc({ arg msg, time;
// 	[time, msg].postln;
// },'/tr', s.addr);
//
// )
