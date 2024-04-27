import os
from concurrent.futures import ThreadPoolExecutor,as_completed
import subprocess

AttackString_directory = '/home/huanghong/EvilStrGen/EvilStrGenGrep_AttackString/'
Regex_directory = '/home/huanghong/EvilStrGen/EvilStrGenGrep_AttackString/Regex/'
Dataset_directory = '/home/huanghong/EvilStrGen/regex_set/'


def run(Regex_File, AttackString_Out, Engine_Type, Length):
    try:
        command = './EvilStrGenGrep_AttackString/EvilStrGen ' + ("%s %s %s %s 0" % (Regex_File, AttackString_Out, Engine_Type, Length))
        completed_process = subprocess.Popen(command, shell=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        stdout, stderr = completed_process.communicate(timeout=600)
        completed_process.wait()
        completed_process.terminate()
    except subprocess.TimeoutExpired:
        print("Time Out")
        completed_process.kill()
        completed_process.wait()

Regex_list = []
with open(Dataset_directory + 'ABOVE20.txt', 'r', encoding='utf-8') as f:   # open the dataset
    count = 0
    for line in f:
        count += 1
        Regex_list.append(line)

for index in range(len(Regex_list)):   # A file has a regular expression
    f = open(Regex_directory + str(index + 1) + '.txt', 'w')
    f.write(Regex_list[index])
    f.close()

thread_num = 14  # you can set the number of thread according to your machine

with ThreadPoolExecutor(max_workers=thread_num) as executor:
    # reverse every regex file
    for root, dirs, files in os.walk(Regex_directory):
        for file in files:
            Regex_File = root + file
            AttackString_Out = file[0:-4]
            # print(AttackString_Out)
            executor.submit(run, Regex_File, AttackString_Out, 1, 100000)