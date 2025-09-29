import os
import subprocess
from threading import Timer

class Philosopher:
    def __init__(self, index, used_utensils = None, eating_time = None):
        self.index = index
        self.used_utensils = used_utensils
        self.eating = True
        self.eating_time = eating_time
        timer_time = eating_time if eating_time else None
        # add a few seconds of buffer to the timer to avoid false positives
        self.timer = Timer(timer_time + 5, self.timer_callback) if timer_time else None
        self.timer.start() if self.timer else None

    def timer_callback(self):
        # timer should be stopped when philosopher sets down utensil the second utensil
        # so if the timer expires, they must still be holding a utensil
        assert False, f"Philosopher {self.index} didn't return utensils in {self.eating_time} seconds!"

    def __eq__(self, other):
        return self.index == other.index

NUM_PHILOSOPHERS = 5
script_stopped = False

def main():
    # clear any stale logs
    file_to_delete = "output.log"
    if os.path.exists(file_to_delete):
        os.remove(file_to_delete)
        print("Deleted old log file")

    # Run the dining philosophers program
    print("Starting subprocess")
    process = subprocess.Popen(["./dining_philosophers"], stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
    print("Subprocess running")

    # start test loop
    try:
        test_loop(process)
    except AssertionError as exception:
        print(f"Test failed! {str(exception)}")
        process.terminate()
    # wait for subprocess to finish - should be handled in the test loop but let's be sure
    process.wait()
    print("Exit main")

def test_loop(process: subprocess.Popen):
    print("Start test loop")
    used_utensils = []
    active_philos = []
    while True:
        try:
            line = process.stdout.readline()
            # when process ends, poll() will return the return code
            if process.poll() is not None:
                print("Process shutdown completed")
                break
            if line:
                parse_line(line.strip(), used_utensils, active_philos)
        except KeyboardInterrupt:
            print("KeyboardInterrupt received, terminating subprocess and finishing test")
            global script_stopped
            script_stopped = True
    print("Test loop complete")


def parse_line(line: str, used_utensils: list[int], active_philos: list[Philosopher]):
    # Example line: "Philosopher 2 is eating with utensils 1 and 2 for 3 seconds"
    # Example line: "Philosopher 2 is releasing utensil 1"
    print(f"Live Output: {line}", flush=True)

    if "started eating" in line:
        eating = True
    elif "releasing utensil" in line:
        eating = False
    else:
        return

    # parse out our philosopher and utensils from the string
    numbers = [int(word) for word in line.split() if word.isdigit()]
    if eating:
        assert len(numbers) >= 3, f"Could not parse line: {line}"
        utensils = numbers[1:-1] # index 0 is philosopher, last index is seconds
        eating_time = numbers[-1]
    else:
        assert len(numbers) == 2, f"Could not parse line: {line}"
        utensils = [numbers[1]] # index 0 is philosopher, index 1 is utensil
        eating_time = None

    # assert that this philosopher is not already eating if they are starting to eat
    # and that they are already eating if they are stopping eating
    philo = Philosopher(numbers[0], utensils, eating_time)
    check_philosopher(eating, philo, utensils, active_philos)

    # assert that the utensils are not already in use if they are starting to eat
    # and that they are already in use if they are setting them down
    check_utensils(eating, philo, utensils, used_utensils, script_stopped)

    # assert that the adjacent philosophers are not eating at the same time
    check_adjacent_philosophers(eating, philo, active_philos)

    # TODO: check for starvation - that every philosopher gets to eat a certain fraction of the time


def check_philosopher(eating: bool, philo: Philosopher, utensils: list[int], active_philos: list[Philosopher]):
    if eating:
        assert philo not in active_philos, f"Philosopher {philo.index} tried to eat while already eating!"
        active_philos.append(philo)
        assert len(active_philos) <= NUM_PHILOSOPHERS, "More active philosophers than total philosophers!"
    else:
        # allow philosopher to set down their utensil without eating if the script is stopping
        if not script_stopped:
            assert philo in active_philos, f"Philosopher {philo.index} tried to stop eating while already not eating!"
        try:
            philo = active_philos[active_philos.index(philo)]
        except ValueError:
            philo = None
        if philo is not None:
            philo.used_utensils.remove(utensils[0])
            philo.eating = False
            if not philo.used_utensils:
                assert philo.timer is not None, f"Philosopher {philo.index} should have a timer if they were eating!"
                assert philo.timer.is_alive(), f"Philosopher {philo.index}'s timer should be alive if they were eating!"
                philo.timer.cancel()
                active_philos.remove(philo)


def check_utensils(eating: bool, philo: Philosopher, utensils: list[int], used_utensils: list[int], script_stopped: bool):
    if eating:
        assert not any(utensil in used_utensils for utensil in utensils), f"Philosopher {philo.index} tried to use a utensil that was already in use!"
        used_utensils.extend(utensils)
    else:
        # allow philosopher to set down their utensil without eating if the script is stopping
        if not script_stopped:
            assert utensils[0] in used_utensils, f"Philosopher {philo.index} tried to return utensil {utensils[0]} that wasn't in use!"
            used_utensils.remove(utensils[0])


def check_adjacent_philosophers(eating: bool, philo: Philosopher, active_philos: list[Philosopher]):
    left_philo = Philosopher((philo.index - 1) % NUM_PHILOSOPHERS) # -1 % 5 wraps around to 4
    right_philo = Philosopher((philo.index + 1) % NUM_PHILOSOPHERS)
    if eating:
        # need to check if they're eating, because they might be active but not eating if they just set down one utensil
        eating_philos = [philo for philo in active_philos if philo.eating]
        assert not any(philo in eating_philos for philo in [left_philo, right_philo]), f"Philosopher {philo.index} tried to eat while an adjacent philosopher was eating!"



# run the program
if __name__ == "__main__":
    main()