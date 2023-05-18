import subprocess
import os

empty_file = "pusty.txt"

open(empty_file, 'w')

test_failed = False
def test_program(prog_path, args, expected_return_code):
    cmd = [prog_path] + args
    try:
        with open(os.devnull, 'w') as devnull:
            return_code = subprocess.run(cmd, stdout=devnull, stderr=devnull, stdin=open(empty_file, 'r'))
            return_code = return_code.returncode
        if return_code == expected_return_code:
            # print(f"Test passed for command: {' '.join(cmd)}")
            pass
        else:
            global test_failed
            test_failed = True
            print(f"Test failed for command: {' '.join(cmd)}. Expected return code: {expected_return_code}, Actual return code: {return_code}")
    except FileNotFoundError:
        print(f"Error: Program '{prog_path}' not found.")

prog = "./sikradio-sender"

# Example test cases
# test_program(prog, ["-a", "-1"], -1)
# test_program(prog, ["-a", "1"], 0)



OK = 0
ERROR = 1

def test_ok(args):
    test_program(prog, args, OK)

def test_error(args):
    test_program(prog, args, ERROR)

test_ok(["-a", "1.1.1.1", "-p", "1"])
test_ok(["-a", "1.1.1.1", "-p", "66000"])

test_error(["-a", "1.1.1.1", "-p", "-1"])
test_error(["-a", "1.1.1.1", "-p", "0"])
test_error(["-a", "1.1.1.1", "-p", "a2"])

test_error(["-a", "1.1.1.1", "-f", "-1"])
test_error(["-a", "1.1.1.1", "-f", "0"])
test_error(["-a", "1.1.1.1", "-f", "a2"])

test_error(["-a", "1.1.1.1", "-R", "-1"])
test_error(["-a", "1.1.1.1", "-R", "a2"])
test_error(["-a", "1.1.1.1", "-R", "0"])

test_error(["-a", "1.1.1.1", "-n", "\"\""])

test_error(["-a", "1.1.1.1", "-P", "800000"])
test_error(["-a", "1.1.1.1", "-P", "-1"])
test_error(["-a", "127.0.0.1", "-P", "2534a"])

test_error(["-a", "1.1.1.1", "-C", "800000"])
test_error(["-a", "1.1.1.1", "-C", "-1"])
test_error(["-a", "127.0.0.1", "-C", "2534a"])



if test_failed:
    print("test FAILED!!!")
else:
    print("test OK!!!")

os.remove(empty_file)