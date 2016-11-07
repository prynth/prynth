fork{

	OSCdef(\test, {|msg, time, addr, recvPort| msg[1].post; "/".post; msg[2].postln;}, '/raspberrypi', n); // def style
	// OSCFunc.trace(true); // Turn posting on
// OSCFunc.trace(false); // Turn posting off
	 "OSCLang test!".postln;
};
