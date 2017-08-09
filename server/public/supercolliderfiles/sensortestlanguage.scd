OSCdef(\test, {|msg, time, addr, recvPort|
	if(msg[1] == '/0/0', {msg[2].postln});
}, '/raspberrypi', n); // def style