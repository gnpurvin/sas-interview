The Dining Philosophers problem is a classic computer science model demonstrating the challenges and solutions for parallel processing and resource contention. Imagine five philosophers sitting at a dinner table, and between each pair of philosophers, there is a dining utensil. To eat the dish before them, they need two utensils, meaning that when one philosopher is eating, neither neighbor can eat at the same time.

Using C code, please write a program that simulates the Dining Philosopher program with the following requirements:
• Each philosopher should be simulated in their own thread
• Each philosopher should wait for a random interval before starting to eat and then eat for a random interval before stopping.
• When a philosopher is eating, their neighbors may not eat.
••• One option is that they may give up and try to eat later
••• Another option is to wait until the utensils they need are freed up
• There should be a way to tell when philosophers start or stop eating, such as a message log.
• The simulation should run endlessly until the program is stopped.

If you are unfamiliar with C threading libraries, you are expected to learn them to complete the exercise. Your program does not have to be perfect! During the interview, you will present the source code, the application running on your machine, and your testing strategies.