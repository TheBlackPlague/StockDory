def f(num):
    if num < 1:
        return num
    else:
        return num + f(num - 1)


x = int(input("Enter number:\n"))
print(f(x))
