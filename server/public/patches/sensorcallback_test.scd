fork{


SynthDef("help-SendTrig",{
    SendTrig.kr(Impulse.kr(5.0),0,In.kr(101));
}).send(s);

s.sync;
// register to receive this message
o = OSCFunc({ arg msg, time;
   msg.postln;
},'/tr', s.addr);

s.sync;
Synth("help-SendTrig");


}