# Helps visualize the steps of Viterbi.
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

    # Initialize base cases (t == 0)
    for y in states:
        V[0][y] = start_p[y] * emit_p[y][obs[0]]
        path[y] = [y]

    # Run Viterbi for t > 0
    for t in range(1,len(obs)):
        V.append({})
        newpath = {}

        for y in states:
            (prob, state) = max([(V[t-1][y0] * trans_p[y0][y] * emit_p[y][obs[t]], y0) for y0 in states])
            V[t][y] = prob
            newpath[y] = path[state] + [y]

        # Don't need to remember the old paths
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

states = ('1', '2')

observations = ('T', 'H', 'T')

start_probability = {'1': 0.5, '2': 0.5}

transition_probability = {
   '1' : {'1': 0.3, '2': 0.7},
   '2' : {'1': 0.7, '2': 0.3},
   }

emission_probability = {
   '1' : {'T': 0.5, 'H': 0.5 },
   '2' : {'T': 0.5, 'H': 0.5 },
   }

				
print example()