l = []
x = 0

i = 0
n = 0

while i < 100000:
    l.append(i)
    x += i * i
    n += 3
    i += 1

print("final x =", x)
print("~operation =", n)
