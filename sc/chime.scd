play{
	Out.ar([0,1],
		LPF.ar(
			(
				(
					Pulse.ar(50,
						width:
						EnvGen.kr(Env.linen(0.5, 1, 1)).range(0.3, 0.7)
					)
				)  *
				EnvGen.kr(
					Env.linen(0.5, 1, 1),
					doneAction: 2)
			),
			EnvGen.kr(Env.linen(0.5, 1, 1)).range(50, 300) );
	)
}