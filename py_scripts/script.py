from os import chdir, getcwd, system, path
import sys
import math
import hashlib

def build():
    cwd = getcwd()
    chdir(cwd[:len(cwd)-len("/py_scripts")])
    cwd = getcwd()
    system("mkdir build")
    chdir(cwd + "/build")
    system("cmake ..")
    system("make")


def run_cpp(to_search, config_path):
    cwd = getcwd()
    script = "/py_scripts"
    if cwd[len(cwd)-len(script):] == script:
        chdir(cwd[:len(cwd)-len(script)])
        cwd = getcwd()
    bld = "/build"
    if cwd[len(cwd) - len(bld):] != bld:
        chdir(cwd + bld)
    # giving config.dat creates an error
    system('./lab_parallel_grep ' + to_search + ' ' + config_path)


def read_output(path):
    with open(path, "r", encoding='utf-8', errors='ignore') as output_file:
        output = output_file.readlines()

    res = []
    for line in output:
        res.append(float(line))
    return res


def hashfile(file):
    BUF_SIZE = 65536
    sha256 = hashlib.sha256()
    with open(file, 'rb') as f:
        while True:
            data = f.read(BUF_SIZE)
            if not data:
                break
            sha256.update(data)
    return sha256.hexdigest()

def compare(compare_array):
    least_time = math.inf

    for i in range(len(compare_array)):
        check_time = compare_array[i][0]
        if check_time < least_time:
            least_time = check_time
    return least_time


def change_threads(cfg_path, new_amount):
    output_file = open(cfg_path, "r")
    output = output_file.readlines()

    for line in output:
        name_of_line = "indexing_threads"

        if line[:len(name_of_line)] == name_of_line:
            output[output.index(line)] = line.replace(line[len(name_of_line) + 3:], str(new_amount))+"\n"

    output_file = open(cfg_path, "w")
    output_file.writelines(output)
    output_file.close()


def change_filenames(cfg_path, amount):
    output_file = open(cfg_path, "r")
    output = output_file.readlines()

    for line in output:
        name_of_line = "res_file"

        if line[:len(name_of_line)] == name_of_line:
            if line[-6].isdigit():
                output[output.index(line)] = line[:-6] + str(amount) + line[-5:]
            else:
                output[output.index(line)] = line[:-5] + str(amount) + line[-5:]

    output_file = open(cfg_path, "w")
    output_file.writelines(output)
    output_file.close()


if __name__ == '__main__':
    number = 1
    
    if len(sys.argv) >= 2:
        to_search = str(sys.argv[1])
        if len(sys.argv) >= 3 and sys.argv[2].isnumeric():
            number = int(sys.argv[2])
        else:
            print("\nIncorrect input\nNumber of simulations will be 1\n")
    else:
        print("\nERROR\nNo string inputted\nInput string to search\nQuitting")
        quit()

    build()
    time_path = "../results/total_time.txt"
    cnf_path = "../config/config.dat"

    cwd = getcwd()
    chdir(cwd[:len(cwd)-len("/build")])
    system("mkdir results")
    chdir(cwd)
    hashes = []
    with open("../results/thread_time_dependency.txt", "w") as f:
        # Change to 1, 8 after testing
        for threads in range(1, 8):
            change_threads(cnf_path, threads)
            change_filenames(cnf_path, threads)
            compare_arr = []
            for _ in range(number):
                run_cpp(to_search, cnf_path)
                compare_arr.append(read_output(time_path))
                temp_hash = hashfile("../results/result_thread" + str(threads) + ".txt")
                for i in hashes:
                    if temp_hash != i:
                        print("Hashes are different")
                hashes.append(temp_hash)
            time = compare(compare_arr)
            print("\n")
            string = "Threads: " + str(threads) + " time: " + str(time) + " s."
            print(string+"\n\n")

            f.write(string + "\n")
