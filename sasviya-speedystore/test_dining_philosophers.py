import os
import subprocess

class Philosopher:
    def __init__(self, index, used_utensils = None):
        self.index = index
        self.used_utensils = used_utensils
        self.eating = True

    def __eq__(self, other):
        return self.index == other.index

num_philosophers = 5
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
            process.terminate()
    print("Test loop complete")


def parse_line(line: str, used_utensils: list[int], active_philos: list[Philosopher]):
    # Example line: "Philosopher 2 is eating with utensils 1 and 2 for 3 seconds"
    # Example line: "Philosopher 2 is releasing utensil 1"
    print(f"Live Output: {line}")

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
    else:
        assert len(numbers) == 2, f"Could not parse line: {line}"
        utensils = [numbers[1]] # index 0 is philosopher, index 1 is utensil

    # assert that this philosopher is not already eating if they are starting to eat
    # and that they are already eating if they are stopping eating
    philo = Philosopher(numbers[0], utensils)
    if eating:
        assert philo not in active_philos, f"Philosopher {philo.index} tried to eat while already eating!"
        active_philos.append(philo)
        assert len(active_philos) <= num_philosophers, "More active philosophers than total philosophers!"
    else:
        # allow philosopher to set down their utensil without eating if the script is stopping
        if not script_stopped:
            assert philo in active_philos, f"Philosopher {philo.index} tried to stop eating while already not eating!"
            active_philos[active_philos.index(philo)].used_utensils.remove(utensils[0])
            active_philos[active_philos.index(philo)].eating = False
            if not active_philos[active_philos.index(philo)].used_utensils:
                active_philos.remove(philo)

    # assert that the utensils are not already in use if they are starting to eat
    # and that they are already in use if they are setting them down
    if eating:
        assert not any(utensil in used_utensils for utensil in utensils), f"Philosopher {philo.index} tried to use a utensil that was already in use!"
        used_utensils.extend(utensils)
    else:
        # allow philosopher to set down their utensil without eating if the script is stopping
        if not script_stopped:
            assert utensils[0] in used_utensils, f"Philosopher {philo.index} tried to return utensil {utensils[0]} that wasn't in use!"
            used_utensils.remove(utensils[0])

    # assert that the adjacent philosophers are not eating at the same time
    left_philo = Philosopher((philo.index - 1) % 5)
    right_philo = Philosopher((philo.index + 1) % 5)
    if eating:
        eating_philos = [philo for philo in active_philos if philo.eating]
        assert not any(philo in eating_philos for philo in [left_philo, right_philo]), f"Philosopher {philo.index} tried to eat while an adjacent philosopher was eating!"



# run the program
main()