s.waitForBoot{
	fork{
		~path = "/home/pi/prynth/server/public/supercolliderfiles/";
		2.wait;
		(~path++"chime.scd").load;
	};
};
