import csv
from pprint import pprint

##use this code to format the mcs table from
# https://en.wikipedia.org/wiki/IEEE_802.11ah
#
# PATH = 'mcs_data_rate_table_formated_0.txt'
# PATH_NEW = 'mcs_data_rate_table_formated.txt'
# wf = open(PATH_NEW,'w')
# with open(PATH, newline='') as csvfile:
#     spamreader = csv.reader(csvfile,delimiter =',')
#     for row in spamreader:
#         # print(row)
#         wf.write(','.join(row))
#         wf.write('\n')


class mcs:
    id = 0
    mod = 0
    cr = 0
    bw = 0
    dr = 0
    gi_type = 0


MCS_LIST = []
PATH_FORMATED = 'mcs_data_rate_table_formated.txt'
i = 0
with open(PATH_FORMATED, newline='') as csvfile:
    spamreader = csv.reader(csvfile,delimiter =',')
    for row in spamreader:
        for b in range(5):
            m = mcs()
            m.id = i
            m.mod = row[2]
            m.rc = row[3]
            m.bw = 2**b
            m.dr = float(row[4+b*2])
            if m.dr == 0:
                continue
            MCS_LIST.append(m)
            print(m.__dict__)


print(len(MCS_LIST))

PATH = 'gen_s1g_rates_fnames.txt'


