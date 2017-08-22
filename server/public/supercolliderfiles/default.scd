s.waitForBoot{
	fork{
		~path = "/home/pi/prynth/server/public/supercolliderfiles/";
		(~path++"chime.scd").load;
	};
};
