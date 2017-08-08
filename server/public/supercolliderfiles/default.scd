s.waitForBoot{
	fork{
		~path = "/home/pi/prynth/server/public/supercolliderfiles/";
		2.wait;
		(~path++"sinetest.scd").load;
	};
};
