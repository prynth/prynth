s.waitForBoot{
	"chime.scd".loadRelative;
	2.wait;
	s.sync;
	"system.scd".loadRelative;
	s.sync;
	"user.scd".loadRelative;
};

