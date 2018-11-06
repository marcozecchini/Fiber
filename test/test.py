import subprocess 
import sys
import matplotlib.pyplot as plt
import numpy as np


while(True):
	choice = raw_input("Digit something among \"fiber\", \"user\", \"plot\" and \"exit\": ")
	if (choice == "fiber"):
		file_fiber = open("output_fiber.txt", "w")
		
		for i in range (0,16):
			res = subprocess.Popen(["../2018-fibers/test", str((i+1)*16)], stdout = subprocess.PIPE)
			time = res.stdout.read().split("\n")
			file_fiber.write(time[2].split(":")[1].strip()+" "+ time[1].split(":")[1].strip()+"\n")
			print("Iteration number "+str(i))
		file_fiber.close()
	
	elif (choice == "user"):
		file_fiber_usr = open("output_fiber_usr.txt", "w")
		for i in range(0,16):
			res = subprocess.Popen(["../2018-fibers-usr/test", str((i+1)*16)], stdout = subprocess.PIPE)
			str_res = res.stdout.read()
			file_fiber_usr.write(str_res.split("Time to run do the work (per-fiber): ")[1].strip() + " " + str_res.split("Time to initialize fibers: ")[1].split("\n")[0]+"\n")
			print("Iteration number "+str(i))		
		file_fiber_usr.close()
	elif (choice == "plot"):
		f1 = open("output_fiber.txt", "r")
		f2 = open("output_fiber_usr.txt", "r")
		list1 = list()
		list2 = list()
		init1 = list()
		init2 = list()
		
		for line in f1:
			n = line.split(" ")
			list1.append(float(n[0]))
			init1.append(float(n[1]))
		for line in f2:
			n = line.split(" ")
			list2.append(float(n[0]))
			init2.append(float(n[1]))
		plt.xlabel("Iteration")
		plt.ylabel("Time per fiber")
		plt.plot(range(1, len(list1)+1), list1, marker = "o", label="Kernel")
		plt.plot(range(1, len(list2)+1), list2, marker = "o", label="User")
		plt.legend()
		plt.show()
		
		
		plt.xlabel("Iteration")
		plt.ylabel("Initialization time")
		plt.plot(range(1, len(init1)+1), init1, marker = "o", label="Kernel")
		plt.plot(range(1, len(init2)+1), init2, marker = "o", label="User")
		plt.legend()
		plt.show()
		
	elif (choice == "exit"):
		print("Exit from the program")
		break
		
	else:
		continue
	
	
