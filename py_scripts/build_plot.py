from matplotlib import pyplot as plt


def plot(threads, times):
    plt.plot(threads, times, marker='o')
    plt.xlabel("Amount of threads")
    plt.ylabel("Time (s)")
    plt.xticks(threads)
    plt.ylim(0)
    plt.show()


def read_file(filename):
    with open(filename, "r", encoding='utf-8', errors='ignore') as f:
        text = f.readlines()

    threads = []
    times = []
    for line in text:
        splitted = line.split(" ")
        threads.append(float(splitted[1]))
        times.append(int(float(splitted[3])))
    return threads, times


if __name__ == "__main__":
    threads, times = read_file("../results/thread_time_dependency.txt")
    plot(threads, times)
