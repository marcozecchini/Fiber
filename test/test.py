import subprocess 
import sys
import matplotlib.pyplot as plt
import numpy as np


while(True):
	choice = raw_input("Digit something among \"fiber\", \"user\", \"plot\" and \"exit\": ")
	if (choice == "fiber"):
		file_fiber = open("output_fiber.txt", "w")
		for i in range (0,15):
			res = subprocess.Popen(["../2018-fibers/test", str((i+1)*32)], stdout = subprocess.PIPE)
			time = res.stdout.read().split("\n")[2].split(":")[1].strip()
			file_fiber.write(time+"\n")
		file_fiber.close()
		
	elif (choice == "user"):
		file_fiber = open("output_fiber_usr.txt", "w")
		for i in range (0,15):
			res = subprocess.Popen(["../2018-fibers/test", str((i+1)*32)], stdout = subprocess.PIPE)
			time = res.stdout.read().split("\n")[2].split(":")[1].strip()
			file_fiber.write(time+"\n")
		file_fiber.close()
	elif (choice == "plot"):
		f1 = open("output_fiber.txt", "r")
		f2 = open("output_fiber_usr.txt", "r")
		list1 = list()
		list2 = list()
		
		for n in f1:
			list1.append(float(n))
		for n in f2:
			list2.append(float(n))
		print(list1)
		plt.xlabel("Iteration")
		plt.ylabel("Time per fiber")
		plt.plot(range(0, len(list1)), list1, marker = "o", label="Kernel")
		plt.plot(range(0, len(list2)), list2, marker = "o", label="User")
		plt.legend()
		plt.show()
		
	elif (choice == "exit"):
		print("Exit from the program")
		break
		
	else:
		continue
	
	
