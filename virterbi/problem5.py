# wikipedia
def print_dptable(V):
    print "",
    for i in range(len(V)): print "%7d" % i,
    print

    for y in V[0].keys():
        print "%.5s: " % y,
        for t in range(len(V)):
            print "%.7s" % ("%f" % V[t][y]),
        print

def viterbi(obs, states, start_p, trans_p, emit_p):
    V = [{}]
    path = {}

    for y in states:
        V[0][y] = start_p[y] * emit_p[y][obs[0]]
        path[y] = [y]

    for t in range(1,len(obs)):
        V.append({})
        newpath = {}

        for y in states:
            (prob, state) = max([(V[t-1][y0] * trans_p[y0][y] * emit_p[y][obs[t]], y0) for y0 in states])
            V[t][y] = prob
            newpath[y] = path[state] + [y]

        path = newpath

    print_dptable(V)
    (prob, state) = max([(V[len(obs) - 1][y], y) for y in states])
    return (prob, path[state])


def example():
    return viterbi(observations,
                   states,
                   start_probability,
                   transition_probability,
                   emission_probability)

states = ('1', '2', '3', '4')

observations = ('L', 'U', 'U', 'G', 'U', 'U', 'L', 'L', 'L', 'U', 'G', 'L', 'G')

start_probability = {'1': 0.25, '2': 0.25, '3': 0.25, '4': 0.25}

transition_probability = {
   '1' : {'1': 0.1, '2': 0.2, '3': 0.5, '4': 0.2},
   '2' : {'1': 0.4, '2': 0.2, '3': 0.2, '4': 0.2},
   '3' : {'1': 0.2, '2': 0.2, '3': 0.3, '4': 0.3},
   '4' : {'1': 0.2, '2': 0.1, '3': 0.3, '4': 0.4},
   }

emission_probability = {
   '1' : {'G': 0.5, 'L': 0.3, 'U': 0.2 },
   '2' : {'G': 0.2, 'L': 0.4, 'U': 0.4 },
   '3' : {'G': 0.4, 'L': 0.5, 'U': 0.1 },
   '4' : {'G': 0.3, 'L': 0.3, 'U': 0.4 },
   }

				
print example()