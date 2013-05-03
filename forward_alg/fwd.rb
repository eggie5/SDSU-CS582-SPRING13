require 'ap' #pretty print lib


def fwd(obs, states, start_p, trans_p, emit_p)
  steps={}
  #init
  steps['0']={}
  steps['0']['1']= start_p['1'] * emit_p['1'][obs[0]]
  steps['0']['2']= start_p['2'] * emit_p['1'][obs[0]]

  _obs=obs[1..-1] #cut out init step

  #induction
  _obs.each_with_index do |ob, j|
    i=j+1
    prev=i-1

    steps[i.to_s]={}
    states.each do |state|

       steps[i.to_s][state]= ((steps[prev.to_s]["1"] * trans_p["1"][state] )+ 
        (steps[prev.to_s]["2"] * trans_p["2"][state])) * emit_p[state.to_s][ob]
    end
  end

  #term
  i=steps.length
  steps['4']=steps[(i-1).to_s]["1"] + steps[(i-1).to_s]["2"]

  ap steps
end


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
