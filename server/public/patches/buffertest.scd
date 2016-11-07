fork{
	SynthDef(\help_PlayBuf, {|bufnum = 0|
		var sig = PlayBuf.ar(1, bufnum, BufRateScale.kr(bufnum), loop:1);
		sig = sig * 0.1;
		Out.ar([0,1], sig)
	}).add;
	
	s.sync;
	
	~b = Buffer.read(s, ~sounds.path.files[2].fullPath);
	s.sync;
	
	Synth(\help_PlayBuf, [bufnum: ~b.bufnum]);
}
	