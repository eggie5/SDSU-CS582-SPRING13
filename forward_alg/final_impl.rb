require './fwd.rb'

states = ['1', '2', '3', '4']

observations = ['L', 'U', 'U', 'G', 'U', 'U', 'L', 'L', 'L', 'U', 'G', 'L', 'G']

start_probability = {'1'=> 0.25, '2'=> 0.25, '3'=> 0.25, '4'=> 0.25}

transition_probability = {
  '1' => {'1'=> 0.1, '2'=> 0.2, '3'=> 0.5, '4'=> 0.2},
  '2' => {'1'=> 0.4, '2'=> 0.2, '3'=> 0.2, '4'=> 0.2},
  '3' => {'1'=> 0.2, '2'=> 0.2, '3'=> 0.3, '4'=> 0.3},
  '4' => {'1'=> 0.2, '2'=> 0.1, '3'=> 0.3, '4'=> 0.4}
}

emission_probability = {
  '1' => {'G'=> 0.5, 'L'=> 0.3, 'U'=> 0.2 },
  '2' => {'G'=> 0.2, 'L'=> 0.4, 'U'=> 0.4 },
  '3' => {'G'=> 0.4, 'L'=> 0.5, 'U'=> 0.1 },
  '4' => {'G'=> 0.3, 'L'=> 0.3, 'U'=> 0.4 },
}


fwd(observations, states, start_probability, transition_probability,  emission_probability)