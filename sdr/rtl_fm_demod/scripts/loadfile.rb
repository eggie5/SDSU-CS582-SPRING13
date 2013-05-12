f = File.new("../../capture.bin")
q=[]
begin
while line = f.readbyte
  q << (line-127)
  f.readbyte #ignore i
end
rescue 
end
f.close

p q[0..50]

w=File.open('reals', 'w')

w.puts(q)
