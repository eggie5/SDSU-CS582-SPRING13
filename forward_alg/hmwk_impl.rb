require './fwd.rb'

states = ['1', '2']

observations = ['T', 'H', 'H', 'T']

start_probability = {'1' => 1, '2' => 0}

transition_probability = {
  '1' => {'1'=> 0.2, '2'=> 0.8},
  '2' => {'1'=> 0.6, '2'=> 0.4},
}

emission_probability = {
  '1' => {'H'=> 0.7, 'T'=> 0.3 },
  '2' => {'H'=> 0.4, 'T'=> 0.6 },
}

fwd(observations, states, start_probability, transition_probability,  emission_probability)


# {
#                 "0" => {
#         "1" => 0.3,
#         "2" => 0.0
#     },
#                 "1" => {
#         "1" => 0.041999999999999996,
#         "2" => 0.096
#     },
#                 "2" => {
#         "1" => 0.0462,
#         "2" => 0.028800000000000006
#     },
#                 "3" => {
#         "1" => 0.007956,
#         "2" => 0.029088
#     },
#     "4 termination" => 0.037044
# }
