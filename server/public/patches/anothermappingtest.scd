fork{
	// OSCdef(\test, {|msg, time, addr, recvPort| msg[1].post; "/".post; msg[2].postln;}, '/raspberrypi', n); // def style
	// OSCFunc.trace(true); // Turn posting on
	// OSCFunc.trace(false); // Turn posting off
	// "OSCLang test!".postln;
	//main button function mapping
	SynthDef(\mySynth, {| mul = 0.1|
		Out.ar([0, 1], SinOsc.ar(mul: mul));
	}).add;

	s.sync;

	x = Synth(\mySynth);
	// b= Bus.control(s);
	x.map(0, 100);

	"bla!".postln;
};
