import csv
from pprint import pprint
import os
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
this_file_path = os.path.dirname(os.path.abspath(__file__))
PATH_FORMATED = os.path.join(this_file_path,PATH_FORMATED)
i = 0
for b in range(5):
    with open(PATH_FORMATED, newline='') as csvfile:
        spamreader = csv.reader(csvfile,delimiter =',')
        for row in spamreader:
                m = mcs()
                m.id = i
                m.mod = row[2]
                m.cr = row[3]
                m.bw = 2**b
                m.dr = float(row[4+b*2])
                if m.dr == 0:
                    continue
                MCS_LIST.append(m)
                print(m.__dict__)


print(len(MCS_LIST))

for m in MCS_LIST:
    rate_str = "%.2f" % m.dr
    print(rate_str.split('.'))
    print('_'.join(rate_str.split('.')))
    rate_str = '_'.join(rate_str.split('.'))
    m.rate_str  = rate_str
    m.uni_name = 'OfdmRate'+m.rate_str+'MbpsBW'+str(m.bw)+'MHz'

    if m.mod == 'BPSK':
        m.mod_constellation = 2
    if m.mod == 'QPSK':
        m.mod_constellation = 4
    if m.mod == '16-QAM':
        m.mod_constellation = 16
    if m.mod == '64-QAM':
        m.mod_constellation = 64
    if m.mod == '256-QAM':
        m.mod_constellation = 256

    print(m.cr)
    if m.cr == '1/2':
        m.code_rate_str = "WIFI_CODE_RATE_1_2"
    if m.cr == '2/3':
        m.code_rate_str = "WIFI_CODE_RATE_3_4"
    if m.cr == '3/4':
        m.code_rate_str = "WIFI_CODE_RATE_3_4"
    if m.cr == '5/6':
        m.code_rate_str = "WIFI_CODE_RATE_5_6"


PATH = 'gen_s1g_rates_fnames_h.txt'
wf = open(PATH,'w')
for m in MCS_LIST:
    '''
        /**
        * Return a WifiMode for OFDM at 70.2Mbps with 16MHz channel spacing.
        *
        * \return a WifiMode for OFDM at 70.2Mbps with 16MHz channel spacing
        */
        static WifiMode GetS1gOfdmRate70_2MbpsBW16MHz ();
    '''
    wf.write('\t\tstatic WifiMode GetS1g'+m.uni_name+' ();\n')

PATH = 'gen_s1g_rates_lookuptable_h.txt'
wf = open(PATH,'w')
for m in MCS_LIST:
    '''
    { "OfdmRate54Mbps",         { WIFI_CODE_RATE_3_4, 64 } },
    '''
    wf.write('\t{"S1g'+m.uni_name+'",\t{'+m.code_rate_str+',\t'+str(m.mod_constellation)+'}},\n')


PATH = 'gen_s1g_getofdmmod_h.txt'
wf = open(PATH,'w')
for m in MCS_LIST:
    '''
    GET_S1G_OFDM_MODE (S1gOfdmRate54Mbps, false)
    '''
    wf.write('\tGET_S1G_OFDM_MODE (S1g'+m.uni_name+', true)\n')

PATH = 'gen_s1g_bpslist_h.txt'
wf = open(PATH,'w')
wf.write('{\n')
for b in range(5):
    wf.write('\t{'+str(2**b)+', {')
    for m in MCS_LIST:
        '''
        GET_S1G_OFDM_MODE (S1gOfdmRate54Mbps, false)
        '''
        if m.bw == 2**b:
            wf.write(str(int(m.dr*1e6))+',')

    wf.write('}},\n')
wf.write('};\n')


PATH = 'gen_s1g_getofdmrates_h.txt'
wf = open(PATH,'w')
for b in range(5):
    wf.write('\tcase '+str(2**b)+':\n')
    wf.write('\t\t switch (rate)\n')
    wf.write('\t\t{\n')
    for m in MCS_LIST:
        '''
          case 6000000:
              return GetOfdmRate6Mbps ();
        '''
        if m.bw == 2**b:
            wf.write('\t\t\tcase '+str(int(m.dr*1e6))+':\n')
            wf.write('\t\t\t\treturn GetS1g'+m.uni_name+'();\n')

    wf.write('\t\t\tdefault:\n')
    wf.write('\t\t\t\tNS_ABORT_MSG ("Inexistent rate (" << rate << " bps) requested for s1g OFDM (default)");\n')
    wf.write('\t\t\t\treturn WifiMode ();\n')
    wf.write('\t\t}\n')


PATH = 'gen_s1g_mode_band_lookuptable_h.txt'
wf = open(PATH,'w')
for m in MCS_LIST:
    '''
    { "OfdmRate54Mbps",         { WIFI_CODE_RATE_3_4, 64 } },
    '''
    wf.write('\t{"S1g'+m.uni_name+'",'+str(m.bw)+'},\n')