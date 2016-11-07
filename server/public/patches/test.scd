fork{
//	SynthDef(\test, {
//		var sig;
//		sig = SinOsc.ar(150, mul: In.kr(100));
//		Out.ar([0,1], sig);
//	}).add;
//    s.sync;
//	Synth(\test);
  ~b = Buffer.read(s, ~sounds.path.files[0].fullPath);
  
  s.sync;

  SynthDef(\help_PlayBuf, {|bufnum = 0|
    var sig = PlayBuf.ar(1, bufnum, BufRateScale.kr(bufnum), loop:1);
	sig = sig * 0.1;
    Out.ar([0,1], sig)
  }).add;
  
  s.sync;
  Synth(\help_PlayBuf, [bufnum: ~b.bufnum]);
            
//play{
//	a=SinOsc.ar(LFNoise0.ar(In.kr(100).range(0,10)).range(100,1e4),0,0.05)*Decay.kr(Dust.kr(In.kr(100).range(0, 0.2)));
//	GVerb.ar(a*LFNoise1.ar(40),299,400,0.2,0.5,50,0,0.2,0.9)
//};

//s.sync;
//Ndef(\a).fadeTime_(9).play;fork{loop{Ndef(\a,{SinOsc.ar(9.collect{[6e3.rand,6e3.rand]},0,0.1).sum*Saw.ar(rrand([0,1],9.9)).max(0)});3.wait}}

//play{x=LFSaw.ar([0.3,0.6])*LFSaw.ar([1,1.5])*SinOsc.ar([
//    In.kr(100).range(80, 220),
//   In.kr(100).range(220, 440)
//]*LFPulse.kr(0.12).range(1,2));CombC.ar(x,0.5,0.5,9,0.5,x)/12}

}