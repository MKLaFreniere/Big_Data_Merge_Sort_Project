# Big Data - Sort and Search

This was a very interesting project I did for my Massive Data Management course. It is
based on LSM trees, as I had just learned about them at the time and wanted to try to implement one in some way.

The final product of this goal is 2 separate programs that run in a Linux environment. MergeSort.cpp takes a write 
transaction file and sorts it by key, though using limited memory. To do this, the file is separated by the memory limit and put into
sections within txt files. Each of these files are sorted independently of each other. The program then sorts all these txt files
completely. As an example: txt1 would go from 1 to 5, txt2 would go from 6 to 9. The actual keys that are sorted are more complicated however.
Once this is done, there is a set of kv stores that have sorted all keys pertaining to the data. 

The Read.cpp allows a user to input a read transaction file. This gives the user a list of the data related to its respective key. Because of the
way this whole thing is sorted, it is much faster to read than other more basic methods, with the addition of using less memory for the entire process.

To run this program, log into a linux environment (I used version gcc (GCC) 4.8.5 20150623 (Red Hat 4.8.5-44)) and type 'make' into the proper directory. 
This will compile the programs. Next, make sure the transaction files are in the same directory as the program and follow the on screen instructions. 
After a program is done, you can look at the directory to see the result.

While doing this project, I learned a lot more than just how a MergeSort algorithm is used to efficiently store data. For instance, this project 
was a great introduction to working in a Linux environment with C++. There were many times where I needed to use functions that people normally wouldn't use in
C++, such as posix for quick reading of a file as well as some major differences between C and C++. This sort of unfamiliarity quickly got
me looking up dense Linux manuals that at first I felt were intimidating. By the end though, I feel I learned a lot on how to push past this
intimidation and I am very grateful for the experience!
